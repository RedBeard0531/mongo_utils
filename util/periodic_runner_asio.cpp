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

#define MONGO_LOG_DEFAULT_COMPONENT ::mongo::logger::LogComponent::kControl

#include "mongo/platform/basic.h"

#include <algorithm>
#include <memory>

#include "mongo/util/periodic_runner_asio.h"

#include "mongo/db/client.h"
#include "mongo/util/log.h"

namespace mongo {

PeriodicRunnerASIO::PeriodicRunnerASIO(
    std::unique_ptr<executor::AsyncTimerFactoryInterface> timerFactory)
    : _io_service(),
      _strand(_io_service),
      _timerFactory(std::move(timerFactory)),
      _state(State::kReady) {}

PeriodicRunnerASIO::~PeriodicRunnerASIO() {
    // We must call shutdown here to join our background thread.
    shutdown();
}

void PeriodicRunnerASIO::scheduleJob(PeriodicJob job) {
    // The interval we use here will get written over by _scheduleJob_inlock.
    auto uniqueTimer = _timerFactory->make(&_strand, Milliseconds{0});
    std::shared_ptr<executor::AsyncTimerInterface> timer{std::move(uniqueTimer)};

    auto asioJob = std::make_shared<PeriodicJobASIO>(std::move(job), _timerFactory->now(), timer);

    {
        stdx::unique_lock<stdx::mutex> lk(_stateMutex);
        _jobs.insert(_jobs.end(), asioJob);
        if (_state == State::kRunning) {
            _scheduleJob(asioJob, true);
            _spawnThreads(lk);
        }
    }
}

void PeriodicRunnerASIO::_scheduleJob(std::weak_ptr<PeriodicJobASIO> job, bool firstTime) {
    auto lockedJob = job.lock();
    if (!lockedJob) {
        return;
    }

    // Adjust the timer to expire at the correct time.
    auto adjustedMS =
        std::max(Milliseconds(0), lockedJob->start + lockedJob->interval - _timerFactory->now());
    lockedJob->timer->expireAfter(adjustedMS);
    lockedJob->timer->asyncWait([this, job, firstTime](std::error_code ec) mutable {
        if (!firstTime) {
            if (ec) {
                severe() << "Encountered an error in PeriodicRunnerASIO: " << ec.message();
                return;
            }

            auto lockedJob = job.lock();
            if (!lockedJob) {
                return;
            }

            lockedJob->start = _timerFactory->now();

            lockedJob->job(Client::getCurrent());
        }

        _io_service.post([this, job]() mutable { _scheduleJob(job, false); });
    });
}

void PeriodicRunnerASIO::_spawnThreads(WithLock) {
    while (_threads.size() < _jobs.size()) {
        _threads.emplace_back([this] {
            try {
                auto client = getGlobalServiceContext()->makeClient("PeriodicRunnerASIO");
                Client::setCurrent(std::move(client));

                asio::io_service::work workItem(_io_service);
                std::error_code ec;
                _io_service.run(ec);

                client = Client::releaseCurrent();

                if (ec) {
                    severe() << "Failure in PeriodicRunnerASIO: " << ec.message();
                    fassertFailed(40438);
                }
            } catch (...) {
                severe() << "Uncaught exception in PeriodicRunnerASIO: " << exceptionToStatus();
                fassertFailed(40439);
            }
        });
    }
}

Status PeriodicRunnerASIO::startup() {
    stdx::unique_lock<stdx::mutex> lk(_stateMutex);
    if (_state != State::kReady) {
        return {ErrorCodes::ShutdownInProgress, "startup() already called"};
    }

    _state = State::kRunning;

    // schedule any jobs that we have
    for (auto& job : _jobs) {
        job->start = _timerFactory->now();
        _scheduleJob(job, true);
    }

    _spawnThreads(lk);

    return Status::OK();
}

void PeriodicRunnerASIO::shutdown() {
    stdx::unique_lock<stdx::mutex> lk(_stateMutex);
    if (_state == State::kRunning) {
        _state = State::kComplete;

        _io_service.stop();
        _jobs.clear();

        lk.unlock();

        for (auto& thread : _threads) {
            thread.join();
        }
    }
}

}  // namespace mongo
