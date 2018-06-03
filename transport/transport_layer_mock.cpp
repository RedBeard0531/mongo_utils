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

#include "mongo/platform/basic.h"

#include "mongo/transport/transport_layer_mock.h"

#include "mongo/base/status.h"
#include "mongo/stdx/memory.h"
#include "mongo/transport/mock_session.h"
#include "mongo/transport/transport_layer.h"
#include "mongo/util/time_support.h"

namespace mongo {
namespace transport {

TransportLayerMock::TransportLayerMock() : _shutdown(false) {}

SessionHandle TransportLayerMock::createSession() {
    auto session = createSessionHook ? createSessionHook(this) : MockSession::create(this);
    Session::Id sessionId = session->id();

    _sessions[sessionId] = Connection{false, session, SSLPeerInfo()};

    return _sessions[sessionId].session;
}

SessionHandle TransportLayerMock::get(Session::Id id) {
    if (!owns(id))
        return nullptr;

    return _sessions[id].session;
}

bool TransportLayerMock::owns(Session::Id id) {
    return _sessions.count(id) > 0;
}

StatusWith<SessionHandle> TransportLayerMock::connect(HostAndPort peer,
                                                      ConnectSSLMode sslMode,
                                                      Milliseconds timeout) {
    MONGO_UNREACHABLE;
}

Future<SessionHandle> TransportLayerMock::asyncConnect(HostAndPort peer,
                                                       ConnectSSLMode sslMode,
                                                       const ReactorHandle& reactor) {
    MONGO_UNREACHABLE;
}

Status TransportLayerMock::setup() {
    return Status::OK();
}

Status TransportLayerMock::start() {
    return Status::OK();
}

void TransportLayerMock::shutdown() {
    if (!inShutdown()) {
        _shutdown = true;
    }
}

ReactorHandle TransportLayerMock::getReactor(WhichReactor which) {
    return nullptr;
}

bool TransportLayerMock::inShutdown() const {
    return _shutdown;
}

TransportLayerMock::~TransportLayerMock() {
    shutdown();
}

}  // namespace transport
}  // namespace mongo
