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

#include <asio.hpp>

#include "mongo/executor/async_timer_interface.h"
#include "mongo/platform/atomic_word.h"
#include "mongo/stdx/thread.h"
#include "mongo/util/concurrency/with_lock.h"
#include "mongo/util/periodic_runner.h"

namespace mongo {

class Client;

/**
 * A PeriodicRunner implementation that uses the ASIO library's eventing system
 * to schedule and run jobs at regular intervals.
 *
 * This class takes a timer factory so that it may be mocked out for testing.
 *
 * The runner will set up a background thread per job and allow asio to distribute jobs across those
 * threads. Thus, scheduled jobs cannot block each other from running (a long running job can only
 * block itself). Scheduled jobs that require an operation context should use
 * Client::getCurrent()->makeOperationContext() to create one for themselves, and MUST clear it
 * before they return.
 *
 * The threads running internally will use the thread name "PeriodicRunnerASIO" and
 * anything logged from within a scheduled background task will use this thread name.
 * Scheduled tasks may set the thread name to a custom value as they run. However,
 * if they do this, they MUST set the thread name back to its original value before
 * they return.
 */
class PeriodicRunnerASIO : public PeriodicRunner {
public:
    using PeriodicRunner::PeriodicJob;

    /**
     * Construct a new instance of this class using the provided timer factory.
     */
    explicit PeriodicRunnerASIO(std::unique_ptr<executor::AsyncTimerFactoryInterface> timerFactory);

    ~PeriodicRunnerASIO();

    /**
     * Schedule a job to be run at periodic intervals.
     */
    void scheduleJob(PeriodicJob job) override;

    /**
     * Starts up this periodic runner.
     *
     * This periodic runner will only run once; if it is subsequently started up
     * again, it will return an error.
     */
    Status startup() override;

    /**
     * Shut down this periodic runner. Stops all jobs from running. This method
     * may safely be called multiple times, but only the first call will have any effect.
     */
    void shutdown() override;

private:
    struct PeriodicJobASIO {
        explicit PeriodicJobASIO(PeriodicJob callable,
                                 Date_t startTime,
                                 std::shared_ptr<executor::AsyncTimerInterface> sharedTimer)
            : job(std::move(callable.job)),
              interval(callable.interval),
              start(startTime),
              timer(sharedTimer) {}
        Job job;
        Milliseconds interval;
        Date_t start;
        std::shared_ptr<executor::AsyncTimerInterface> timer;
    };

    // Internally, we will transition through these states
    enum class State { kReady, kRunning, kComplete };

    void _scheduleJob(std::weak_ptr<PeriodicJobASIO> job, bool firstTime);

    void _spawnThreads(WithLock);

    asio::io_service _io_service;
    asio::io_service::strand _strand;

    std::vector<stdx::thread> _threads;

    std::unique_ptr<executor::AsyncTimerFactoryInterface> _timerFactory;

    stdx::mutex _stateMutex;
    State _state;

    std::vector<std::shared_ptr<PeriodicJobASIO>> _jobs;
};

}  // namespace mongo
