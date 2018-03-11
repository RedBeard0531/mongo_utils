// debug_util.cpp

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

#include "mongo/platform/basic.h"

#include "mongo/util/debugger.h"

#include <cstdlib>
#include <mutex>

#if defined(USE_GDBSERVER)
#include <cstdio>
#include <unistd.h>
#endif

#ifndef _WIN32
#include <signal.h>
#endif

#include "mongo/util/debug_util.h"

#ifndef _WIN32
namespace {
std::once_flag breakpointOnceFlag;
}  // namespace
#endif

namespace mongo {
void breakpoint() {
#ifdef _WIN32
    if (IsDebuggerPresent()) {
        DebugBreak();
    };
#endif
#ifndef _WIN32
    // code to raise a breakpoint in GDB
    std::call_once(breakpointOnceFlag, []() {
        // prevent SIGTRAP from crashing the program if default action is specified and we are not
        // in gdb
        struct sigaction current;
        if (sigaction(SIGTRAP, nullptr, &current) != 0) {
            std::abort();
        }
        if (current.sa_handler == SIG_DFL) {
            signal(SIGTRAP, SIG_IGN);
        }
    });

    raise(SIGTRAP);
#endif
}

#if defined(USE_GDBSERVER)

/* Magic gdb trampoline
 * Do not call directly! call setupSIGTRAPforGDB()
 * Assumptions:
 *  1) gdbserver is on your path
 *  2) You have run "handle SIGSTOP noprint" in gdb
 *  3) serverGlobalParams.port + 2000 is free
 */
void launchGDB(int) {
    // Don't come back here
    signal(SIGTRAP, SIG_IGN);

    char pidToDebug[16];
    int pidRet = snprintf(pidToDebug, sizeof(pidToDebug), "%d", getpid());
    if (!(pidRet >= 0 && size_t(pidRet) < sizeof(pidToDebug)))
        std::abort();

    char msg[128];
    int msgRet = snprintf(
        msg, sizeof(msg), "\n\n\t**** Launching gdbserver (use lsof to find port) ****\n\n");
    if (!(msgRet >= 0 && size_t(msgRet) < sizeof(msg)))
        std::abort();

    if (!(write(STDERR_FILENO, msg, msgRet) == msgRet))
        std::abort();

    if (fork() == 0) {
        // child
        execlp("gdbserver", "gdbserver", "--attach", ":0", pidToDebug, nullptr);
        perror(nullptr);
        _exit(1);
    } else {
        // parent
        raise(SIGSTOP);  // pause all threads until gdb connects and continues
        raise(SIGTRAP);  // break inside gdbserver
    }
}

void setupSIGTRAPforGDB() {
    if (!(signal(SIGTRAP, launchGDB) != SIG_ERR))
        std::abort();
}
#else
void setupSIGTRAPforGDB() {}
#endif
}
