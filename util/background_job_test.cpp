/*    Copyright 2013 10gen Inc.
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

#include "mongo/platform/atomic_word.h"
#include "mongo/stdx/mutex.h"
#include "mongo/stdx/thread.h"
#include "mongo/unittest/unittest.h"
#include "mongo/util/background.h"
#include "mongo/util/concurrency/notification.h"
#include "mongo/util/time_support.h"

namespace mongo {
namespace {

class TestJob final : public BackgroundJob {
public:
    TestJob(bool selfDelete,
            AtomicWord<bool>* flag,
            Notification<void>* canProceed = nullptr,
            Notification<void>* destructorInvoked = nullptr)
        : BackgroundJob(selfDelete),
          _flag(flag),
          _canProceed(canProceed),
          _destructorInvoked(destructorInvoked) {}

    ~TestJob() override {
        if (_destructorInvoked)
            _destructorInvoked->set();
    }

    std::string name() const override {
        return "TestJob";
    }

    void run() override {
        if (_canProceed)
            _canProceed->get();
        _flag->store(true);
    }

private:
    AtomicWord<bool>* const _flag;
    Notification<void>* const _canProceed;
    Notification<void>* const _destructorInvoked;
};

TEST(BackgroundJobBasic, NormalCase) {
    AtomicWord<bool> flag(false);
    TestJob tj(false, &flag);
    tj.go();
    ASSERT(tj.wait());
    ASSERT_EQUALS(true, flag.load());
}

TEST(BackgroundJobBasic, TimeOutCase) {
    AtomicWord<bool> flag(false);
    Notification<void> canProceed;
    TestJob tj(false, &flag, &canProceed);
    tj.go();

    ASSERT(!tj.wait(1000));
    ASSERT_EQUALS(false, flag.load());

    canProceed.set();
    ASSERT(tj.wait());
    ASSERT_EQUALS(true, flag.load());
}

TEST(BackgroundJobBasic, SelfDeletingCase) {
    AtomicWord<bool> flag(false);
    Notification<void> destructorInvoked;
    // Though it looks like one, this is not a leak since the job is self deleting.
    (new TestJob(true, &flag, nullptr, &destructorInvoked))->go();
    destructorInvoked.get();
    ASSERT_EQUALS(true, flag.load());
}

TEST(BackgroundJobLifeCycle, Go) {
    class Job : public BackgroundJob {
    public:
        Job() : _hasRun(false) {}

        virtual std::string name() const {
            return "BackgroundLifeCycle::CannotCallGoAgain";
        }

        virtual void run() {
            {
                stdx::lock_guard<stdx::mutex> lock(_mutex);
                ASSERT_FALSE(_hasRun);
                _hasRun = true;
            }

            _n.get();
        }

        void notify() {
            _n.set();
        }

    private:
        stdx::mutex _mutex;
        bool _hasRun;
        Notification<void> _n;
    };

    Job j;

    // This call starts Job running.
    j.go();

    // Calling 'go' again while it is running is an error.
    ASSERT_THROWS(j.go(), AssertionException);

    // Stop the Job
    j.notify();
    j.wait();

    // Calling 'go' on a done task is a no-op. If it were not,
    // we would fail the assert in Job::run above.
    j.go();
}

}  // namespace
}  // namespace mongo
