// @file elapsed_tracker.h

/**
 *    Copyright (C) 2009 10gen Inc.
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

#include <cstdint>

#include "mongo/util/time_support.h"

namespace mongo {

class ClockSource;

/** Keep track of elapsed time. After a set amount of time, tells you to do something. */
class ElapsedTracker {
public:
    ElapsedTracker(ClockSource* cs, int32_t hitsBetweenMarks, Milliseconds msBetweenMarks);

    /**
     * Call this for every iteration.
     * @return true if one of the triggers has gone off.
     */
    bool intervalHasElapsed();

    void resetLastTime();

private:
    ClockSource* const _clock;
    const int32_t _hitsBetweenMarks;
    const Milliseconds _msBetweenMarks;

    int32_t _pings;

    Date_t _last;
};

}  // namespace mongo
