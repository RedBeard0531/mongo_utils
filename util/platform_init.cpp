/**
*    Copyright (C) 2012 10gen Inc.
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

#define MONGO_LOG_DEFAULT_COMPONENT ::mongo::logger::LogComponent::kControl

#include "mongo/platform/basic.h"

#ifdef _WIN32
#include <crtdbg.h>
#include <mmsystem.h>
#include <stdio.h>
#include <stdlib.h>
#endif

#include "mongo/base/init.h"
#include "mongo/util/log.h"
#include "mongo/util/stacktrace.h"

#ifdef _WIN32

namespace mongo {

MONGO_INITIALIZER(Behaviors_Win32)(InitializerContext*) {
    // do not display dialog on abort()
    _set_abort_behavior(0, _CALL_REPORTFAULT | _WRITE_ABORT_MSG);

    // hook the C runtime's error display
    _CrtSetReportHook(crtDebugCallback);

    if (_setmaxstdio(2048) == -1) {
        warning() << "Failed to increase max open files limit from default of 512 to 2048";
    }

    // Let's try to set minimum Windows Kernel quantum length to smallest viable timer resolution in
    // order to allow sleepmillis() to support waiting periods below Windows default quantum length
    // (which can vary per Windows version)
    // See https://msdn.microsoft.com/en-us/library/windows/desktop/dd743626(v=vs.85).aspx
    TIMECAPS tc;
    int targetResolution = 1;
    int timerResolution;

    if (timeGetDevCaps(&tc, sizeof(TIMECAPS)) != TIMERR_NOERROR) {
        warning() << "Failed to read timer resolution range.";
        if (timeBeginPeriod(1) != TIMERR_NOERROR) {
            warning() << "Failed to set minimum timer resolution to 1 millisecond.";
        }
    } else {
        timerResolution =
            std::min(std::max(int(tc.wPeriodMin), targetResolution), int(tc.wPeriodMax));
        invariant(timeBeginPeriod(timerResolution) == TIMERR_NOERROR);
    }

    return Status::OK();
}

}  // namespace mongo

#endif  // _WIN32
