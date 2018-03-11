/**
 *    Copyright (C) 2018 MongoDB Inc.
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

#include <benchmark/benchmark.h>

#include "mongo/util/clock_source.h"
#include "mongo/util/fast_clock_source_factory.h"
#include "mongo/util/processinfo.h"
#include "mongo/util/system_clock_source.h"

namespace mongo {
namespace {

/**
 * Benchmark calls to the now() method of a clock source. With an argument of 0,
 * tests the system clock source, and with larger values, uses the FastClockSource
 * with a clock resolution of the specified number of milliseconds.
 *
 * All threads executing the benchmark use the same instance of the clock source,
 * to allow benchmarking to identify synchronization costs inside the now() method.
 */
void BM_ClockNow(benchmark::State& state) {
    static std::unique_ptr<ClockSource> clock;
    if (state.thread_index == 0) {
        if (state.range(0) > 0) {
            clock = FastClockSourceFactory::create(Milliseconds{state.range(0)});
        } else if (state.range(0) == 0) {
            clock = std::make_unique<SystemClockSource>();
        } else {
            state.SkipWithError("poll period must be non-negative");
            return;
        }
    }

    for (auto keepRunning : state) {
        benchmark::DoNotOptimize(clock->now());
    }

    if (state.thread_index == 0) {
        clock.reset();
    }
}

BENCHMARK(BM_ClockNow)
    ->ThreadRange(1,
                  [] {
                      ProcessInfo::initializeSystemInfo();
                      ProcessInfo pi;
                      return static_cast<int>(pi.getNumAvailableCores().value_or(pi.getNumCores()));
                  }())
    ->ArgName("poll period")
    ->Arg(0)
    ->Arg(1)
    ->Arg(10);

}  // namespace
}  // namespace mongo
