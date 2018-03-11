// assert_util.cpp

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

#define MONGO_LOG_DEFAULT_COMPONENT ::mongo::logger::LogComponent::kDefault

#include "mongo/platform/basic.h"

#include "mongo/util/assert_util.h"

using namespace std;

#ifndef _WIN32
#include <cxxabi.h>
#include <sys/file.h>
#endif

#include <boost/exception/diagnostic_information.hpp>
#include <boost/exception/exception.hpp>
#include <exception>

#include "mongo/config.h"
#include "mongo/util/debug_util.h"
#include "mongo/util/debugger.h"
#include "mongo/util/exit.h"
#include "mongo/util/log.h"
#include "mongo/util/mongoutils/str.h"
#include "mongo/util/quick_exit.h"
#include "mongo/util/stacktrace.h"

namespace mongo {

AssertionCount assertionCount;

AssertionCount::AssertionCount() : regular(0), warning(0), msg(0), user(0), rollovers(0) {}

void AssertionCount::rollover() {
    rollovers++;
    regular = 0;
    warning = 0;
    msg = 0;
    user = 0;
}

void AssertionCount::condrollover(int newvalue) {
    static const int rolloverPoint = (1 << 30);
    if (newvalue >= rolloverPoint)
        rollover();
}

AtomicBool DBException::traceExceptions(false);

void DBException::traceIfNeeded(const DBException& e) {
    if (traceExceptions.load()) {
        warning() << "DBException thrown" << causedBy(e) << endl;
        printStackTrace();
    }
}

NOINLINE_DECL void verifyFailed(const char* expr, const char* file, unsigned line) {
    assertionCount.condrollover(++assertionCount.regular);
    error() << "Assertion failure " << expr << ' ' << file << ' ' << dec << line << endl;
    logContext();
    stringstream temp;
    temp << "assertion " << file << ":" << line;

    breakpoint();
#if defined(MONGO_CONFIG_DEBUG_BUILD)
    // this is so we notice in buildbot
    severe() << "\n\n***aborting after verify() failure as this is a debug/test build\n\n" << endl;
    std::abort();
#endif
    error_details::throwExceptionForStatus(Status(ErrorCodes::UnknownError, temp.str()));
}

NOINLINE_DECL void invariantFailed(const char* expr, const char* file, unsigned line) noexcept {
    severe() << "Invariant failure " << expr << ' ' << file << ' ' << dec << line << endl;
    breakpoint();
    severe() << "\n\n***aborting after invariant() failure\n\n" << endl;
    std::abort();
}

NOINLINE_DECL void invariantFailedWithMsg(const char* expr,
                                          const char* msg,
                                          const char* file,
                                          unsigned line) noexcept {
    severe() << "Invariant failure " << expr << " " << msg << " " << file << ' ' << dec << line
             << endl;
    breakpoint();
    severe() << "\n\n***aborting after invariant() failure\n\n" << endl;
    std::abort();
}

NOINLINE_DECL void invariantFailedWithMsg(const char* expr,
                                          const std::string& msg,
                                          const char* file,
                                          unsigned line) noexcept {
    invariantFailedWithMsg(expr, msg.c_str(), file, line);
}

NOINLINE_DECL void invariantOKFailed(const char* expr,
                                     const Status& status,
                                     const char* file,
                                     unsigned line) noexcept {
    severe() << "Invariant failure: " << expr << " resulted in status " << redact(status) << " at "
             << file << ' ' << dec << line;
    breakpoint();
    severe() << "\n\n***aborting after invariant() failure\n\n" << endl;
    std::abort();
}

NOINLINE_DECL void fassertFailedWithLocation(int msgid, const char* file, unsigned line) noexcept {
    severe() << "Fatal Assertion " << msgid << " at " << file << " " << dec << line;
    breakpoint();
    severe() << "\n\n***aborting after fassert() failure\n\n" << endl;
    std::abort();
}

NOINLINE_DECL void fassertFailedNoTraceWithLocation(int msgid,
                                                    const char* file,
                                                    unsigned line) noexcept {
    severe() << "Fatal Assertion " << msgid << " at " << file << " " << dec << line;
    breakpoint();
    severe() << "\n\n***aborting after fassert() failure\n\n" << endl;
    quickExit(EXIT_ABRUPT);
}

MONGO_COMPILER_NORETURN void fassertFailedWithStatusWithLocation(int msgid,
                                                                 const Status& status,
                                                                 const char* file,
                                                                 unsigned line) noexcept {
    severe() << "Fatal assertion " << msgid << " " << redact(status) << " at " << file << " " << dec
             << line;
    breakpoint();
    severe() << "\n\n***aborting after fassert() failure\n\n" << endl;
    std::abort();
}

MONGO_COMPILER_NORETURN void fassertFailedWithStatusNoTraceWithLocation(int msgid,
                                                                        const Status& status,
                                                                        const char* file,
                                                                        unsigned line) noexcept {
    severe() << "Fatal assertion " << msgid << " " << redact(status) << " at " << file << " " << dec
             << line;
    breakpoint();
    severe() << "\n\n***aborting after fassert() failure\n\n" << endl;
    quickExit(EXIT_ABRUPT);
}

NOINLINE_DECL void uassertedWithLocation(const Status& status, const char* file, unsigned line) {
    assertionCount.condrollover(++assertionCount.user);
    LOG(1) << "User Assertion: " << redact(status) << ' ' << file << ' ' << dec << line;
    error_details::throwExceptionForStatus(status);
}

NOINLINE_DECL void msgassertedWithLocation(const Status& status, const char* file, unsigned line) {
    assertionCount.condrollover(++assertionCount.msg);
    error() << "Assertion: " << redact(status) << ' ' << file << ' ' << dec << line;
    error_details::throwExceptionForStatus(status);
}

std::string causedBy(StringData e) {
    constexpr auto prefix = " :: caused by :: "_sd;
    std::string out;
    out.reserve(prefix.size() + e.size());
    out.append(prefix.rawData(), prefix.size());
    out.append(e.rawData(), e.size());
    return out;
}

std::string causedBy(const char* e) {
    return causedBy(StringData(e));
}

std::string causedBy(const DBException& e) {
    return causedBy(e.toString());
}

std::string causedBy(const std::exception& e) {
    return causedBy(e.what());
}

std::string causedBy(const std::string& e) {
    return causedBy(StringData(e));
}

std::string causedBy(const Status& e) {
    return causedBy(e.toString());
}

string demangleName(const type_info& typeinfo) {
#ifdef _WIN32
    return typeinfo.name();
#else
    int status;

    char* niceName = abi::__cxa_demangle(typeinfo.name(), 0, 0, &status);
    if (!niceName)
        return typeinfo.name();

    string s = niceName;
    free(niceName);
    return s;
#endif
}

Status exceptionToStatus() noexcept {
    try {
        throw;
    } catch (const DBException& ex) {
        return ex.toStatus();
    } catch (const std::exception& ex) {
        return Status(ErrorCodes::UnknownError,
                      str::stream() << "Caught std::exception of type " << demangleName(typeid(ex))
                                    << ": "
                                    << ex.what());
    } catch (const boost::exception& ex) {
        return Status(
            ErrorCodes::UnknownError,
            str::stream() << "Caught boost::exception of type " << demangleName(typeid(ex)) << ": "
                          << boost::diagnostic_information(ex));

    } catch (...) {
        severe() << "Caught unknown exception in exceptionToStatus()";
        std::terminate();
    }
}
}
