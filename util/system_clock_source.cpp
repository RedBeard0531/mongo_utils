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

#include "mongo/util/system_clock_source.h"

#include "mongo/stdx/memory.h"
#include "mongo/util/time_support.h"

namespace mongo {

Date_t SystemClockSource::now() {
    return Date_t::now();
}

SystemClockSource* SystemClockSource::get() {
    static const auto globalSystemClockSource = stdx::make_unique<SystemClockSource>();
    return globalSystemClockSource.get();
}

Milliseconds SystemClockSource::getPrecision() {
    return Milliseconds(1);
}

}  // namespace mongo
