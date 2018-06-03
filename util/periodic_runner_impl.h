/**
 * Copyright (C) 2018 MongoDB Inc.
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

#include <memory>
#include <vector>

#include "mongo/stdx/condition_variable.h"
#include "mongo/stdx/mutex.h"
#include "mongo/stdx/thread.h"
#include "mongo/util/clock_source.h"
#include "mongo/util/periodic_runner.h"

namespace mongo {

class Client;
class ServiceContext;

/**
 * An implementation of the PeriodicRunner which uses a thread per job and condvar waits on those
 * threads to independently sleep.
 */
class PeriodicRunnerImpl : public PeriodicRunner {
public:
    PeriodicRunnerImpl(ServiceContext* svc, ClockSource* clockSource);
    ~PeriodicRunnerImpl();

    void scheduleJob(PeriodicJob job) override;

    void startup() override;

    void shutdown() override;

private:
    struct PeriodicJobImpl : public std::enable_shared_from_this<PeriodicJobImpl> {
        PeriodicJobImpl(PeriodicJob job, PeriodicRunnerImpl* parent);

        void run();

        PeriodicJob job;
        PeriodicRunnerImpl* parent;
        stdx::thread thread;
    };

    ServiceContext* _svc;
    ClockSource* _clockSource;

    std::vector<std::shared_ptr<PeriodicJobImpl>> _jobs;

    stdx::mutex _mutex;
    stdx::condition_variable _condvar;
    bool _running = false;
};

}  // namespace mongo
