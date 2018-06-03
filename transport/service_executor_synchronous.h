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

#include <deque>

#include "mongo/base/status.h"
#include "mongo/platform/atomic_word.h"
#include "mongo/stdx/condition_variable.h"
#include "mongo/stdx/mutex.h"
#include "mongo/transport/service_executor.h"
#include "mongo/transport/service_executor_task_names.h"

namespace mongo {
namespace transport {

/**
 * The passthrough service executor emulates a thread per connection.
 * Each connection has its own worker thread where jobs get scheduled.
 */
class ServiceExecutorSynchronous final : public ServiceExecutor {
public:
    explicit ServiceExecutorSynchronous(ServiceContext* ctx);

    Status start() override;
    Status shutdown(Milliseconds timeout) override;
    Status schedule(Task task, ScheduleFlags flags, ServiceExecutorTaskName taskName) override;

    Mode transportMode() const override {
        return Mode::kSynchronous;
    }

    void appendStats(BSONObjBuilder* bob) const override;

private:
    static thread_local std::deque<Task> _localWorkQueue;
    static thread_local int _localRecursionDepth;
    static thread_local int64_t _localThreadIdleCounter;

    AtomicBool _stillRunning{false};

    mutable stdx::mutex _shutdownMutex;
    stdx::condition_variable _shutdownCondition;

    AtomicWord<size_t> _numRunningWorkerThreads{0};
    size_t _numHardwareCores{0};
};

}  // namespace transport
}  // namespace mongo
