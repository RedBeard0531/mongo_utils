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

#define MONGO_LOG_DEFAULT_COMPONENT ::mongo::logger::LogComponent::kNetwork

#include "mongo/platform/basic.h"

#include "mongo/transport/service_entry_point_impl.h"

#include <vector>

#include "mongo/db/auth/restriction_environment.h"
#include "mongo/transport/service_state_machine.h"
#include "mongo/transport/session.h"
#include "mongo/util/log.h"
#include "mongo/util/processinfo.h"
#include "mongo/util/scopeguard.h"

#if !defined(_WIN32)
#include <sys/resource.h>
#endif

namespace mongo {
ServiceEntryPointImpl::ServiceEntryPointImpl(ServiceContext* svcCtx) : _svcCtx(svcCtx) {

    const auto supportedMax = [] {
#ifdef _WIN32
        return serverGlobalParams.maxConns;
#else
        struct rlimit limit;
        verify(getrlimit(RLIMIT_NOFILE, &limit) == 0);

        size_t max = (size_t)(limit.rlim_cur * .8);

        LOG(1) << "fd limit"
               << " hard:" << limit.rlim_max << " soft:" << limit.rlim_cur << " max conn: " << max;

        return std::min(max, serverGlobalParams.maxConns);
#endif
    }();

    // If we asked for more connections than supported, inform the user.
    if (supportedMax < serverGlobalParams.maxConns &&
        serverGlobalParams.maxConns != DEFAULT_MAX_CONN) {
        log() << " --maxConns too high, can only handle " << supportedMax;
    }

    _maxNumConnections = supportedMax;
}

void ServiceEntryPointImpl::startSession(transport::SessionHandle session) {
    // Setup the restriction environment on the Session, if the Session has local/remote Sockaddrs
    const auto& remoteAddr = session->remote().sockAddr();
    const auto& localAddr = session->local().sockAddr();
    invariant(remoteAddr && localAddr);
    auto restrictionEnvironment =
        stdx::make_unique<RestrictionEnvironment>(*remoteAddr, *localAddr);
    RestrictionEnvironment::set(session, std::move(restrictionEnvironment));

    SSMListIterator ssmIt;

    const bool quiet = serverGlobalParams.quiet.load();
    size_t connectionCount;
    auto transportMode = _svcCtx->getServiceExecutor()->transportMode();

    auto ssm = ServiceStateMachine::create(_svcCtx, session, transportMode);
    {
        stdx::lock_guard<decltype(_sessionsMutex)> lk(_sessionsMutex);
        connectionCount = _sessions.size() + 1;
        if (connectionCount <= _maxNumConnections) {
            ssmIt = _sessions.emplace(_sessions.begin(), ssm);
            _currentConnections.store(connectionCount);
            _createdConnections.addAndFetch(1);
        }
    }

    // Checking if we successfully added a connection above. Separated from the lock so we don't log
    // while holding it.
    if (connectionCount > _maxNumConnections) {
        if (!quiet) {
            log() << "connection refused because too many open connections: " << connectionCount;
        }
        return;
    }

    if (!quiet) {
        const auto word = (connectionCount == 1 ? " connection"_sd : " connections"_sd);
        log() << "connection accepted from " << session->remote() << " #" << session->id() << " ("
              << connectionCount << word << " now open)";
    }

    ssm->setCleanupHook([ this, ssmIt, session = std::move(session) ] {
        size_t connectionCount;
        auto remote = session->remote();
        {
            stdx::lock_guard<decltype(_sessionsMutex)> lk(_sessionsMutex);
            _sessions.erase(ssmIt);
            connectionCount = _sessions.size();
            _currentConnections.store(connectionCount);
        }
        _shutdownCondition.notify_one();
        const auto word = (connectionCount == 1 ? " connection"_sd : " connections"_sd);
        log() << "end connection " << remote << " (" << connectionCount << word << " now open)";

    });

    auto ownership = ServiceStateMachine::Ownership::kOwned;
    if (transportMode == transport::Mode::kSynchronous) {
        ownership = ServiceStateMachine::Ownership::kStatic;
    }
    ssm->start(ownership);
}

void ServiceEntryPointImpl::endAllSessions(transport::Session::TagMask tags) {
    // While holding the _sesionsMutex, loop over all the current connections, and if their tags
    // do not match the requested tags to skip, terminate the session.
    {
        stdx::unique_lock<decltype(_sessionsMutex)> lk(_sessionsMutex);
        for (auto& ssm : _sessions) {
            ssm->terminateIfTagsDontMatch(tags);
        }
    }
}

bool ServiceEntryPointImpl::shutdown(Milliseconds timeout) {
    using logger::LogComponent;

    stdx::unique_lock<decltype(_sessionsMutex)> lk(_sessionsMutex);

    // Request that all sessions end, while holding the _sesionsMutex, loop over all the current
    // connections and terminate them
    for (auto& ssm : _sessions) {
        ssm->terminate();
    }

    // Close all sockets and then wait for the number of active connections to reach zero with a
    // condition_variable that notifies in the session cleanup hook. If we haven't closed drained
    // all active operations within the deadline, just keep going with shutdown: the OS will do it
    // for us when the process terminates.
    auto timeSpent = Milliseconds(0);
    const auto checkInterval = std::min(Milliseconds(250), timeout);

    auto noWorkersLeft = [this] { return numOpenSessions() == 0; };
    while (timeSpent < timeout &&
           !_shutdownCondition.wait_for(lk, checkInterval.toSystemDuration(), noWorkersLeft)) {
        log(LogComponent::kNetwork) << "shutdown: still waiting on " << numOpenSessions()
                                    << " active workers to drain... ";
        timeSpent += checkInterval;
    }

    bool result = noWorkersLeft();
    if (result) {
        log(LogComponent::kNetwork) << "shutdown: no running workers found...";
    } else {
        log(LogComponent::kNetwork) << "shutdown: exhausted grace period for" << numOpenSessions()
                                    << " active workers to drain; continuing with shutdown... ";
    }
    return result;
}

ServiceEntryPoint::Stats ServiceEntryPointImpl::sessionStats() const {

    size_t sessionCount = _currentConnections.load();

    ServiceEntryPoint::Stats ret;
    ret.numOpenSessions = sessionCount;
    ret.numCreatedSessions = _createdConnections.load();
    ret.numAvailableSessions = _maxNumConnections - sessionCount;
    return ret;
}

}  // namespace mongo
