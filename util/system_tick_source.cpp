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

#include "mongo/util/system_tick_source.h"

#include "mongo/config.h"

#include <ctime>
#include <limits>
#if defined(MONGO_CONFIG_HAVE_HEADER_UNISTD_H)
#include <unistd.h>
#endif

#include "mongo/base/init.h"
#include "mongo/stdx/memory.h"
#include "mongo/util/assert_util.h"
#include "mongo/util/tick_source.h"
#include "mongo/util/time_support.h"

namespace mongo {

namespace {

const int64_t kMillisPerSecond = 1000;
const int64_t kMicrosPerSecond = 1000 * kMillisPerSecond;
const int64_t kNanosPerSecond = 1000 * kMicrosPerSecond;

/**
 * Internally, the timer counts platform-dependent ticks of some sort, and
 * must then convert those ticks to microseconds and their ilk.  This field
 * stores the frequency of the platform-dependent counter.
 *
 * Define ticksPerSecond before init to ensure correct relative sequencing
 * regardless of how it is initialized (static or dynamic).
 */
TickSource::Tick ticksPerSecond = kMicrosPerSecond;

// "Generic" implementation for _timerNow.
TickSource::Tick _timerNowGeneric() {
    return curTimeMicros64();
}

// Function pointer to timer implementation.
// Overridden in initTickSource() with better implementation where available.
TickSource::Tick (*_timerNow)() = &_timerNowGeneric;

SystemTickSource globalSystemTickSource;

#if defined(_WIN32)

/**
 * Windows-specific implementation of the
 * Timer class.  Windows selects the best available timer, in its estimation, for
 * measuring time at high resolution.  This may be the HPET of the TSC on x86 systems,
 * but is promised to be synchronized across processors, barring BIOS errors.
 */
TickSource::Tick timerNowWindows() {
    LARGE_INTEGER i;
    fassert(16161, QueryPerformanceCounter(&i));
    return i.QuadPart;
}

void initTickSource() {
    LARGE_INTEGER x;
    bool ok = QueryPerformanceFrequency(&x);
    verify(ok);
    ticksPerSecond = x.QuadPart;
    _timerNow = &timerNowWindows;
}

#elif defined(MONGO_CONFIG_HAVE_POSIX_MONOTONIC_CLOCK)

/**
 * Implementation for timer on systems that support the
 * POSIX clock API and CLOCK_MONOTONIC clock.
 */
TickSource::Tick timerNowPosixMonotonicClock() {
    timespec the_time;
    long long result;

    fassert(16160, !clock_gettime(CLOCK_MONOTONIC, &the_time));

    // Safe for 292 years after the clock epoch, even if we switch to a signed time value.
    // On Linux, the monotonic clock's epoch is the UNIX epoch.
    result = static_cast<long long>(the_time.tv_sec);
    result *= kNanosPerSecond;
    result += static_cast<long long>(the_time.tv_nsec);
    return result;
}

void initTickSource() {
    // If the monotonic clock is not available at runtime (sysconf() returns 0 or -1),
    // do not override the generic implementation or modify ticksPerSecond.
    if (sysconf(_SC_MONOTONIC_CLOCK) <= 0) {
        return;
    }

    ticksPerSecond = kNanosPerSecond;
    _timerNow = &timerNowPosixMonotonicClock;

    // Make sure that the current time relative to the (unspecified) epoch isn't already too
    // big to represent as a 64-bit count of nanoseconds.
    long long maxSecs = std::numeric_limits<long long>::max() / kNanosPerSecond;
    timespec the_time;
    fassert(16162, !clock_gettime(CLOCK_MONOTONIC, &the_time));
    fassert(16163, static_cast<long long>(the_time.tv_sec) < maxSecs);
}
#else
void initTickSource() {}
#endif

}  // unnamed namespace

MONGO_INITIALIZER(SystemTickSourceInit)(InitializerContext* context) {
    initTickSource();
    SystemTickSource::get();
    return Status::OK();
}

TickSource::Tick SystemTickSource::getTicks() {
    return _timerNow();
}

TickSource::Tick SystemTickSource::getTicksPerSecond() {
    return ticksPerSecond;
}

SystemTickSource* SystemTickSource::get() {
    static const auto globalSystemTickSource = stdx::make_unique<SystemTickSource>();
    return globalSystemTickSource.get();
}

}  // namespace mongo
