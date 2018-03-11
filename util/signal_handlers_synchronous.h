/**
 *    Copyright (C) 2010 10gen Inc.
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

namespace mongo {

/**
 * Sets up handlers for synchronous events, like segv, abort, terminate and malloc-failure.
 *
 * Call this very early in main(), before runGlobalInitializers().
 *
 * Called by setupSignalHandlers() in signal_handlers.h.  Prefer that method to this one,
 * in server code and tools that use the storage engine.
 */
void setupSynchronousSignalHandlers();

/**
 * Report out of memory error with a stack trace and exit.
 *
 * Called when any of the following functions fails to allocate memory:
 *     operator new
 *     mongoMalloc
 *     mongoRealloc
 */
void reportOutOfMemoryErrorAndExit();

/**
 * Clears the signal mask for the process. This is called from forkServer and to setup
 * the unit tests. On Windows, this is a noop.
 */
void clearSignalMask();

}  // namespace mongo
