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

#include "mongo/platform/basic.h"

#include "mongo/platform/stack_locator.h"
#include "mongo/stdx/thread.h"
#include "mongo/unittest/unittest.h"

namespace mongo {
namespace {

TEST(StackLocator, StacLocatorFindsStackOfTestExecutorThread) {
    const StackLocator locator;

    const auto begin = locator.begin();
    ASSERT_TRUE(nullptr != begin);

    const auto end = locator.end();
    ASSERT_TRUE(nullptr != end);

    ASSERT_TRUE(begin != end);

    const auto available = locator.available();
    ASSERT_TRUE(available);
    ASSERT_TRUE(available.get() > 0);

    const auto size = locator.size();
    ASSERT_TRUE(size);
    ASSERT_TRUE(size.get() > 0);
    ASSERT_TRUE(size.get() > available.get());
}

TEST(StackLocator, StacksGrowsDown) {
    // The current implementation assumes a downward growing stack. Write a test
    // that confirms that the current platform is downward growing, so that if the
    // system is ever ported to an upward growing stack, we get a test failure.

    const StackLocator locator;
    ASSERT_TRUE(nullptr != locator.begin());
    ASSERT_TRUE(nullptr != locator.end());

    // NOTE: Technically, comparing pointers for ordering is UB if
    // they aren't in the same aggregate, but we are already out
    // with the dragons at the edge of the map.
    ASSERT_TRUE(locator.begin() > locator.end());
}

TEST(StackLocator, StackLocatorFindsStackOfStdThread) {
    bool foundBounds = false;

    stdx::thread thr([&] {
        const StackLocator locator;
        auto avail = locator.available();
        foundBounds = static_cast<bool>(avail);
    });
    thr.join();
    ASSERT_TRUE(foundBounds);
}

struct LocatorThreadHelper {
#ifdef _WIN32
    static DWORD WINAPI run(LPVOID arg) {
        static_cast<LocatorThreadHelper*>(arg)->_run();
        return 0;
    }
#else
    static void* run(void* arg) {
        static_cast<LocatorThreadHelper*>(arg)->_run();
        return nullptr;
    }
#endif

    void _run() {
        const StackLocator locator;
        located = static_cast<bool>(locator.available());
        if (located)
            size = locator.size().get();
    }

    bool located = false;
    size_t size = 0;
};

TEST(StackLocator, StackLocatorFindsStackOfNativeThreadWithDefaultStack) {
    LocatorThreadHelper helper;

#ifdef _WIN32

    HANDLE thread = CreateThread(nullptr, 0, &LocatorThreadHelper::run, &helper, 0, nullptr);
    ASSERT_NE(WAIT_FAILED, WaitForSingleObject(thread, INFINITE));

#else

    pthread_attr_t attrs;
    ASSERT_EQ(0, pthread_attr_init(&attrs));
    pthread_t thread;
    ASSERT_EQ(0, pthread_create(&thread, &attrs, &LocatorThreadHelper::run, &helper));
    ASSERT_EQ(0, pthread_join(thread, nullptr));

#endif

    ASSERT_TRUE(helper.located);
}

TEST(StackLocator, StackLocatorFindStackOfNativeThreadWithCustomStack) {
    const size_t kThreadStackSize = 64 * 1024 * 1024;

#ifdef _WIN32

    LocatorThreadHelper helperNoCommit;
    HANDLE thread = CreateThread(nullptr,
                                 kThreadStackSize,
                                 &LocatorThreadHelper::run,
                                 &helperNoCommit,
                                 STACK_SIZE_PARAM_IS_A_RESERVATION,
                                 nullptr);
    ASSERT_NE(WAIT_FAILED, WaitForSingleObject(thread, INFINITE));
    ASSERT_TRUE(helperNoCommit.located);
    ASSERT_EQ(kThreadStackSize, helperNoCommit.size);

    LocatorThreadHelper helperCommit;
    thread = CreateThread(
        nullptr, kThreadStackSize, &LocatorThreadHelper::run, &helperCommit, 0, nullptr);
    ASSERT_NE(WAIT_FAILED, WaitForSingleObject(thread, INFINITE));
    ASSERT_TRUE(helperCommit.located);
    ASSERT_TRUE(kThreadStackSize <= helperCommit.size);

#else

    LocatorThreadHelper helper;
    pthread_attr_t attrs;
    ASSERT_EQ(0, pthread_attr_init(&attrs));
    ASSERT_EQ(0, pthread_attr_setstacksize(&attrs, kThreadStackSize));
    pthread_t thread;
    ASSERT_EQ(0, pthread_create(&thread, &attrs, &LocatorThreadHelper::run, &helper));
    ASSERT_EQ(0, pthread_join(thread, nullptr));
    ASSERT_TRUE(helper.located);
    ASSERT_TRUE(kThreadStackSize <= helper.size);

#endif
}

}  // namespace
}  // namespace mongo
