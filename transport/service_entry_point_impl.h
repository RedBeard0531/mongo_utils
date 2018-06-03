/**
 *    Copyright (C) 2017 MongoDB Inc.
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


#include "mongo/base/disallow_copying.h"
#include "mongo/platform/atomic_word.h"
#include "mongo/stdx/condition_variable.h"
#include "mongo/stdx/list.h"
#include "mongo/stdx/mutex.h"
#include "mongo/transport/service_entry_point.h"
#include "mongo/transport/service_state_machine.h"

namespace mongo {
class ServiceContext;

namespace transport {
class Session;
}  // namespace transport

/**
 * A basic entry point from the TransportLayer into a server.
 *
 * The server logic is implemented inside of handleRequest() by a subclass.
 * startSession() spawns and detaches a new thread for each incoming connection
 * (transport::Session).
 */
class ServiceEntryPointImpl : public ServiceEntryPoint {
    MONGO_DISALLOW_COPYING(ServiceEntryPointImpl);

public:
    explicit ServiceEntryPointImpl(ServiceContext* svcCtx);

    void startSession(transport::SessionHandle session) override;

    void endAllSessions(transport::Session::TagMask tags) final;

    bool shutdown(Milliseconds timeout) final;

    Stats sessionStats() const final;

    size_t numOpenSessions() const final {
        return _currentConnections.load();
    }

private:
    using SSMList = stdx::list<std::shared_ptr<ServiceStateMachine>>;
    using SSMListIterator = SSMList::iterator;

    ServiceContext* const _svcCtx;
    AtomicWord<std::size_t> _nWorkers;

    mutable stdx::mutex _sessionsMutex;
    stdx::condition_variable _shutdownCondition;
    SSMList _sessions;

    size_t _maxNumConnections{DEFAULT_MAX_CONN};
    AtomicWord<size_t> _currentConnections{0};
    AtomicWord<size_t> _createdConnections{0};
};

}  // namespace mongo
