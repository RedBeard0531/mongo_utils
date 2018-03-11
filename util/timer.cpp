// @file mongo/util/timer.cpp

/*    Copyright 2009 10gen Inc.
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

#include "mongo/util/timer.h"

#include "mongo/util/system_tick_source.h"
#include "mongo/util/tick_source.h"

namespace mongo {

namespace {

const int64_t kMicrosPerSecond = 1000 * 1000;

}  // unnamed namespace

Timer::Timer() : Timer(SystemTickSource::get()) {}

Timer::Timer(TickSource* tickSource)
    : _tickSource(tickSource),
      _microsPerCount(static_cast<double>(kMicrosPerSecond) / _tickSource->getTicksPerSecond()) {
    reset();
}

long long Timer::now() const {
    return _tickSource->getTicks();
}

}  // namespace mongo
