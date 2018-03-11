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

#include <cmath>
#include <csignal>
#include <cstdlib>

#include "mongo/unittest/death_test.h"
#include "mongo/unittest/unittest.h"
#include "mongo/util/signal_handlers_synchronous.h"

namespace {
using namespace mongo;

// Tests of signals that should be ignored raise each signal twice, to ensure that the handler isn't
// reset.
#define IGNORED_SIGNAL(SIGNUM)           \
    TEST(IgnoredSignalTest, SIGNUM##_) { \
        ASSERT_EQ(0, raise(SIGNUM));     \
        ASSERT_EQ(0, raise(SIGNUM));     \
    }

#define FATAL_SIGNAL(SIGNUM)                                                                   \
    DEATH_TEST(FatalSignalTest, SIGNUM##_, str::stream() << "Got signal: " << SIGNUM << " ") { \
        ASSERT_EQ(0, raise(SIGNUM));                                                           \
    }

IGNORED_SIGNAL(SIGUSR2)
IGNORED_SIGNAL(SIGHUP)
IGNORED_SIGNAL(SIGPIPE)
FATAL_SIGNAL(SIGQUIT)
FATAL_SIGNAL(SIGILL)

#if not defined(__has_feature)
#define __has_feature(X) 0
#endif

#if !__has_feature(address_sanitizer)
// These signals trip the leak sanitizer
FATAL_SIGNAL(SIGABRT)
FATAL_SIGNAL(SIGSEGV)
FATAL_SIGNAL(SIGBUS)
FATAL_SIGNAL(SIGFPE)
#endif

DEATH_TEST(FatalTerminateTest,
           TerminateIsFatalWithoutException,
           "terminate() called. No exception") {
    std::terminate();
}

DEATH_TEST(FatalTerminateTest,
           TerminateIsFatalWithDBException,
           " terminate() called. An exception is active") {
    try {
        uasserted(28720, "Fatal DBException occurrence");
    } catch (...) {
        std::terminate();
    }
}

DEATH_TEST(FatalTerminateTest,
           TerminateIsFatalWithDoubleException,
           "terminate() called. An exception is active") {
    class ThrowInDestructor {
    public:
        ~ThrowInDestructor() {
            uasserted(28721, "Fatal second exception");
        }
    } tid;

    // This is a workaround for https://gcc.gnu.org/bugzilla/show_bug.cgi?id=83400. We should delete
    // this variable once we are on a compiler that doesn't require it.
    volatile bool workaroundGCCBug = true;  // NOLINT
    if (workaroundGCCBug)
        uasserted(28719, "Non-fatal first exception");
}

}  // namespace
