// signal_handlers.h

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

enum class LogFileStatus {
    kNeedToRotateLogFile,
    kNoLogFileToRotate,
};

/**
 * Sets up handlers for signals and other events like terminate and new_handler.
 *
 * This must be called very early in main, before runGlobalInitializers().
 */
void setupSignalHandlers();

/**
 * Starts the thread to handle asynchronous signals.
 *
 * This must be the first thread started from the main thread. Call this immediately after
 * initializeServerGlobalState().
 */
void startSignalProcessingThread(LogFileStatus rotate = LogFileStatus::kNeedToRotateLogFile);

/*
 * Uninstall the Control-C handler
 *
 * Windows Only
 * Used by nt services to remove the Control-C handler after the system knows it is running
 * as a service, and not as a console program.
 */
void removeControlCHandler();

}  // namespace mongo
