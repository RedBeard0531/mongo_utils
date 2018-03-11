// message_port.h

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

#pragma once

#include <vector>

#include "mongo/config.h"
#include "mongo/util/net/abstract_message_port.h"
#include "mongo/util/net/message.h"
#include "mongo/util/net/sock.h"

namespace mongo {

class MessagingPort;

class MessagingPort final : public AbstractMessagingPort {
public:
    MessagingPort(int fd, const SockAddr& remote);

    // in some cases the timeout will actually be 2x this value - eg we do a partial send,
    // then the timeout fires, then we try to send again, then the timeout fires again with
    // no data sent, then we detect that the other side is down
    MessagingPort(double so_timeout = 0, logger::LogSeverity logLevel = logger::LogSeverity::Log());

    MessagingPort(std::shared_ptr<Socket> socket);

    ~MessagingPort() override;

    void setTimeout(Milliseconds millis) override;

    void shutdown() override;

    /* it's assumed if you reuse a message object, that it doesn't cross MessagingPort's.
       also, the Message data will go out of scope on the subsequent recv call.
    */
    bool recv(Message& m) override;
    bool call(const Message& toSend, Message& response) override;

    void say(const Message& toSend) override;

    unsigned remotePort() const override {
        return _psock->remotePort();
    }
    virtual HostAndPort remote() const override;
    virtual SockAddr remoteAddr() const override;
    virtual SockAddr localAddr() const override;

    void send(const char* data, int len, const char* context) override {
        _psock->send(data, len, context);
    }
    void send(const std::vector<std::pair<char*, int>>& data, const char* context) override {
        _psock->send(data, context);
    }
    bool connect(SockAddr& farEnd) override {
        return _psock->connect(farEnd);
    }

    void setLogLevel(logger::LogSeverity ll) override {
        _psock->setLogLevel(ll);
    }

    void clearCounters() override {
        _psock->clearCounters();
    }

    long long getBytesIn() const override {
        return _psock->getBytesIn();
    }

    long long getBytesOut() const override {
        return _psock->getBytesOut();
    }

    void setX509PeerInfo(SSLPeerInfo x509PeerInfo) override;

    const SSLPeerInfo& getX509PeerInfo() const override;

    void setConnectionId(const long long connectionId) override;

    long long connectionId() const override;

    void setTag(const AbstractMessagingPort::Tag tag) override;

    AbstractMessagingPort::Tag getTag() const override;

    /**
     * Initiates the TLS/SSL handshake on this MessagingPort.
     * When this function returns, further communication on this
     * MessagingPort will be encrypted.
     * ssl - Pointer to the global SSLManager.
     * remoteHost - The hostname of the remote server.
     */
    bool secure(SSLManagerInterface* ssl, const std::string& remoteHost) override {
#ifdef MONGO_CONFIG_SSL
        return _psock->secure(ssl, remoteHost);
#else
        return false;
#endif
    }

    bool isStillConnected() const override {
        return _psock->isStillConnected();
    }

    uint64_t getSockCreationMicroSec() const override {
        return _psock->getSockCreationMicroSec();
    }

private:
    // this is the parsed version of remote
    HostAndPort _remoteParsed;
    SSLPeerInfo _x509PeerInfo;
    long long _connectionId;
    AbstractMessagingPort::Tag _tag;
    std::shared_ptr<Socket> _psock;
};

}  // namespace mongo
