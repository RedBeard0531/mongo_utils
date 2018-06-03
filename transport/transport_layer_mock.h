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

#include "mongo/base/status.h"
#include "mongo/stdx/unordered_map.h"
#include "mongo/transport/session.h"
#include "mongo/transport/transport_layer.h"
#include "mongo/util/net/ssl_types.h"
#include "mongo/util/time_support.h"

namespace mongo {
namespace transport {

/**
 * This TransportLayerMock is a noop TransportLayer implementation.
 */
class TransportLayerMock : public TransportLayer {
    MONGO_DISALLOW_COPYING(TransportLayerMock);

public:
    TransportLayerMock();
    ~TransportLayerMock();

    SessionHandle createSession();
    SessionHandle get(Session::Id id);
    bool owns(Session::Id id);

    StatusWith<SessionHandle> connect(HostAndPort peer,
                                      ConnectSSLMode sslMode,
                                      Milliseconds timeout) override;
    Future<SessionHandle> asyncConnect(HostAndPort peer,
                                       ConnectSSLMode sslMode,
                                       const ReactorHandle& reactor) override;

    Status setup() override;
    Status start() override;
    void shutdown() override;
    bool inShutdown() const;


    virtual ReactorHandle getReactor(WhichReactor which) override;

    // Set to a factory function to use your own session type.
    std::function<SessionHandle(TransportLayer*)> createSessionHook;

private:
    friend class MockSession;

    struct Connection {
        bool ended;
        SessionHandle session;
        SSLPeerInfo peerInfo;
    };
    stdx::unordered_map<Session::Id, Connection> _sessions;
    bool _shutdown;
};

}  // namespace transport
}  // namespace mongo
