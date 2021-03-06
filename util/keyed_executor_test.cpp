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

#include "mongo/util/keyed_executor.h"

#include <random>
#include <string>
#include <vector>

#include "mongo/platform/atomic_word.h"
#include "mongo/stdx/thread.h"
#include "mongo/unittest/unittest.h"
#include "mongo/util/concurrency/thread_pool.h"

namespace mongo {
namespace {

class MockExecutor : public OutOfLineExecutor {
public:
    void schedule(stdx::function<void()> func) override {
        _deque.push_front(std::move(func));
    }

    size_t depth() const {
        return _deque.size();
    }

    bool runOne() {
        if (_deque.empty()) {
            return false;
        }

        auto x = std::move(_deque.back());
        _deque.pop_back();
        x();

        return true;
    }

    void runAll() {
        while (runOne()) {
        }
    }

private:
    std::deque<stdx::function<void()>> _deque;
};

class ThreadPoolExecutor : public OutOfLineExecutor {
public:
    ThreadPoolExecutor() : _threadPool(ThreadPool::Options{}) {}

    void start() {
        _threadPool.startup();
    }

    void shutdown() {
        _threadPool.shutdown();
    }

    void schedule(stdx::function<void()> func) override {
        ASSERT_OK(_threadPool.schedule(std::move(func)));
    }

private:
    ThreadPool _threadPool;
};

TEST(KeyedExecutor, basicExecute) {
    MockExecutor me;
    KeyedExecutor<std::string> ke(&me);

    auto run1 = ke.execute("foo", [] { return 1; });

    ASSERT_EQUALS(me.depth(), 1ul);
    ASSERT_FALSE(run1.isReady());

    auto run2 = ke.execute("foo", [] { return 2; });

    ASSERT_EQUALS(me.depth(), 1ul);
    ASSERT_FALSE(run1.isReady());
    ASSERT_FALSE(run2.isReady());

    ASSERT(me.runOne());
    ASSERT_EQUALS(me.depth(), 1ul);
    ASSERT_EQUALS(run1.get(), 1);
    ASSERT_FALSE(run2.isReady());

    ASSERT(me.runOne());
    ASSERT_EQUALS(me.depth(), 0ul);
    ASSERT_EQUALS(run2.get(), 2);
}

TEST(KeyedExecutor, differentKeysDontConflict) {
    MockExecutor me;
    KeyedExecutor<std::string> ke(&me);

    auto foo = ke.execute("foo", [] { return true; });

    ASSERT_EQUALS(me.depth(), 1ul);
    ASSERT_FALSE(foo.isReady());

    auto bar = ke.execute("bar", [] { return true; });

    ASSERT_EQUALS(me.depth(), 2ul);
    ASSERT_FALSE(foo.isReady());
    ASSERT_FALSE(bar.isReady());

    me.runAll();
    ASSERT_EQUALS(me.depth(), 0ul);
    ASSERT(foo.get());
    ASSERT(bar.get());
}

TEST(KeyedExecutor, onCurrentTasksDrained) {
    MockExecutor me;
    KeyedExecutor<std::string> ke(&me);

    auto run1 = ke.execute("foo", [] { return true; });
    auto bar = ke.execute("bar", [] { return true; });
    auto onBarDone = ke.onCurrentTasksDrained("bar");
    auto onRun1Done = ke.onCurrentTasksDrained("foo");
    auto run2 = ke.execute("foo", [] { return true; });

    ASSERT_EQUALS(me.depth(), 2ul);
    ASSERT_FALSE(run1.isReady());
    ASSERT_FALSE(run2.isReady());
    ASSERT_FALSE(onRun1Done.isReady());
    ASSERT_FALSE(bar.isReady());
    ASSERT_FALSE(onBarDone.isReady());

    auto onRun2Done = ke.onCurrentTasksDrained("foo");

    ASSERT_EQUALS(me.depth(), 2ul);
    ASSERT_FALSE(run1.isReady());
    ASSERT_FALSE(run2.isReady());
    ASSERT_FALSE(onRun1Done.isReady());
    ASSERT_FALSE(onRun2Done.isReady());
    ASSERT_FALSE(bar.isReady());
    ASSERT_FALSE(onBarDone.isReady());

    ASSERT(me.runOne());
    ASSERT_EQUALS(me.depth(), 2ul);
    ASSERT(run1.get());
    ASSERT_OK(onRun1Done.getNoThrow());
    ASSERT_FALSE(run2.isReady());
    ASSERT_FALSE(onRun2Done.isReady());
    ASSERT_FALSE(bar.isReady());
    ASSERT_FALSE(onBarDone.isReady());

    ASSERT(me.runOne());
    ASSERT_EQUALS(me.depth(), 1ul);
    ASSERT(bar.get());
    ASSERT_OK(onBarDone.getNoThrow());

    ASSERT(me.runOne());
    ASSERT_EQUALS(me.depth(), 0ul);
    ASSERT(run2.get());
    ASSERT_OK(onRun2Done.getNoThrow());
}

TEST(KeyedExecutor, onAllCurrentTasksDrained) {
    MockExecutor me;
    KeyedExecutor<std::string> ke(&me);

    auto run1 = ke.execute("foo", [] { return true; });

    ASSERT_EQUALS(me.depth(), 1ul);
    ASSERT_FALSE(run1.isReady());

    auto run2 = ke.execute("bar", [] { return true; });

    ASSERT_EQUALS(me.depth(), 2ul);
    ASSERT_FALSE(run1.isReady());
    ASSERT_FALSE(run2.isReady());

    auto onAllDone = ke.onAllCurrentTasksDrained();

    ASSERT_EQUALS(me.depth(), 2ul);
    ASSERT_FALSE(run1.isReady());
    ASSERT_FALSE(run2.isReady());
    ASSERT_FALSE(onAllDone.isReady());

    auto run3 = ke.execute("foo", [] { return true; });

    ASSERT_EQUALS(me.depth(), 2ul);
    ASSERT_FALSE(run1.isReady());
    ASSERT_FALSE(run2.isReady());
    ASSERT_FALSE(run3.isReady());
    ASSERT_FALSE(onAllDone.isReady());

    ASSERT(me.runOne());
    ASSERT_EQUALS(me.depth(), 2ul);
    ASSERT(run1.get());
    ASSERT_FALSE(run2.isReady());
    ASSERT_FALSE(onAllDone.isReady());

    ASSERT(me.runOne());
    ASSERT_EQUALS(me.depth(), 1ul);
    ASSERT(run2.get());
    ASSERT_OK(onAllDone.getNoThrow());

    ASSERT(me.runOne());
    ASSERT_EQUALS(me.depth(), 0ul);
    ASSERT(run3.get());
}

TEST(KeyedExecutor, onCurrentTasksDrainedEmpty) {
    MockExecutor me;
    KeyedExecutor<std::string> ke(&me);

    ASSERT_OK(ke.onCurrentTasksDrained("foo").getNoThrow());
}

TEST(KeyedExecutor, onAllCurrentTasksDrainedEmpty) {
    MockExecutor me;
    KeyedExecutor<std::string> ke(&me);

    ASSERT_OK(ke.onAllCurrentTasksDrained().getNoThrow());
}

TEST(KeyedExecutor, retriesFailureWithSpecialCode) {
    MockExecutor me;
    KeyedExecutor<std::string> ke(&me);

    int count = 2;

    auto run1 = ke.execute("foo", [&count] {
        if (--count) {
            uasserted(ErrorCodes::KeyedExecutorRetry, "force a retry");
        }

        return true;
    });

    auto run2 = ke.execute("foo", [] { return true; });

    ASSERT_EQUALS(me.depth(), 1ul);
    ASSERT_FALSE(run1.isReady());
    ASSERT_FALSE(run2.isReady());

    ASSERT(me.runOne());
    ASSERT_EQUALS(count, 1);
    ASSERT_EQUALS(me.depth(), 1ul);
    ASSERT_FALSE(run1.isReady());
    ASSERT_FALSE(run2.isReady());

    ASSERT(me.runOne());
    ASSERT_EQUALS(count, 0);
    ASSERT_EQUALS(me.depth(), 1ul);
    ASSERT(run1.get());
    ASSERT_FALSE(run2.isReady());

    ASSERT(me.runOne());
    ASSERT_EQUALS(me.depth(), 0ul);
    ASSERT(run2.get());
}

TEST(KeyedExecutor, doesntRetryFailureWithoutSpecialCode) {
    MockExecutor me;
    KeyedExecutor<std::string> ke(&me);

    auto run1 = ke.execute("foo", [] { uasserted(ErrorCodes::BadValue, "some other code"); });

    auto run2 = ke.execute("foo", [] { return true; });

    ASSERT_EQUALS(me.depth(), 1ul);
    ASSERT_FALSE(run1.isReady());
    ASSERT_FALSE(run2.isReady());

    ASSERT(me.runOne());
    ASSERT_EQUALS(me.depth(), 1ul);
    ASSERT_THROWS_CODE(run1.get(), DBException, ErrorCodes::BadValue);
    ASSERT_FALSE(run2.isReady());

    ASSERT(me.runOne());
    ASSERT_EQUALS(me.depth(), 0ul);
    ASSERT(run2.get());
}

TEST(KeyedExecutor, gracefulShutdown) {
    MockExecutor me;
    KeyedExecutor<std::string> ke(&me);

    Status status = Status::OK();

    auto adaptForInShutdown = [&](auto&& cb) {
        return [&] {
            uassertStatusOK(status);
            cb();
        };
    };

    auto run = ke.execute("foo", adaptForInShutdown([] {}));
    auto onRunDone = ke.onCurrentTasksDrained("foo");
    auto onAllRunDone = ke.onAllCurrentTasksDrained();

    status = Status(ErrorCodes::InterruptedAtShutdown, "shutting down");
    me.runAll();

    ASSERT_THROWS_CODE(run.get(), DBException, ErrorCodes::InterruptedAtShutdown);
    onRunDone.get();
    onAllRunDone.get();
}

TEST(KeyedExecutor, withThreadsTest) {
    ThreadPoolExecutor tpe;
    KeyedExecutor<int> ke(&tpe);
    tpe.start();

    constexpr size_t n = (1 << 16);

    stdx::mutex mutex;
    stdx::condition_variable condvar;
    size_t counter = 0;

    auto incCounter = [&](auto&&) {
        stdx::lock_guard<stdx::mutex> lk(mutex);
        counter++;

        if (counter == n) {
            condvar.notify_one();
        }
    };

    std::mt19937 gen(1);
    std::uniform_int_distribution<> keyDistribution(1, 3);
    std::uniform_int_distribution<> actionDistribution(1, 100);

    for (size_t i = 0; i < n; ++i) {
        auto action = actionDistribution(gen);

        if (action <= 65) {
            ke.execute(keyDistribution(gen), [] {
                  stdx::this_thread::yield();
              }).getAsync(incCounter);
        } else if (action <= 90) {
            ke.onCurrentTasksDrained(keyDistribution(gen)).getAsync(incCounter);
        } else {
            ke.onAllCurrentTasksDrained().getAsync(incCounter);
        }
    }

    stdx::unique_lock<stdx::mutex> lk(mutex);
    condvar.wait(lk, [&] { return counter == n; });

    tpe.shutdown();

    ASSERT_EQUALS(counter, n);
}

}  // namespace
}  // namespace mongo
