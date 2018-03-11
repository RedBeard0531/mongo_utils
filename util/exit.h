/**
* Copyright (C) 2014 MongoDB Inc.
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

#include "mongo/platform/compiler.h"
#include "mongo/stdx/functional.h"
#include "mongo/util/exit_code.h"

namespace mongo {

/**
 * Determines if the shutdown flag is set.
 *
 * Calling this function is deprecated because modules that consult it
 * cannot engage in an orderly, coordinated shutdown. Instead, such
 * modules tend to just stop working at some point after mongo::shutdown() is
 * invoked, without regard to whether modules that depend on them have
 * already shut down.
 */
bool globalInShutdownDeprecated();

/**
 * Does not return until all shutdown tasks have run.
 */
ExitCode waitForShutdown();

/**
 * Registers a new shutdown task to be called when shutdown or
 * shutdownNoTerminate is called. If this function is invoked after
 * shutdown or shutdownNoTerminate has been called, std::terminate is
 * called.
 */
void registerShutdownTask(stdx::function<void()>);

/**
 * Toggles the shutdown flag to 'true', runs registered shutdown
 * tasks, and then exits with the given code. It is safe to call this
 * function from multiple threads, only the first caller executes
 * shutdown tasks. It is illegal to reenter this function from a
 * registered shutdown task. The function does not return.
 */
MONGO_COMPILER_NORETURN void shutdown(ExitCode code);

/**
 * Toggles the shutdown flag to 'true' and runs the registered
 * shutdown tasks. It is safe to call this function from multiple
 * threads, only the first caller executes shutdown tasks, subsequent
 * callers return immediately. It is legal to call shutdownNoTerminate
 * from a shutdown task.
 */
void shutdownNoTerminate();

/** An alias for 'shutdown'. */
MONGO_COMPILER_NORETURN inline void exitCleanly(ExitCode code) {
    shutdown(code);
}

}  // namespace mongo
