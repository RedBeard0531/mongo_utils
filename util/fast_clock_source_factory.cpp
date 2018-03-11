/**
 *    Copyright (C) 2016 MongoDB Inc.
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

#include "mongo/util/fast_clock_source_factory.h"

#include <memory>

#include "mongo/stdx/memory.h"
#include "mongo/util/background_thread_clock_source.h"
#include "mongo/util/system_clock_source.h"

namespace mongo {

std::unique_ptr<ClockSource> FastClockSourceFactory::create(Milliseconds granularity) {
    // TODO: Create the fastest to read wall clock available on the system.
    // For now, assume there is no built-in fast wall clock so instead
    // create a background-thread-based timer.
    return stdx::make_unique<BackgroundThreadClockSource>(stdx::make_unique<SystemClockSource>(),
                                                          granularity);
}

}  // namespace mongo
