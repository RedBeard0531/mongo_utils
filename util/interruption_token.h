/**
 *    Copyright (C) 2018 MongoDB Inc.
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

#include "mongo/base/status.h"
#include "mongo/base/status_with.h"
#include "mongo/stdx/condition_variable.h"
#include "mongo/stdx/mutex.h"
#include "mongo/util/time_support.h"

namespace mongo {

class InterruptionToken {
public:
    /**
     * Waits for either the condition "cv" to be signaled, this operation to be interrupted, or the
     * deadline on this operation to expire.  In the event of interruption or operation deadline
     * expiration, raises a AssertionException with an error code indicating the interruption type.
     */
    void waitForConditionOrInterrupt(stdx::condition_variable& cv,
                                     stdx::unique_lock<stdx::mutex>& m);

    /**
     * Waits on condition "cv" for "pred" until "pred" returns true, or this operation
     * is interrupted or its deadline expires. Throws a DBException for interruption and
     * deadline expiration.
     */
    template <typename Pred>
    void waitForConditionOrInterrupt(stdx::condition_variable& cv,
                                     stdx::unique_lock<stdx::mutex>& m,
                                     Pred pred) {
        while (!pred()) {
            waitForConditionOrInterrupt(cv, m);
        }
    }

    /**
     * Same as waitForConditionOrInterrupt, except returns a Status instead of throwing
     * a DBException to report interruption.
     */
    Status waitForConditionOrInterruptNoAssert(stdx::condition_variable& cv,
                                               stdx::unique_lock<stdx::mutex>& m) noexcept;

    /**
     * Waits for condition "cv" to be signaled, or for the given "deadline" to expire, or
     * for the operation to be interrupted, or for the operation's own deadline to expire.
     *
     * If the operation deadline expires or the operation is interrupted, throws a DBException.  If
     * the given "deadline" expires, returns cv_status::timeout. Otherwise, returns
     * cv_status::no_timeout.
     */
    stdx::cv_status waitForConditionOrInterruptUntil(stdx::condition_variable& cv,
                                                     stdx::unique_lock<stdx::mutex>& m,
                                                     Date_t deadline);

    /**
     * Waits on condition "cv" for "pred" until "pred" returns true, or the given "deadline"
     * expires, or this operation is interrupted, or this operation's own deadline expires.
     *
     *
     * If the operation deadline expires or the operation is interrupted, throws a DBException.  If
     * the given "deadline" expires, returns cv_status::timeout. Otherwise, returns
     * cv_status::no_timeout indicating that "pred" finally returned true.
     */
    template <typename Pred>
    bool waitForConditionOrInterruptUntil(stdx::condition_variable& cv,
                                          stdx::unique_lock<stdx::mutex>& m,
                                          Date_t deadline,
                                          Pred pred) {
        while (!pred()) {
            if (stdx::cv_status::timeout == waitForConditionOrInterruptUntil(cv, m, deadline)) {
                return pred();
            }
        }
        return true;
    }

    /**
     * Same as the predicate form of waitForConditionOrInterruptUntil, but takes a relative
     * amount of time to wait instead of an absolute time point.
     */
    template <typename Pred>
    bool waitForConditionOrInterruptFor(stdx::condition_variable& cv,
                                        stdx::unique_lock<stdx::mutex>& m,
                                        Milliseconds duration,
                                        Pred pred) {
        return waitForConditionOrInterruptUntil(cv, m, nowForInterruption() + duration, pred);
    }

    /**
     * Same as waitForConditionOrInterruptUntil, except returns StatusWith<stdx::cv_status> and
     * non-ok status indicates the error instead of a DBException.
     */
    virtual StatusWith<stdx::cv_status> waitForConditionOrInterruptNoAssertUntil(
        stdx::condition_variable& cv,
        stdx::unique_lock<stdx::mutex>& m,
        Date_t deadline) noexcept = 0;

    /**
     * Returns an interruption token backed by (*this) that will expire no later than deadline.
     *
     * This can be used to temporarily have an earlier deadline than the current interruption token,
     * but it cannot be used to make the deadline later.
     *
     * It is the callers responsibility to ensure that (*this) outlives the returned object.
     */
    auto withDeadline(Date_t maxDeadline) & {
        class WithDeadLine final : public InterruptionToken {
        public:
            WithDeadLine(InterruptionToken* underlying, Date_t maxDeadline)
                : underlying(underlying), maxDeadline(maxDeadline) {}

            StatusWith<stdx::cv_status> waitForConditionOrInterruptNoAssertUntil(
                stdx::condition_variable& cv,
                stdx::unique_lock<stdx::mutex>& m,
                Date_t deadline) noexcept override {
                auto out = underlying->waitForConditionOrInterruptNoAssertUntil(
                    cv, m, std::min(deadline, maxDeadline));
                if (out == stdx::cv_status::timeout && nowForInterruption() >= maxDeadline)
                    return Status(ErrorCodes::ExceededTimeLimit, "exceeded time limit");
                return out;
            }

            Date_t nowForInterruption() override {
                return underlying->nowForInterruption();
            }

            // Public because it is harmless and useful for testing.
            InterruptionToken* const underlying;
            const Date_t maxDeadline;
        };

        return WithDeadLine(this, maxDeadline);
    }

    /**
     * Like withDeadline, but for a timeout duration.
     */
    auto withTimeout(Milliseconds maxTimeout) & {
        return withDeadline(nowForInterruption() + maxTimeout);
    }

protected:
    /**
     * Returns now according to the ServiceContext's precise clock.
     */
    virtual Date_t nowForInterruption() = 0;

    /*Not virtual*/ ~InterruptionToken() = default;
};

/**
 * This is a trivial instance of Interruption token that is never interrupted.
 * The instance is thread-safe, has static storage duration, and is initialized before dynamic init.
 */
extern InterruptionToken* const notInterruptible;

}  // namespace mongo
