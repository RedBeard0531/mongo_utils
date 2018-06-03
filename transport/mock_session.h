/**
 *    Copyright (C) 2016 MongoDB Inc.
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

#include "mongo/base/checked_cast.h"
#include "mongo/transport/session.h"
#include "mongo/transport/transport_layer_mock.h"
#include "mongo/util/net/hostandport.h"

namespace mongo {
namespace transport {

class MockSession : public Session {
    MONGO_DISALLOW_COPYING(MockSession);

public:
    static std::shared_ptr<MockSession> create(TransportLayer* tl) {
        std::shared_ptr<MockSession> handle(new MockSession(tl));
        return handle;
    }

    static std::shared_ptr<MockSession> create(HostAndPort remote,
                                               HostAndPort local,
                                               TransportLayer* tl) {
        std::shared_ptr<MockSession> handle(
            new MockSession(std::move(remote), std::move(local), tl));
        return handle;
    }

    TransportLayer* getTransportLayer() const override {
        return _tl;
    }

    const HostAndPort& remote() const override {
        return _remote;
    }

    const HostAndPort& local() const override {
        return _local;
    }

    void end() override {
        if (!_tl || !_tl->owns(id()))
            return;
        _tl->_sessions[id()].ended = true;
    }

    StatusWith<Message> sourceMessage() override {
        if (!_tl || _tl->inShutdown()) {
            return TransportLayer::ShutdownStatus;
        } else if (!_tl->owns(id())) {
            return TransportLayer::SessionUnknownStatus;
        } else if (_tl->_sessions[id()].ended) {
            return TransportLayer::TicketSessionClosedStatus;
        }

        return Message();  // Subclasses can do something different.
    }

    Future<Message> asyncSourceMessage(const transport::BatonHandle& handle = nullptr) override {
        return Future<Message>::makeReady(sourceMessage());
    }

    Status sinkMessage(Message message) override {
        if (!_tl || _tl->inShutdown()) {
            return TransportLayer::ShutdownStatus;
        } else if (!_tl->owns(id())) {
            return TransportLayer::SessionUnknownStatus;
        } else if (_tl->_sessions[id()].ended) {
            return TransportLayer::TicketSessionClosedStatus;
        }

        return Status::OK();
    }

    Future<void> asyncSinkMessage(Message message,
                                  const transport::BatonHandle& handle = nullptr) override {
        return Future<void>::makeReady(sinkMessage(message));
    }

    void cancelAsyncOperations(const transport::BatonHandle& handle = nullptr) override {}

    void setTimeout(boost::optional<Milliseconds>) override {}

    bool isConnected() override {
        return true;
    }

    explicit MockSession(TransportLayer* tl)
        : _tl(checked_cast<TransportLayerMock*>(tl)), _remote(), _local() {}
    explicit MockSession(HostAndPort remote, HostAndPort local, TransportLayer* tl)
        : _tl(checked_cast<TransportLayerMock*>(tl)),
          _remote(std::move(remote)),
          _local(std::move(local)) {}

protected:
    TransportLayerMock* _tl;

    HostAndPort _remote;
    HostAndPort _local;
};

}  // namespace transport
}  // namespace mongo
