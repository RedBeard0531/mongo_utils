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

#define MONGO_LOG_DEFAULT_COMPONENT ::mongo::logger::LogComponent::kControl

#include "mongo/platform/basic.h"

#include "mongo/util/signal_handlers.h"

#include <signal.h>
#include <time.h>

#if !defined(_WIN32)
#include <unistd.h>
#endif

#include "mongo/db/log_process_details.h"
#include "mongo/db/server_options.h"
#include "mongo/db/service_context.h"
#include "mongo/platform/process_id.h"
#include "mongo/stdx/thread.h"
#include "mongo/util/assert_util.h"
#include "mongo/util/concurrency/idle_thread_block.h"
#include "mongo/util/exit.h"
#include "mongo/util/log.h"
#include "mongo/util/quick_exit.h"
#include "mongo/util/scopeguard.h"
#include "mongo/util/signal_handlers_synchronous.h"
#include "mongo/util/signal_win32.h"

#if defined(_WIN32)
namespace {
const char* strsignal(int signalNum) {
    // should only see SIGABRT on windows
    switch (signalNum) {
        case SIGABRT:
            return "SIGABRT";
        default:
            return "UNKNOWN";
    }
}
}
#endif

namespace mongo {

using std::endl;

/*
 * WARNING: PLEASE READ BEFORE CHANGING THIS MODULE
 *
 * All code in this module must be signal-friendly. Before adding any system
 * call or other dependency, please make sure that this still holds.
 *
 * All code in this file follows this pattern:
 *   Generic code
 *   #ifdef _WIN32
 *       Windows code
 *   #else
 *       Posix code
 *   #endif
 *
 */

namespace {

#ifdef _WIN32
void consoleTerminate(const char* controlCodeName) {
    setThreadName("consoleTerminate");
    log() << "got " << controlCodeName << ", will terminate after current cmd ends";
    exitCleanly(EXIT_KILL);
}

BOOL WINAPI CtrlHandler(DWORD fdwCtrlType) {
    switch (fdwCtrlType) {
        case CTRL_C_EVENT:
            log() << "Ctrl-C signal";
            consoleTerminate("CTRL_C_EVENT");
            return TRUE;

        case CTRL_CLOSE_EVENT:
            log() << "CTRL_CLOSE_EVENT signal";
            consoleTerminate("CTRL_CLOSE_EVENT");
            return TRUE;

        case CTRL_BREAK_EVENT:
            log() << "CTRL_BREAK_EVENT signal";
            consoleTerminate("CTRL_BREAK_EVENT");
            return TRUE;

        case CTRL_LOGOFF_EVENT:
            // only sent to services, and only in pre-Vista Windows; FALSE means ignore
            return FALSE;

        case CTRL_SHUTDOWN_EVENT:
            log() << "CTRL_SHUTDOWN_EVENT signal";
            consoleTerminate("CTRL_SHUTDOWN_EVENT");
            return TRUE;

        default:
            return FALSE;
    }
}

void eventProcessingThread() {
    std::string eventName = getShutdownSignalName(ProcessId::getCurrent().asUInt32());

    HANDLE event = CreateEventA(NULL, TRUE, FALSE, eventName.c_str());
    if (event == NULL) {
        warning() << "eventProcessingThread CreateEvent failed: " << errnoWithDescription();
        return;
    }

    ON_BLOCK_EXIT(CloseHandle, event);

    int returnCode = WaitForSingleObject(event, INFINITE);
    if (returnCode != WAIT_OBJECT_0) {
        if (returnCode == WAIT_FAILED) {
            warning() << "eventProcessingThread WaitForSingleObject failed: "
                      << errnoWithDescription();
            return;
        } else {
            warning() << "eventProcessingThread WaitForSingleObject failed: "
                      << errnoWithDescription(returnCode);
            return;
        }
    }

    setThreadName("eventTerminate");

    log() << "shutdown event signaled, will terminate after current cmd ends";
    exitCleanly(EXIT_CLEAN);
}

#else

// The signals in asyncSignals will be processed by this thread only, in order to
// ensure the db and log mutexes aren't held. Because this is run in a different thread, it does
// not need to be safe to call in signal context.
sigset_t asyncSignals;
void signalProcessingThread(LogFileStatus rotate) {
    setThreadName("signalProcessingThread");

    time_t signalTimeSeconds = -1;
    time_t lastSignalTimeSeconds = -1;

    while (true) {
        int actualSignal = 0;
        int status = [&] {
            MONGO_IDLE_THREAD_BLOCK;
            return sigwait(&asyncSignals, &actualSignal);
        }();
        fassert(16781, status == 0);
        switch (actualSignal) {
            case SIGUSR1:
                // log rotate signal
                signalTimeSeconds = time(0);
                if (signalTimeSeconds <= lastSignalTimeSeconds) {
                    // ignore multiple signals in the same or earlier second.
                    break;
                }

                lastSignalTimeSeconds = signalTimeSeconds;
                fassert(16782, rotateLogs(serverGlobalParams.logRenameOnRotate));
                if (rotate == LogFileStatus::kNeedToRotateLogFile) {
                    logProcessDetailsForLogRotate(getGlobalServiceContext());
                }
                break;
            default:
                // interrupt/terminate signal
                log() << "got signal " << actualSignal << " (" << strsignal(actualSignal)
                      << "), will terminate after current cmd ends" << endl;
                exitCleanly(EXIT_CLEAN);
                break;
        }
    }
}
#endif
}  // namespace

void setupSignalHandlers() {
    setupSynchronousSignalHandlers();
#ifdef _WIN32
    massert(10297,
            "Couldn't register Windows Ctrl-C handler",
            SetConsoleCtrlHandler(static_cast<PHANDLER_ROUTINE>(CtrlHandler), TRUE));
#else
    // asyncSignals is a global variable listing the signals that should be handled by the
    // interrupt thread, once it is started via startSignalProcessingThread().
    sigemptyset(&asyncSignals);
    sigaddset(&asyncSignals, SIGHUP);
    sigaddset(&asyncSignals, SIGINT);
    sigaddset(&asyncSignals, SIGTERM);
    sigaddset(&asyncSignals, SIGUSR1);
    sigaddset(&asyncSignals, SIGXCPU);
#endif
}

void startSignalProcessingThread(LogFileStatus rotate) {
#ifdef _WIN32
    stdx::thread(eventProcessingThread).detach();
#else
    // Mask signals in the current (only) thread. All new threads will inherit this mask.
    invariant(pthread_sigmask(SIG_SETMASK, &asyncSignals, 0) == 0);
    // Spawn a thread to capture the signals we just masked off.
    stdx::thread(signalProcessingThread, rotate).detach();
#endif
}

#ifdef _WIN32
void removeControlCHandler() {
    massert(28600,
            "Couldn't unregister Windows Ctrl-C handler",
            SetConsoleCtrlHandler(static_cast<PHANDLER_ROUTINE>(CtrlHandler), FALSE));
}
#endif

}  // namespace mongo
