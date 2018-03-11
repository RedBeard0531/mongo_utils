/**
 *    Copyright (C) 2016 MongoDB Inc.
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

#include <chrono>
#include <thread>

#include "mongo/base/disallow_copying.h"
#include "mongo/platform/atomic_word.h"
#include "mongo/stdx/condition_variable.h"
#include "mongo/stdx/memory.h"
#include "mongo/stdx/mutex.h"
#include "mongo/stdx/thread.h"
#include "mongo/util/clock_source.h"
#include "mongo/util/time_support.h"

namespace mongo {

/**
 * A clock source that uses a periodic timer to build a low-resolution, fast-to-read clock.
 * Essentially uses a background thread that repeatedly sleeps for X amount of milliseconds
 * and wakes up to store the current time. If nothing reads the time for a whole granularity, the
 * thread will sleep until it is needed again.
 */
class BackgroundThreadClockSource final : public ClockSource {
    MONGO_DISALLOW_COPYING(BackgroundThreadClockSource);

public:
    BackgroundThreadClockSource(std::unique_ptr<ClockSource> clockSource, Milliseconds granularity);
    ~BackgroundThreadClockSource() override;
    Milliseconds getPrecision() override;
    Date_t now() override;
    Status setAlarm(Date_t when, stdx::function<void()> action) override;

    /**
     * Doesn't count as a call to now() for determining whether this ClockSource is idle.
     *
     * Unlike now(), returns Date_t() if the thread is currently paused.
     */
    Date_t peekNowForTest() const {
        return Date_t::fromMillisSinceEpoch(_current.load());
    }

private:
    Date_t _slowNow();
    void _startTimerThread();
    int64_t _updateCurrent_inlock();

    const std::unique_ptr<ClockSource> _clockSource;
    AtomicInt64 _current{0};           // 0 if _timer is paused due to idleness.
    AtomicBool _timerWillPause{true};  // If true when _timer wakes up, it will pause.

    const Milliseconds _granularity;

    stdx::mutex _mutex;
    stdx::condition_variable _condition;
    bool _inShutdown = false;
    bool _started = false;
    stdx::thread _timer;
};

}  // namespace mongo
