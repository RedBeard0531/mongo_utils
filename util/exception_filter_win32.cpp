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

#ifdef _WIN32

#include "mongo/platform/basic.h"

#pragma warning(push)
// C4091: 'typedef ': ignored on left of '' when no variable is declared
#pragma warning(disable : 4091)
#include <DbgHelp.h>
#pragma warning(pop)

#include <ostream>

#include "mongo/config.h"
#include "mongo/util/assert_util.h"
#include "mongo/util/exit_code.h"
#include "mongo/util/log.h"
#include "mongo/util/quick_exit.h"
#include "mongo/util/stacktrace.h"
#include "mongo/util/text.h"

namespace mongo {

namespace {
/* create a process dump.
    To use, load up windbg.  Set your symbol and source path.
    Open the crash dump file.  To see the crashing context, use .ecxr in windbg
    TODO: consider using WER local dumps in the future
    */
void doMinidumpWithException(struct _EXCEPTION_POINTERS* exceptionInfo) {
    WCHAR moduleFileName[MAX_PATH];

    DWORD ret = GetModuleFileNameW(NULL, &moduleFileName[0], ARRAYSIZE(moduleFileName));
    if (ret == 0) {
        int gle = GetLastError();
        log() << "GetModuleFileName failed " << errnoWithDescription(gle);

        // Fallback name
        wcscpy_s(moduleFileName, L"mongo");
    } else {
        WCHAR* dotStr = wcschr(&moduleFileName[0], L'.');
        if (dotStr != NULL) {
            *dotStr = L'\0';
        }
    }

    std::wstring dumpName(moduleFileName);

    std::string currentTime = terseCurrentTime(false);

    dumpName += L".";

    dumpName += toWideString(currentTime.c_str());

    dumpName += L".mdmp";

    HANDLE hFile = CreateFileW(
        dumpName.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (INVALID_HANDLE_VALUE == hFile) {
        DWORD lasterr = GetLastError();
        log() << "failed to open minidump file " << toUtf8String(dumpName.c_str()) << " : "
              << errnoWithDescription(lasterr);
        return;
    }

    MINIDUMP_EXCEPTION_INFORMATION aMiniDumpInfo;
    aMiniDumpInfo.ThreadId = GetCurrentThreadId();
    aMiniDumpInfo.ExceptionPointers = exceptionInfo;
    aMiniDumpInfo.ClientPointers = FALSE;

    MINIDUMP_TYPE miniDumpType =
#ifdef MONGO_CONFIG_DEBUG_BUILD
        MiniDumpWithFullMemory;
#else
        static_cast<MINIDUMP_TYPE>(MiniDumpNormal | MiniDumpWithIndirectlyReferencedMemory |
                                   MiniDumpWithProcessThreadData);
#endif
    log() << "writing minidump diagnostic file " << toUtf8String(dumpName.c_str());

    BOOL bstatus = MiniDumpWriteDump(GetCurrentProcess(),
                                     GetCurrentProcessId(),
                                     hFile,
                                     miniDumpType,
                                     exceptionInfo != NULL ? &aMiniDumpInfo : NULL,
                                     NULL,
                                     NULL);
    if (FALSE == bstatus) {
        DWORD lasterr = GetLastError();
        log() << "failed to create minidump : " << errnoWithDescription(lasterr);
    }

    CloseHandle(hFile);
}

LONG WINAPI exceptionFilter(struct _EXCEPTION_POINTERS* excPointers) {
    char exceptionString[128];
    sprintf_s(exceptionString,
              sizeof(exceptionString),
              (excPointers->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION)
                  ? "(access violation)"
                  : "0x%08X",
              excPointers->ExceptionRecord->ExceptionCode);
    char addressString[32];
    sprintf_s(addressString,
              sizeof(addressString),
              "0x%p",
              excPointers->ExceptionRecord->ExceptionAddress);
    log() << "*** unhandled exception " << exceptionString << " at " << addressString
          << ", terminating";
    if (excPointers->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION) {
        ULONG acType = excPointers->ExceptionRecord->ExceptionInformation[0];
        const char* acTypeString;
        switch (acType) {
            case 0:
                acTypeString = "read from";
                break;
            case 1:
                acTypeString = "write to";
                break;
            case 8:
                acTypeString = "DEP violation at";
                break;
            default:
                acTypeString = "unknown violation at";
                break;
        }
        sprintf_s(addressString,
                  sizeof(addressString),
                  " 0x%llx",
                  excPointers->ExceptionRecord->ExceptionInformation[1]);
        log() << "*** access violation was a " << acTypeString << addressString;
    }

    log() << "*** stack trace for unhandled exception:";

    // Create a copy of context record because printWindowsStackTrace will mutate it.
    CONTEXT contextCopy(*(excPointers->ContextRecord));

    printWindowsStackTrace(contextCopy);

    doMinidumpWithException(excPointers);

    // Don't go through normal shutdown procedure. It may make things worse.
    // Do not go through _exit or ExitProcess(), terminate immediately
    log() << "*** immediate exit due to unhandled exception";
    TerminateProcess(GetCurrentProcess(), EXIT_ABRUPT);

    // We won't reach here
    return EXCEPTION_EXECUTE_HANDLER;
}
}

LPTOP_LEVEL_EXCEPTION_FILTER filtLast = 0;

void setWindowsUnhandledExceptionFilter() {
    filtLast = SetUnhandledExceptionFilter(exceptionFilter);
}

}  // namespace mongo

#else

namespace mongo {
void setWindowsUnhandledExceptionFilter() {}
}

#endif  // _WIN32
