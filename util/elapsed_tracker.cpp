// @file elapsed_tracker.cpp

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

#include "mongo/platform/basic.h"

#include "mongo/util/elapsed_tracker.h"

#include "mongo/util/clock_source.h"

namespace mongo {

ElapsedTracker::ElapsedTracker(ClockSource* cs,
                               int32_t hitsBetweenMarks,
                               Milliseconds msBetweenMarks)
    : _clock(cs),
      _hitsBetweenMarks(hitsBetweenMarks),
      _msBetweenMarks(msBetweenMarks),
      _pings(0),
      _last(cs->now()) {}

bool ElapsedTracker::intervalHasElapsed() {
    if (++_pings >= _hitsBetweenMarks) {
        _pings = 0;
        _last = _clock->now();
        return true;
    }

    const auto now = _clock->now();
    if (now - _last > _msBetweenMarks) {
        _pings = 0;
        _last = now;
        return true;
    }

    return false;
}

void ElapsedTracker::resetLastTime() {
    _pings = 0;
    _last = _clock->now();
}

}  // namespace mongo
