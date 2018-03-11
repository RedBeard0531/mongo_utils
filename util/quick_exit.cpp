/**
*    Copyright (C) 2014 MongoDB Inc.
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

// NOTE: This file *must not* depend on any mongo symbols.

#include "mongo/platform/basic.h"

#include "mongo/config.h"

#if defined(MONGO_CONFIG_HAVE_HEADER_UNISTD_H)
#include <unistd.h>
#endif

// This will probably get us _exit on non-unistd platforms like Windows.
#include <cstdlib>

// NOTE: Header only dependencies are OK in this library.
#include "mongo/stdx/mutex.h"

#if !defined(__has_feature)
#define __has_feature(x) 0
#endif

#if !defined(__has_include)
#define __has_include(x) 0
#endif

#if __has_feature(address_sanitizer)

#if __has_include("sanitizer/coverage_interface.h")
// In Clang 3.7+, the coverage interface was split out into its own header file.
#include <sanitizer/coverage_interface.h>
#elif __has_include("sanitizer/common_interface_defs.h")
#include <sanitizer/common_interface_defs.h>
#endif

#include <sanitizer/lsan_interface.h>
#endif

#if defined(MONGO_CPU_PROFILER)
#include <gperftools/profiler.h>
#endif

#ifdef MONGO_GCOV
extern "C" void __gcov_flush();
#endif

namespace mongo {

namespace {
stdx::mutex* const quickExitMutex = new stdx::mutex;
}  // namespace

void quickExit(int code) {
    // Ensure that only one thread invokes the last rites here. No
    // RAII here - we never want to unlock this.
    if (quickExitMutex)
        quickExitMutex->lock();

#if defined(MONGO_CPU_PROFILER)
    ::ProfilerStop();
#endif

#ifdef MONGO_GCOV
    __gcov_flush();
#endif

#if __has_feature(address_sanitizer)
    // Always dump coverage data first because older versions of sanitizers may not write coverage
    // data before exiting with errors. The underlying issue is fixed in clang 3.6, which also
    // prevents coverage data from being written more than once via an atomic guard.
    __sanitizer_cov_dump();
    __lsan_do_leak_check();
#endif

#if defined(_WIN32)
    // SERVER-23860: VS 2015 Debug Builds abort and Release builds AV when _exit is called on
    // multiple threads. Each call to _exit shuts down the CRT, and so subsequent calls into the
    // CRT result in undefined behavior. Bypass _exit CRT shutdown code and call TerminateProcess
    // directly instead to match GLibc's _exit which calls the syscall exit_group.
    ::TerminateProcess(GetCurrentProcess(), code);
#else
    ::_exit(code);
#endif
}

}  // namespace mongo
