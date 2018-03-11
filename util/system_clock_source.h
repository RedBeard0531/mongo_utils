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

#include "mongo/util/clock_source.h"
#include "mongo/util/time_support.h"

namespace mongo {

/**
 * Clock source based on the system clock.
 */
class SystemClockSource final : public ClockSource {
public:
    Date_t now() final;

    /**
     * Returns the singleton instance of SystemClockSource.
     */
    static SystemClockSource* get();
    Milliseconds getPrecision() override;
};

}  // namespace mongo
