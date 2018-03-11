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

#define MONGO_LOG_DEFAULT_COMPONENT ::mongo::logger::LogComponent::kControl

#include "mongo/platform/basic.h"

#include "mongo/util/background_thread_clock_source.h"

#include <chrono>
#include <thread>

#include "mongo/stdx/memory.h"
#include "mongo/stdx/thread.h"
#include "mongo/util/concurrency/idle_thread_block.h"
#include "mongo/util/log.h"
#include "mongo/util/time_support.h"

namespace mongo {

BackgroundThreadClockSource::BackgroundThreadClockSource(std::unique_ptr<ClockSource> clockSource,
                                                         Milliseconds granularity)
    : _clockSource(std::move(clockSource)), _granularity(granularity) {
    _startTimerThread();
    _tracksSystemClock = _clockSource->tracksSystemClock();
}

BackgroundThreadClockSource::~BackgroundThreadClockSource() {
    {
        stdx::unique_lock<stdx::mutex> lock(_mutex);
        _inShutdown = true;
        _condition.notify_one();
    }

    _timer.join();
}

Milliseconds BackgroundThreadClockSource::getPrecision() {
    return _granularity;
}

Status BackgroundThreadClockSource::setAlarm(Date_t when, stdx::function<void()> action) {
    return _clockSource->setAlarm(when, action);
}

Date_t BackgroundThreadClockSource::now() {
    // Since this is called very frequently by many threads, the common case should not write to
    // shared memory.
    if (MONGO_unlikely(_timerWillPause.load())) {
        return _slowNow();
    }
    auto now = _current.load();
    if (MONGO_unlikely(!now)) {
        return _slowNow();
    }
    return Date_t::fromMillisSinceEpoch(now);
}

// This will be called at most once per _granularity per thread. In common cases it will only be
// called by a single thread per _granularity.
Date_t BackgroundThreadClockSource::_slowNow() {
    _timerWillPause.store(false);
    auto now = _current.load();
    if (!now) {
        stdx::lock_guard<stdx::mutex> lock(_mutex);
        // Reload and check after locking since someone else may have done this for us.
        now = _current.load();
        if (!now) {
            // Wake up timer but have it pause if no else calls now() for the next _granularity.
            _condition.notify_one();
            _timerWillPause.store(true);

            now = _updateCurrent_inlock();
        }
    }
    return Date_t::fromMillisSinceEpoch(now);
}

void BackgroundThreadClockSource::_startTimerThread() {
    // Start the background thread that repeatedly sleeps for the specified duration of milliseconds
    // and wakes up to store the current time.
    _timer = stdx::thread([&]() {
        setThreadName("BackgroundThreadClockSource");
        stdx::unique_lock<stdx::mutex> lock(_mutex);
        _started = true;
        _condition.notify_one();

        while (!_inShutdown) {
            if (!_timerWillPause.swap(true)) {
                _updateCurrent_inlock();
            } else {
                // Stop running if nothing has read the time since we last updated the time.
                _current.store(0);
                MONGO_IDLE_THREAD_BLOCK;
                _condition.wait(lock, [this] { return _inShutdown || _current.load() != 0; });
            }

            const auto sleepUntil = Date_t::fromMillisSinceEpoch(_current.load()) + _granularity;
            MONGO_IDLE_THREAD_BLOCK;
            _clockSource->waitForConditionUntil(
                _condition, lock, sleepUntil, [this] { return _inShutdown; });
        }
    });

    // Wait for the thread to start. This prevents other threads from calling now() until the timer
    // thread is at its first wait() call. While the code would work without this, it makes startup
    // more predictable and therefore easier to test.
    stdx::unique_lock<stdx::mutex> lock(_mutex);
    _condition.wait(lock, [this] { return _started; });
}

int64_t BackgroundThreadClockSource::_updateCurrent_inlock() {
    auto now = _clockSource->now().toMillisSinceEpoch();
    if (!now) {
        // We use 0 to indicate that the thread isn't running.
        severe() << "ClockSource " << demangleName(typeid(*_clockSource)) << " reported time 0."
                 << " Is it 1970?";
        fassertFailed(40399);
    }
    _current.store(now);
    return now;
}

}  // namespace mongo
