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

#include "mongo/util/tick_source.h"

namespace mongo {

/**
 * Tick source based on platform specific clock ticks. Should be of reasonably high
 * performance. The maximum span measurable by the counter and convertible to microseconds
 * is about 10 trillion ticks. As long as there are fewer than 100 ticks per nanosecond,
 * timer durations of 2.5 years will be supported. Since a typical tick duration will be
 * under 10 per nanosecond, if not below 1 per nanosecond, this should not be an issue.
 */
class SystemTickSource final : public TickSource {
public:
    TickSource::Tick getTicks() override;

    TickSource::Tick getTicksPerSecond() override;

    /**
     * Gets the singleton instance of SystemTickSource. Should not be called before
     * the global initializers are done.
     */
    static SystemTickSource* get();
};
}  // namespace mongo
