/*    Copyright 2015 MongoDB Inc.
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

// stacktrace_${TARGET_OS_FAMILY}.cpp sets default log component to kControl.
// Setting kDefault to preserve previous behavior in (defunct) getStacktraceLogger().
#define MONGO_LOG_DEFAULT_COMPONENT ::mongo::logger::LogComponent::kDefault

#include "mongo/platform/basic.h"

#include "mongo/util/stacktrace.h"

#include "mongo/util/log.h"

namespace mongo {

void printStackTrace() {
    // NOTE: We disable long-line truncation for the stack trace, because the JSON representation of
    // the stack trace can sometimes exceed the long line limit.
    printStackTrace(log().setIsTruncatable(false).stream());
}

#if defined(_WIN32)

void printWindowsStackTrace(CONTEXT& context) {
    printWindowsStackTrace(context, log().stream());
}

#endif  // defined(_WIN32)

}  // namespace mongo
