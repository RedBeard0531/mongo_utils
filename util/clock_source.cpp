/**
 *    Copyright (C) 2017 MongoDB Inc.
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

#include "mongo/stdx/thread.h"
#include "mongo/util/clock_source.h"

namespace mongo {
stdx::cv_status ClockSource::waitForConditionUntil(stdx::condition_variable& cv,
                                                   stdx::unique_lock<stdx::mutex>& m,
                                                   Date_t deadline) {
    if (_tracksSystemClock) {
        return cv.wait_until(m, deadline.toSystemTimePoint());
    }

    // The rest of this function only runs during testing, when the clock source is virtualized and
    // does not track the system clock.

    if (deadline <= now()) {
        return stdx::cv_status::timeout;
    }

    struct AlarmInfo {
        stdx::mutex controlMutex;
        stdx::mutex* waitMutex;
        stdx::condition_variable* waitCV;
        stdx::cv_status cvWaitResult = stdx::cv_status::no_timeout;
    };
    auto alarmInfo = std::make_shared<AlarmInfo>();
    alarmInfo->waitCV = &cv;
    alarmInfo->waitMutex = m.mutex();
    const auto waiterThreadId = stdx::this_thread::get_id();
    bool invokedAlarmInline = false;
    invariant(setAlarm(deadline, [alarmInfo, waiterThreadId, &invokedAlarmInline] {
        stdx::lock_guard<stdx::mutex> controlLk(alarmInfo->controlMutex);
        alarmInfo->cvWaitResult = stdx::cv_status::timeout;
        if (!alarmInfo->waitMutex) {
            return;
        }
        if (stdx::this_thread::get_id() == waiterThreadId) {
            // In NetworkInterfaceMock, setAlarm may invoke its callback immediately if the deadline
            // has expired, so we detect that case and avoid self-deadlock by returning early, here.
            // It is safe to set invokedAlarmInline without synchronization in this case, because it
            // is exactly the case where the same thread is writing and consulting the value.
            invokedAlarmInline = true;
            return;
        }
        stdx::lock_guard<stdx::mutex> waitLk(*alarmInfo->waitMutex);
        alarmInfo->waitCV->notify_all();
    }));
    if (!invokedAlarmInline) {
        cv.wait(m);
    }
    m.unlock();
    stdx::lock_guard<stdx::mutex> controlLk(alarmInfo->controlMutex);
    m.lock();
    alarmInfo->waitMutex = nullptr;
    alarmInfo->waitCV = nullptr;
    return alarmInfo->cvWaitResult;
}
}  // namespace mongo
