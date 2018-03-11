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

#define MONGO_LOG_DEFAULT_COMPONENT ::mongo::logger::LogComponent::kControl;

#include "mongo/platform/basic.h"

#include "mongo/util/exit.h"

#include <boost/optional.hpp>
#include <stack>

#include "mongo/stdx/condition_variable.h"
#include "mongo/stdx/functional.h"
#include "mongo/stdx/mutex.h"
#include "mongo/stdx/thread.h"
#include "mongo/util/log.h"
#include "mongo/util/quick_exit.h"

namespace mongo {

namespace {

stdx::mutex shutdownMutex;
stdx::condition_variable shutdownTasksComplete;
boost::optional<ExitCode> shutdownExitCode;
bool shutdownTasksInProgress = false;
AtomicUInt32 shutdownFlag;
std::stack<stdx::function<void()>> shutdownTasks;
stdx::thread::id shutdownTasksThreadId;

void runTasks(decltype(shutdownTasks) tasks) {
    while (!tasks.empty()) {
        const auto& task = tasks.top();
        try {
            task();
        } catch (...) {
            std::terminate();
        }
        tasks.pop();
    }
}

// The logAndQuickExit_inlock() function should be called while holding the 'shutdownMutex' to
// prevent multiple threads from attempting to log that they are exiting. The quickExit() function
// has its own 'quickExitMutex' to prohibit multiple threads from attempting to call _exit().
MONGO_COMPILER_NORETURN void logAndQuickExit_inlock() {
    ExitCode code = shutdownExitCode.get();
    log() << "shutting down with code:" << code;
    quickExit(code);
}

void setShutdownFlag() {
    shutdownFlag.fetchAndAdd(1);
}

}  // namespace

bool globalInShutdownDeprecated() {
    return shutdownFlag.loadRelaxed() != 0;
}

ExitCode waitForShutdown() {
    stdx::unique_lock<stdx::mutex> lk(shutdownMutex);
    shutdownTasksComplete.wait(lk, [] {
        const auto shutdownStarted = static_cast<bool>(shutdownExitCode);
        return shutdownStarted && !shutdownTasksInProgress;
    });

    return shutdownExitCode.get();
}

void registerShutdownTask(stdx::function<void()> task) {
    stdx::lock_guard<stdx::mutex> lock(shutdownMutex);
    invariant(!globalInShutdownDeprecated());
    shutdownTasks.emplace(std::move(task));
}

void shutdown(ExitCode code) {
    decltype(shutdownTasks) localTasks;

    {
        stdx::unique_lock<stdx::mutex> lock(shutdownMutex);

        if (shutdownTasksInProgress) {
            // Someone better have called shutdown in some form already.
            invariant(globalInShutdownDeprecated());

            // Re-entrant calls to shutdown are not allowed.
            invariant(shutdownTasksThreadId != stdx::this_thread::get_id());

            ExitCode originallyRequestedCode = shutdownExitCode.get();
            if (code != originallyRequestedCode) {
                log() << "While running shutdown tasks with the intent to exit with code "
                      << originallyRequestedCode << ", an additional shutdown request arrived with "
                                                    "the intent to exit with a different exit code "
                      << code << "; ignoring the conflicting exit code";
            }

            // Wait for the shutdown tasks to complete
            while (shutdownTasksInProgress)
                shutdownTasksComplete.wait(lock);

            logAndQuickExit_inlock();
        }

        setShutdownFlag();
        shutdownExitCode.emplace(code);
        shutdownTasksInProgress = true;
        shutdownTasksThreadId = stdx::this_thread::get_id();

        localTasks.swap(shutdownTasks);
    }

    runTasks(std::move(localTasks));

    {
        stdx::lock_guard<stdx::mutex> lock(shutdownMutex);
        shutdownTasksInProgress = false;

        shutdownTasksComplete.notify_all();

        logAndQuickExit_inlock();
    }
}

void shutdownNoTerminate() {
    decltype(shutdownTasks) localTasks;

    {
        stdx::lock_guard<stdx::mutex> lock(shutdownMutex);

        if (globalInShutdownDeprecated())
            return;

        setShutdownFlag();
        shutdownTasksInProgress = true;
        shutdownTasksThreadId = stdx::this_thread::get_id();

        localTasks.swap(shutdownTasks);
    }

    runTasks(std::move(localTasks));

    {
        stdx::lock_guard<stdx::mutex> lock(shutdownMutex);
        shutdownTasksInProgress = false;
        shutdownExitCode.emplace(EXIT_CLEAN);
    }

    shutdownTasksComplete.notify_all();
}

}  // namespace mongo
