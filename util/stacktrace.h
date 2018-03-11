/*    Copyright 2009 10gen Inc.
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

/**
 * Tools for working with in-process stack traces.
 */

#pragma once

#include <iosfwd>

#if defined(_WIN32)
// We need to pick up a decl for CONTEXT. Forward declaring would be preferable, but it is
// unclear that we can do so.
#include "mongo/platform/windows_basic.h"
#endif

namespace mongo {

// Print stack trace information to "os", default to the log stream.
void printStackTrace(std::ostream& os);
void printStackTrace();

#if defined(_WIN32)
// Print stack trace (using a specified stack context) to "os", default to the log stream.
void printWindowsStackTrace(CONTEXT& context, std::ostream& os);
void printWindowsStackTrace(CONTEXT& context);

// Print error message from C runtime followed by stack trace
int crtDebugCallback(int, char* originalMessage, int*);
#endif

}  // namespace mongo
