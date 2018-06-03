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

#include "mongo/platform/basic.h"

#include "mongo/util/periodic_runner_impl.h"

#include "mongo/db/client.h"
#include "mongo/db/service_context.h"
#include "mongo/util/clock_source.h"
#include "mongo/util/scopeguard.h"

namespace mongo {

PeriodicRunnerImpl::PeriodicRunnerImpl(ServiceContext* svc, ClockSource* clockSource)
    : _svc(svc), _clockSource(clockSource) {}

PeriodicRunnerImpl::~PeriodicRunnerImpl() {
    shutdown();
}

void PeriodicRunnerImpl::scheduleJob(PeriodicJob job) {
    auto impl = std::make_shared<PeriodicJobImpl>(std::move(job), this);

    {
        stdx::unique_lock<stdx::mutex> lk(_mutex);
        _jobs.push_back(impl);
        if (_running) {
            impl->run();
        }
    }
}

void PeriodicRunnerImpl::startup() {
    stdx::lock_guard<stdx::mutex> lk(_mutex);

    if (_running) {
        return;
    }

    _running = true;

    // schedule any jobs that we have
    for (auto job : _jobs) {
        job->run();
    }
}

void PeriodicRunnerImpl::shutdown() {
    std::vector<stdx::thread> threads;
    const auto guard = MakeGuard([&] {
        for (auto& thread : threads) {
            thread.join();
        }
    });

    {
        stdx::lock_guard<stdx::mutex> lk(_mutex);
        if (_running) {
            _running = false;

            for (auto&& job : _jobs) {
                threads.push_back(std::move(job->thread));
            }

            _jobs.clear();

            _condvar.notify_all();
        }
    }
}

PeriodicRunnerImpl::PeriodicJobImpl::PeriodicJobImpl(PeriodicJob job, PeriodicRunnerImpl* parent)
    : job(std::move(job)), parent(parent) {}

void PeriodicRunnerImpl::PeriodicJobImpl::run() {
    thread = stdx::thread([ this, anchor = shared_from_this() ] {
        Client::initThread(job.name, parent->_svc, nullptr);

        while (true) {
            auto start = parent->_clockSource->now();

            job.job(Client::getCurrent());

            stdx::unique_lock<stdx::mutex> lk(parent->_mutex);
            if (parent->_clockSource->waitForConditionUntil(
                    parent->_condvar, lk, start + job.interval, [&] {
                        return !parent->_running;
                    })) {
                break;
            }
        }
    });
}

}  // namespace mongo
