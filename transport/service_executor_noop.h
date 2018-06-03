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
#include "mongo/transport/service_executor.h"
#include "mongo/transport/service_executor_task_names.h"

namespace mongo {
namespace transport {

/**
 * The noop service executor provides the necessary interface for some unittests. Doesn't actually
 * execute any work
 */
class ServiceExecutorNoop final : public ServiceExecutor {
public:
    explicit ServiceExecutorNoop(ServiceContext* ctx) {}

    Status start() override {
        return Status::OK();
    }
    Status shutdown(Milliseconds timeout) override {
        return Status::OK();
    }
    Status schedule(Task task, ScheduleFlags flags, ServiceExecutorTaskName taskName) override {
        return Status::OK();
    }

    Mode transportMode() const override {
        return Mode::kSynchronous;
    }

    void appendStats(BSONObjBuilder* bob) const override {}
};

}  // namespace transport
}  // namespace mongo
