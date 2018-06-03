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

#define MONGO_LOG_DEFAULT_COMPONENT ::mongo::logger::LogComponent::kExecutor;

#include "mongo/platform/basic.h"

#include "mongo/transport/service_executor_synchronous.h"

#include "mongo/db/server_parameters.h"
#include "mongo/stdx/thread.h"
#include "mongo/transport/service_entry_point_utils.h"
#include "mongo/transport/service_executor_task_names.h"
#include "mongo/transport/thread_idle_callback.h"
#include "mongo/util/log.h"
#include "mongo/util/processinfo.h"

namespace mongo {
namespace transport {
namespace {

// Tasks scheduled with MayRecurse may be called recursively if the recursion depth is below this
// value.
MONGO_EXPORT_SERVER_PARAMETER(synchronousServiceExecutorRecursionLimit, int, 8);

constexpr auto kThreadsRunning = "threadsRunning"_sd;
constexpr auto kExecutorLabel = "executor"_sd;
constexpr auto kExecutorName = "passthrough"_sd;
}  // namespace

thread_local std::deque<ServiceExecutor::Task> ServiceExecutorSynchronous::_localWorkQueue = {};
thread_local int ServiceExecutorSynchronous::_localRecursionDepth = 0;
thread_local int64_t ServiceExecutorSynchronous::_localThreadIdleCounter = 0;

ServiceExecutorSynchronous::ServiceExecutorSynchronous(ServiceContext* ctx) {}

Status ServiceExecutorSynchronous::start() {
    _numHardwareCores = static_cast<size_t>(ProcessInfo::getNumAvailableCores());

    _stillRunning.store(true);

    return Status::OK();
}

Status ServiceExecutorSynchronous::shutdown(Milliseconds timeout) {
    LOG(3) << "Shutting down passthrough executor";

    _stillRunning.store(false);

    stdx::unique_lock<stdx::mutex> lock(_shutdownMutex);
    bool result = _shutdownCondition.wait_for(lock, timeout.toSystemDuration(), [this]() {
        return _numRunningWorkerThreads.load() == 0;
    });

    return result
        ? Status::OK()
        : Status(ErrorCodes::Error::ExceededTimeLimit,
                 "passthrough executor couldn't shutdown all worker threads within time limit.");
}

Status ServiceExecutorSynchronous::schedule(Task task,
                                            ScheduleFlags flags,
                                            ServiceExecutorTaskName taskName) {
    if (!_stillRunning.load()) {
        return Status{ErrorCodes::ShutdownInProgress, "Executor is not running"};
    }

    if (!_localWorkQueue.empty()) {
        /*
         * In perf testing we found that yielding after running a each request produced
         * at 5% performance boost in microbenchmarks if the number of worker threads
         * was greater than the number of available cores.
         */
        if (flags & ScheduleFlags::kMayYieldBeforeSchedule) {
            if ((_localThreadIdleCounter++ & 0xf) == 0) {
                markThreadIdle();
            }
            if (_numRunningWorkerThreads.loadRelaxed() > _numHardwareCores) {
                stdx::this_thread::yield();
            }
        }

        // Execute task directly (recurse) if allowed by the caller as it produced better
        // performance in testing. Try to limit the amount of recursion so we don't blow up the
        // stack, even though this shouldn't happen with this executor that uses blocking network
        // I/O.
        if ((flags & ScheduleFlags::kMayRecurse) &&
            (_localRecursionDepth < synchronousServiceExecutorRecursionLimit.loadRelaxed())) {
            ++_localRecursionDepth;
            task();
        } else {
            _localWorkQueue.emplace_back(std::move(task));
        }
        return Status::OK();
    }

    // First call to schedule() for this connection, spawn a worker thread that will push jobs
    // into the thread local job queue.
    LOG(3) << "Starting new executor thread in passthrough mode";

    Status status = launchServiceWorkerThread([ this, task = std::move(task) ] {
        _numRunningWorkerThreads.addAndFetch(1);

        _localWorkQueue.emplace_back(std::move(task));
        while (!_localWorkQueue.empty() && _stillRunning.loadRelaxed()) {
            _localRecursionDepth = 1;
            _localWorkQueue.front()();
            _localWorkQueue.pop_front();
        }

        if (_numRunningWorkerThreads.subtractAndFetch(1) == 0) {
            _shutdownCondition.notify_all();
        }
    });

    return status;
}

void ServiceExecutorSynchronous::appendStats(BSONObjBuilder* bob) const {
    BSONObjBuilder section(bob->subobjStart("serviceExecutorTaskStats"));
    section << kExecutorLabel << kExecutorName << kThreadsRunning
            << static_cast<int>(_numRunningWorkerThreads.loadRelaxed());
}

}  // namespace transport
}  // namespace mongo
