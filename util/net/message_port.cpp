// message_port.cpp

/*    Copyright 2009 10gen Inc.
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 */

#define MONGO_LOG_DEFAULT_COMPONENT ::mongo::logger::LogComponent::kNetwork

#include "mongo/platform/basic.h"

#include "mongo/util/net/message_port.h"

#include <fcntl.h>
#include <time.h>

#include "mongo/config.h"
#include "mongo/util/allocator.h"
#include "mongo/util/background.h"
#include "mongo/util/log.h"
#include "mongo/util/net/listen.h"
#include "mongo/util/net/message.h"
#include "mongo/util/net/socket_exception.h"
#include "mongo/util/net/ssl_manager.h"
#include "mongo/util/net/ssl_options.h"
#include "mongo/util/scopeguard.h"
#include "mongo/util/time_support.h"

#ifndef _WIN32
#ifndef __sun
#include <ifaddrs.h>
#endif
#include <sys/resource.h>
#include <sys/stat.h>
#endif

namespace mongo {

using std::shared_ptr;
using std::string;

/* messagingport -------------------------------------------------------------- */

MessagingPort::MessagingPort(int fd, const SockAddr& remote)
    : MessagingPort(std::make_shared<Socket>(fd, remote)) {}

MessagingPort::MessagingPort(double timeout, logger::LogSeverity ll)
    : MessagingPort(std::make_shared<Socket>(timeout, ll)) {}

MessagingPort::MessagingPort(std::shared_ptr<Socket> sock)
    : _x509PeerInfo(), _connectionId(), _tag(), _psock(std::move(sock)) {
    SockAddr sa = _psock->remoteAddr();
    _remoteParsed = HostAndPort(sa.getAddr(), sa.getPort());
}

void MessagingPort::setTimeout(Milliseconds millis) {
    double timeout = double(millis.count()) / 1000;
    _psock->setTimeout(timeout);
}

void MessagingPort::shutdown() {
    _psock->close();
}

MessagingPort::~MessagingPort() {
    shutdown();
}

bool MessagingPort::recv(Message& m) {
    try {
#ifdef MONGO_CONFIG_SSL
    again:
#endif
        MSGHEADER::Value header;
        _psock->recv((char*)&header, sizeof(header));
        int len = header.constView().getMessageLength();

        if (len == 542393671) {
            // an http GET
            string msg =
                "It looks like you are trying to access MongoDB over HTTP on the native driver "
                "port.\n";
            LOG(_psock->getLogLevel()) << msg;
            std::stringstream ss;
            ss << "HTTP/1.0 200 OK\r\nConnection: close\r\nContent-Type: "
                  "text/plain\r\nContent-Length: "
               << msg.size() << "\r\n\r\n"
               << msg;
            string s = ss.str();
            send(s.c_str(), s.size(), "http");
            return false;
        }
        // If responseTo is not 0 or -1 for first packet assume SSL
        else if (_psock->isAwaitingHandshake()) {
#ifndef MONGO_CONFIG_SSL
            if (header.constView().getResponseToMsgId() != 0 &&
                header.constView().getResponseToMsgId() != -1) {
                uasserted(17133,
                          "SSL handshake requested, SSL feature not available in this build");
            }
#else
            if (header.constView().getResponseToMsgId() != 0 &&
                header.constView().getResponseToMsgId() != -1) {
                uassert(17132,
                        "SSL handshake received but server is started without SSL support",
                        sslGlobalParams.sslMode.load() != SSLParams::SSLMode_disabled);
                setX509PeerInfo(
                    _psock->doSSLHandshake(reinterpret_cast<const char*>(&header), sizeof(header)));
                LOG(1) << "new ssl connection, SNI server name [" << _psock->getSNIServerName()
                       << "]";
                _psock->setHandshakeReceived();

                goto again;
            }

            auto sslMode = sslGlobalParams.sslMode.load();

            uassert(17189,
                    "The server is configured to only allow SSL connections",
                    sslMode != SSLParams::SSLMode_requireSSL);

            // For users attempting to upgrade their applications from no SSL to SSL, provide
            // information about connections that still aren't using SSL (but only once per
            // connection)
            if (!sslGlobalParams.disableNonSSLConnectionLogging &&
                (sslMode == SSLParams::SSLMode_preferSSL)) {
                LOG(0) << "SSL mode is set to 'preferred' and connection " << _connectionId
                       << " to " << remote() << " is not using SSL.";
            }

#endif  // MONGO_CONFIG_SSL
        }
        if (static_cast<size_t>(len) < sizeof(header) ||
            static_cast<size_t>(len) > MaxMessageSizeBytes) {
            LOG(0) << "recv(): message len " << len << " is invalid. "
                   << "Min " << sizeof(header) << " Max: " << MaxMessageSizeBytes;
            return false;
        }

        _psock->setHandshakeReceived();

        auto buf = SharedBuffer::allocate(len);
        MsgData::View md = buf.get();
        memcpy(md.view2ptr(), &header, sizeof(header));

        const int left = len - sizeof(header);
        if (left)
            _psock->recv(md.data(), left);

        m.setData(std::move(buf));
        return true;

    } catch (const NetworkException& e) {
        LOG(_psock->getLogLevel()) << "NetworkException: remote: " << remote() << " error: " << e;
        m.reset();
        return false;
    }
}

bool MessagingPort::call(const Message& toSend, Message& response) {
    say(toSend);
    bool success = recv(response);
    if (success) {
        invariant(!response.empty());
        if (response.header().getResponseToMsgId() != toSend.header().getId()) {
            response.reset();
            uasserted(40134, "Response ID did not match the sent message ID.");
        }
    }
    return success;
}

void MessagingPort::say(const Message& toSend) {
    invariant(!toSend.empty());
    auto buf = toSend.buf();
    if (buf) {
        send(buf, MsgData::ConstView(buf).getLen(), "say");
    }
}

HostAndPort MessagingPort::remote() const {
    return _remoteParsed;
}

SockAddr MessagingPort::remoteAddr() const {
    return _psock->remoteAddr();
}

SockAddr MessagingPort::localAddr() const {
    return _psock->localAddr();
}

void MessagingPort::setX509PeerInfo(SSLPeerInfo x509PeerInfo) {
    _x509PeerInfo = std::move(x509PeerInfo);
}

const SSLPeerInfo& MessagingPort::getX509PeerInfo() const {
    return _x509PeerInfo;
}

void MessagingPort::setConnectionId(const long long connectionId) {
    _connectionId = connectionId;
}

long long MessagingPort::connectionId() const {
    return _connectionId;
}

void MessagingPort::setTag(const AbstractMessagingPort::Tag tag) {
    _tag = tag;
}

AbstractMessagingPort::Tag MessagingPort::getTag() const {
    return _tag;
}

}  // namespace mongo
