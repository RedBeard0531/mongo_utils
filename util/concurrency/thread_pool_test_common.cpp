/**
 *    Copyright (C) 2015 MongoDB Inc.
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

#define MONGO_LOG_DEFAULT_COMPONENT ::mongo::logger::LogComponent::kDefault

#include "mongo/platform/basic.h"

#include "mongo/util/concurrency/thread_pool_test_common.h"

#include "mongo/stdx/condition_variable.h"
#include "mongo/stdx/memory.h"
#include "mongo/stdx/mutex.h"
#include "mongo/unittest/death_test.h"
#include "mongo/util/assert_util.h"
#include "mongo/util/concurrency/thread_pool_interface.h"
#include "mongo/util/concurrency/thread_pool_test_fixture.h"
#include "mongo/util/log.h"

namespace mongo {
namespace {

using ThreadPoolFactory = stdx::function<std::unique_ptr<ThreadPoolInterface>()>;

class CommonThreadPoolTestFixture : public ThreadPoolTest {
public:
    CommonThreadPoolTestFixture(ThreadPoolFactory makeThreadPool)
        : _makeThreadPool(std::move(makeThreadPool)) {}

private:
    std::unique_ptr<ThreadPoolInterface> makeThreadPool() override {
        return _makeThreadPool();
    }

    ThreadPoolFactory _makeThreadPool;
};

using ThreadPoolTestCaseFactory =
    stdx::function<std::unique_ptr<::mongo::unittest::Test>(ThreadPoolFactory)>;
using ThreadPoolTestCaseMap = stdx::unordered_map<std::string, ThreadPoolTestCaseFactory>;

static ThreadPoolTestCaseMap& threadPoolTestCaseRegistry() {
    static ThreadPoolTestCaseMap registry;
    return registry;
}

class TptRegistrationAgent {
    MONGO_DISALLOW_COPYING(TptRegistrationAgent);

public:
    TptRegistrationAgent(const std::string& name, ThreadPoolTestCaseFactory makeTest) {
        auto& entry = threadPoolTestCaseRegistry()[name];
        if (entry) {
            severe() << "Multiple attempts to register ThreadPoolTest named " << name;
            fassertFailed(34355);
        }
        entry = std::move(makeTest);
    }
};

template <typename T>
class TptDeathRegistrationAgent {
    MONGO_DISALLOW_COPYING(TptDeathRegistrationAgent);

public:
    TptDeathRegistrationAgent(const std::string& name, ThreadPoolTestCaseFactory makeTest) {
        auto& entry = threadPoolTestCaseRegistry()[name];
        if (entry) {
            severe() << "Multiple attempts to register ThreadPoolDeathTest named " << name;
            fassertFailed(34356);
        }
        entry = [makeTest](ThreadPoolFactory makeThreadPool) {
            return stdx::make_unique<::mongo::unittest::DeathTest<T>>(std::move(makeThreadPool));
        };
    }
};

#define COMMON_THREAD_POOL_TEST(TEST_NAME)                                        \
    class TPT_##TEST_NAME : public CommonThreadPoolTestFixture {                  \
    public:                                                                       \
        TPT_##TEST_NAME(ThreadPoolFactory makeThreadPool)                         \
            : CommonThreadPoolTestFixture(std::move(makeThreadPool)) {}           \
                                                                                  \
    private:                                                                      \
        void _doTest() override;                                                  \
        static const TptRegistrationAgent _agent;                                 \
    };                                                                            \
    const TptRegistrationAgent TPT_##TEST_NAME::_agent(                           \
        #TEST_NAME, [](ThreadPoolFactory makeThreadPool) {                        \
            return stdx::make_unique<TPT_##TEST_NAME>(std::move(makeThreadPool)); \
        });                                                                       \
    void TPT_##TEST_NAME::_doTest()

#define COMMON_THREAD_POOL_DEATH_TEST(TEST_NAME, MATCH_EXPR)                      \
    class TPT_##TEST_NAME : public CommonThreadPoolTestFixture {                  \
    public:                                                                       \
        TPT_##TEST_NAME(ThreadPoolFactory makeThreadPool)                         \
            : CommonThreadPoolTestFixture(std::move(makeThreadPool)) {}           \
                                                                                  \
    private:                                                                      \
        void _doTest() override;                                                  \
        static const TptDeathRegistrationAgent<TPT_##TEST_NAME> _agent;           \
    };                                                                            \
    const TptDeathRegistrationAgent<TPT_##TEST_NAME> TPT_##TEST_NAME::_agent(     \
        #TEST_NAME, [](ThreadPoolFactory makeThreadPool) {                        \
            return stdx::make_unique<TPT_##TEST_NAME>(std::move(makeThreadPool)); \
        });                                                                       \
    std::string getDeathTestPattern(TPT_##TEST_NAME*) {                           \
        return MATCH_EXPR;                                                        \
    }                                                                             \
    void TPT_##TEST_NAME::_doTest()

COMMON_THREAD_POOL_TEST(UnusedPool) {
    getThreadPool();
}

COMMON_THREAD_POOL_TEST(CannotScheduleAfterShutdown) {
    auto& pool = getThreadPool();
    pool.shutdown();
    ASSERT_EQ(ErrorCodes::ShutdownInProgress, pool.schedule([] {}));
}

COMMON_THREAD_POOL_DEATH_TEST(DieOnDoubleStartUp, "it has already started") {
    auto& pool = getThreadPool();
    pool.startup();
    pool.startup();
}

COMMON_THREAD_POOL_DEATH_TEST(DieWhenExceptionBubblesUp, "Exception escaped task in") {
    auto& pool = getThreadPool();
    pool.startup();
    ASSERT_OK(pool.schedule([] {
        uassertStatusOK(Status({ErrorCodes::BadValue, "No good very bad exception"}));
    }));
    pool.shutdown();
    pool.join();
}

COMMON_THREAD_POOL_DEATH_TEST(DieOnDoubleJoin, "Attempted to join pool") {
    auto& pool = getThreadPool();
    pool.shutdown();
    pool.join();
    pool.join();
}

COMMON_THREAD_POOL_TEST(PoolDestructorExecutesRemainingTasks) {
    auto& pool = getThreadPool();
    bool executed = false;
    ASSERT_OK(pool.schedule([&executed] { executed = true; }));
    deleteThreadPool();
    ASSERT_EQ(executed, true);
}

COMMON_THREAD_POOL_TEST(PoolJoinExecutesRemainingTasks) {
    auto& pool = getThreadPool();
    bool executed = false;
    ASSERT_OK(pool.schedule([&executed] { executed = true; }));
    pool.shutdown();
    pool.join();
    ASSERT_EQ(executed, true);
}

COMMON_THREAD_POOL_TEST(RepeatedScheduleDoesntSmashStack) {
    const std::size_t depth = 10000ul;
    auto& pool = getThreadPool();
    stdx::function<void()> func;
    std::size_t n = 0;
    stdx::mutex mutex;
    stdx::condition_variable condvar;
    func = [&pool, &n, &func, &condvar, &mutex, depth] {
        stdx::unique_lock<stdx::mutex> lk(mutex);
        if (n < depth) {
            n++;
            lk.unlock();
            ASSERT_OK(pool.schedule(func));
        } else {
            pool.shutdown();
            condvar.notify_one();
        }
    };
    func();
    pool.startup();
    pool.join();

    stdx::unique_lock<stdx::mutex> lk(mutex);
    condvar.wait(lk, [&n, depth] { return n == depth; });
}

}  // namespace

void addTestsForThreadPool(const std::string& suiteName, ThreadPoolFactory makeThreadPool) {
    auto suite = unittest::Suite::getSuite(suiteName);
    for (auto testCase : threadPoolTestCaseRegistry()) {
        suite->add(str::stream() << suiteName << "::" << testCase.first,
                   [testCase, makeThreadPool] { testCase.second(makeThreadPool)->run(); });
    }
}

}  // namespace mongo
