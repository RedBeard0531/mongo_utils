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

#include <memory>
#include <utility>
#include <vector>

#include "mongo/stdx/mutex.h"
#include "mongo/util/clock_source.h"
#include "mongo/util/time_support.h"

namespace mongo {

/**
 * Mock clock source that returns a fixed time until explicitly advanced.
 */
class ClockSourceMock final : public ClockSource {
public:
    /**
     * Constructs a ClockSourceMock with the current time set to the Unix epoch.
     */
    ClockSourceMock() {
        _tracksSystemClock = false;
    }

    Milliseconds getPrecision() override;
    Date_t now() override;
    Status setAlarm(Date_t when, stdx::function<void()> action) override;

    /**
     * Advances the current time by the given value.
     */
    void advance(Milliseconds ms);

    /**
     * Resets the current time to the given value.
     */
    void reset(Date_t newNow);

private:
    using Alarm = std::pair<Date_t, stdx::function<void()>>;
    void _processAlarms(stdx::unique_lock<stdx::mutex> lk);

    stdx::mutex _mutex;
    Date_t _now{Date_t::fromMillisSinceEpoch(1)};
    std::vector<Alarm> _alarms;
};

class SharedClockSourceAdapter final : public ClockSource {
public:
    explicit SharedClockSourceAdapter(std::shared_ptr<ClockSource> source)
        : _source(std::move(source)) {
        _tracksSystemClock = _source->tracksSystemClock();
    }

    Milliseconds getPrecision() override {
        return _source->getPrecision();
    }

    Date_t now() override {
        return _source->now();
    }

    Status setAlarm(Date_t when, stdx::function<void()> action) override {
        return _source->setAlarm(when, std::move(action));
    }

private:
    std::shared_ptr<ClockSource> _source;
};

}  // namespace mongo
