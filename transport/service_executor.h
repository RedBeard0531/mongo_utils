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

#include "mongo/base/status.h"
#include "mongo/bson/bsonobjbuilder.h"
#include "mongo/platform/bitwise_enum_operators.h"
#include "mongo/stdx/functional.h"
#include "mongo/transport/service_executor_task_names.h"
#include "mongo/transport/transport_mode.h"
#include "mongo/util/duration.h"

namespace mongo {
// This needs to be forward declared here because the service_context.h is a circular dependency.
class ServiceContext;

namespace transport {

/*
 * This is the interface for all ServiceExecutors.
 */
class ServiceExecutor {
public:
    virtual ~ServiceExecutor() = default;
    using Task = stdx::function<void()>;
    enum ScheduleFlags {
        // No flags (kEmptyFlags) specifies that this is a normal task and that the executor should
        // launch new threads as needed to run the task.
        kEmptyFlags = 1 << 0,

        // Deferred tasks will never get a new thread launched to run them.
        kDeferredTask = 1 << 1,

        // MayRecurse indicates that a task may be run recursively.
        kMayRecurse = 1 << 2,

        // MayYieldBeforeSchedule indicates that the executor may yield on the current thread before
        // scheduling the task.
        kMayYieldBeforeSchedule = 1 << 3,
    };

    /*
     * Starts the ServiceExecutor. This may create threads even if no tasks are scheduled.
     */
    virtual Status start() = 0;

    /*
     * Schedules a task with the ServiceExecutor and returns immediately.
     *
     * This is guaranteed to unwind the stack before running the task, although the task may be
     * run later in the same thread.
     *
     * If defer is true, then the executor may defer execution of this Task until an available
     * thread is available.
     */
    virtual Status schedule(Task task, ScheduleFlags flags, ServiceExecutorTaskName taskName) = 0;

    /*
     * Stops and joins the ServiceExecutor. Any outstanding tasks will not be executed, and any
     * associated callbacks waiting on I/O may get called with an error code.
     *
     * This should only be called during server shutdown to gracefully destroy the ServiceExecutor
     */
    virtual Status shutdown(Milliseconds timeout) = 0;

    /*
     * Returns if this service executor is using asynchronous or synchronous networking.
     */
    virtual Mode transportMode() const = 0;

    /*
     * Appends statistics about task scheduling to a BSONObjBuilder for serverStatus output.
     */
    virtual void appendStats(BSONObjBuilder* bob) const = 0;
};

}  // namespace transport

ENABLE_BITMASK_OPERATORS(transport::ServiceExecutor::ScheduleFlags)

}  // namespace mongo
