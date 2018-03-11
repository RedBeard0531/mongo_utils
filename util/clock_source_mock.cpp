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

#include "mongo/util/clock_source_mock.h"

#include <algorithm>

namespace mongo {

Milliseconds ClockSourceMock::getPrecision() {
    return Milliseconds(1);
}

Date_t ClockSourceMock::now() {
    stdx::lock_guard<stdx::mutex> lk(_mutex);
    return _now;
}

void ClockSourceMock::advance(Milliseconds ms) {
    stdx::unique_lock<stdx::mutex> lk(_mutex);
    _now += ms;
    _processAlarms(std::move(lk));
}

void ClockSourceMock::reset(Date_t newNow) {
    stdx::unique_lock<stdx::mutex> lk(_mutex);
    _now = newNow;
    _processAlarms(std::move(lk));
}

Status ClockSourceMock::setAlarm(Date_t when, stdx::function<void()> action) {
    stdx::unique_lock<stdx::mutex> lk(_mutex);
    if (when <= _now) {
        lk.unlock();
        action();
        return Status::OK();
    }
    _alarms.emplace_back(std::make_pair(when, std::move(action)));
    return Status::OK();
}

void ClockSourceMock::_processAlarms(stdx::unique_lock<stdx::mutex> lk) {
    using std::swap;
    invariant(lk.owns_lock());
    std::vector<Alarm> readyAlarms;
    std::vector<Alarm>::iterator iter;
    auto alarmIsNotExpired = [&](const Alarm& alarm) { return alarm.first > _now; };
    auto expiredAlarmsBegin = std::partition(_alarms.begin(), _alarms.end(), alarmIsNotExpired);
    std::move(expiredAlarmsBegin, _alarms.end(), std::back_inserter(readyAlarms));
    _alarms.erase(expiredAlarmsBegin, _alarms.end());
    lk.unlock();
    for (const auto& alarm : readyAlarms) {
        alarm.second();
    }
}

}  // namespace mongo
