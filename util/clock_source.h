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

#pragma once

#include "mongo/stdx/condition_variable.h"
#include "mongo/stdx/functional.h"
#include "mongo/stdx/mutex.h"
#include "mongo/util/time_support.h"

namespace mongo {

class Date_t;

/**
 * An interface for getting the current wall clock time.
 */
class ClockSource {
public:
    virtual ~ClockSource() = default;

    /**
     * Returns the minimum time change that the clock can describe.
     */
    virtual Milliseconds getPrecision() = 0;

    /**
     * Returns the current wall clock time, as defined by this source.
     */
    virtual Date_t now() = 0;

    /**
     * Schedules "action" to run sometime after this clock source reaches "when".
     *
     * Returns InternalError if this clock source does not implement setAlarm. May also
     * return ShutdownInProgress during shutdown. Other errors are also allowed.
     */
    virtual Status setAlarm(Date_t when, stdx::function<void()> action) {
        return {ErrorCodes::InternalError, "This clock source does not implement setAlarm."};
    }

    /**
     * Returns true if this clock source (loosely) tracks the OS clock used for things
     * like condition_variable::wait_until. Virtualized clocks used for testing return
     * false here, and should provide an implementation for setAlarm, above.
     */
    bool tracksSystemClock() const {
        return _tracksSystemClock;
    }

    /**
     * Like cv.wait_until(m, deadline), but uses this ClockSource instead of
     * stdx::chrono::system_clock to measure the passage of time.
     */
    stdx::cv_status waitForConditionUntil(stdx::condition_variable& cv,
                                          stdx::unique_lock<stdx::mutex>& m,
                                          Date_t deadline);

    /**
     * Like cv.wait_until(m, deadline, pred), but uses this ClockSource instead of
     * stdx::chrono::system_clock to measure the passage of time.
     */
    template <typename Pred>
    bool waitForConditionUntil(stdx::condition_variable& cv,
                               stdx::unique_lock<stdx::mutex>& m,
                               Date_t deadline,
                               const Pred& pred) {
        while (!pred()) {
            if (waitForConditionUntil(cv, m, deadline) == stdx::cv_status::timeout) {
                return pred();
            }
        }
        return true;
    }

    /**
     * Like cv.wait_for(m, duration, pred), but uses this ClockSource instead of
     * stdx::chrono::system_clock to measure the passage of time.
     */
    template <typename Duration, typename Pred>
    bool waitForConditionFor(stdx::condition_variable& cv,
                             stdx::unique_lock<stdx::mutex>& m,
                             Duration duration,
                             const Pred& pred) {
        return waitForConditionUntil(cv, m, now() + duration, pred);
    }

protected:
    bool _tracksSystemClock = true;
};

}  // namespace mongo
