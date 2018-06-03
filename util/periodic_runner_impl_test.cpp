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

#include "mongo/unittest/unittest.h"

#include "mongo/util/periodic_runner_impl.h"

#include "mongo/db/service_context_noop.h"
#include "mongo/stdx/condition_variable.h"
#include "mongo/stdx/mutex.h"
#include "mongo/util/clock_source_mock.h"

namespace mongo {

class Client;

namespace {

class PeriodicRunnerImplTestNoSetup : public unittest::Test {
public:
    void setUp() override {
        _clockSource = std::make_unique<ClockSourceMock>();
        _svc = stdx::make_unique<ServiceContextNoop>();
        _runner = stdx::make_unique<PeriodicRunnerImpl>(_svc.get(), _clockSource.get());
    }

    void tearDown() override {
        _runner->shutdown();
    }

    ClockSourceMock& clockSource() {
        return *_clockSource;
    }

    PeriodicRunner& runner() {
        return *_runner;
    }

private:
    std::unique_ptr<ServiceContext> _svc;
    std::unique_ptr<ClockSourceMock> _clockSource;
    std::unique_ptr<PeriodicRunner> _runner;
};

class PeriodicRunnerImplTest : public PeriodicRunnerImplTestNoSetup {
public:
    void setUp() override {
        PeriodicRunnerImplTestNoSetup::setUp();
        runner().startup();
    }
};

TEST_F(PeriodicRunnerImplTest, OneJobTest) {
    int count = 0;
    Milliseconds interval{5};

    stdx::mutex mutex;
    stdx::condition_variable cv;

    // Add a job, ensure that it runs once
    PeriodicRunner::PeriodicJob job("job",
                                    [&count, &mutex, &cv](Client*) {
                                        {
                                            stdx::unique_lock<stdx::mutex> lk(mutex);
                                            count++;
                                        }
                                        cv.notify_all();
                                    },
                                    interval);

    runner().scheduleJob(std::move(job));

    // Fast forward ten times, we should run all ten times.
    for (int i = 0; i < 10; i++) {
        clockSource().advance(interval);
        {
            stdx::unique_lock<stdx::mutex> lk(mutex);
            cv.wait(lk, [&count, &i] { return count > i; });
        }
    }
}

TEST_F(PeriodicRunnerImplTestNoSetup, ScheduleBeforeStartupTest) {
    int count = 0;
    Milliseconds interval{5};

    stdx::mutex mutex;
    stdx::condition_variable cv;

    // Schedule a job before startup
    PeriodicRunner::PeriodicJob job("job",
                                    [&count, &mutex, &cv](Client*) {
                                        {
                                            stdx::unique_lock<stdx::mutex> lk(mutex);
                                            count++;
                                        }
                                        cv.notify_all();
                                    },
                                    interval);

    runner().scheduleJob(std::move(job));

    // Start the runner, job should still run
    runner().startup();

    clockSource().advance(interval);

    stdx::unique_lock<stdx::mutex> lk(mutex);
    cv.wait(lk, [&count] { return count > 0; });
}

TEST_F(PeriodicRunnerImplTest, TwoJobsTest) {
    int countA = 0;
    int countB = 0;
    Milliseconds intervalA{5};
    Milliseconds intervalB{10};

    stdx::mutex mutex;
    stdx::condition_variable cv;

    // Add two jobs, ensure they both run the proper number of times
    PeriodicRunner::PeriodicJob jobA("job",
                                     [&countA, &mutex, &cv](Client*) {
                                         {
                                             stdx::unique_lock<stdx::mutex> lk(mutex);
                                             countA++;
                                         }
                                         cv.notify_all();
                                     },
                                     intervalA);

    PeriodicRunner::PeriodicJob jobB("job",
                                     [&countB, &mutex, &cv](Client*) {
                                         {
                                             stdx::unique_lock<stdx::mutex> lk(mutex);
                                             countB++;
                                         }
                                         cv.notify_all();
                                     },
                                     intervalB);

    runner().scheduleJob(std::move(jobA));
    runner().scheduleJob(std::move(jobB));

    // Fast forward and wait for both jobs to run the right number of times
    for (int i = 0; i <= 10; i++) {
        clockSource().advance(intervalA);
        {
            stdx::unique_lock<stdx::mutex> lk(mutex);
            cv.wait(lk, [&countA, &countB, &i] { return (countA > i && countB >= i / 2); });
        }
    }
}

TEST_F(PeriodicRunnerImplTest, TwoJobsDontDeadlock) {
    stdx::mutex mutex;
    stdx::condition_variable cv;
    stdx::condition_variable doneCv;
    bool a = false;
    bool b = false;

    PeriodicRunner::PeriodicJob jobA("job",
                                     [&](Client*) {
                                         stdx::unique_lock<stdx::mutex> lk(mutex);
                                         a = true;

                                         cv.notify_one();
                                         cv.wait(lk, [&] { return b; });
                                         doneCv.notify_one();
                                     },
                                     Milliseconds(1));

    PeriodicRunner::PeriodicJob jobB("job",
                                     [&](Client*) {
                                         stdx::unique_lock<stdx::mutex> lk(mutex);
                                         b = true;

                                         cv.notify_one();
                                         cv.wait(lk, [&] { return a; });
                                         doneCv.notify_one();
                                     },
                                     Milliseconds(1));

    runner().scheduleJob(std::move(jobA));
    runner().scheduleJob(std::move(jobB));

    clockSource().advance(Milliseconds(1));

    {
        stdx::unique_lock<stdx::mutex> lk(mutex);
        doneCv.wait(lk, [&] { return a && b; });

        ASSERT(a);
        ASSERT(b);
    }

    tearDown();
}

}  // namespace
}  // namespace mongo
