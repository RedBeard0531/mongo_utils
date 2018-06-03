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

#include "mongo/platform/basic.h"

#include "mongo/util/interruption_token.h"

namespace mongo {

namespace {
class NotInterruptible final : public InterruptionToken {
public:
    StatusWith<stdx::cv_status> waitForConditionOrInterruptNoAssertUntil(
        stdx::condition_variable& cv,
        stdx::unique_lock<stdx::mutex>& m,
        Date_t deadline) noexcept override {
        if (deadline == Date_t::max()) {
            // Wait forever.
            cv.wait(m);
            return stdx::cv_status::no_timeout;
        }
        return cv.wait_until(m, deadline.toSystemTimePoint());
    }

    Date_t nowForInterruption() override {
        return Date_t::now();
    }
};

// This just exists to prove that the real instance doesn't need runtime init.
constexpr NotInterruptible unused;

NotInterruptible notInterruptibleInstance;
}  // namespace

InterruptionToken* const notInterruptible = &notInterruptibleInstance;

void InterruptionToken::waitForConditionOrInterrupt(stdx::condition_variable& cv,
                                                    stdx::unique_lock<stdx::mutex>& m) {
    uassertStatusOK(waitForConditionOrInterruptNoAssert(cv, m));
}


Status InterruptionToken::waitForConditionOrInterruptNoAssert(
    stdx::condition_variable& cv, stdx::unique_lock<stdx::mutex>& m) noexcept {
    auto status = waitForConditionOrInterruptNoAssertUntil(cv, m, Date_t::max());
    if (!status.isOK()) {
        return status.getStatus();
    }
    invariant(status.getValue() == stdx::cv_status::no_timeout);
    return status.getStatus();
}

stdx::cv_status InterruptionToken::waitForConditionOrInterruptUntil(
    stdx::condition_variable& cv, stdx::unique_lock<stdx::mutex>& m, Date_t deadline) {

    return uassertStatusOK(waitForConditionOrInterruptNoAssertUntil(cv, m, deadline));
}

}  // namespace mongo
