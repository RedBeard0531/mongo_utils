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

#define MONGO_LOG_DEFAULT_COMPONENT ::mongo::logger::LogComponent::kControl

#include "mongo/platform/basic.h"

#include "mongo/util/log.h"

#include "mongo/logger/console_appender.h"
#include "mongo/logger/message_event_utf8_encoder.h"
#include "mongo/logger/ramlog.h"
#include "mongo/logger/rotatable_file_manager.h"
#include "mongo/util/assert_util.h"
#include "mongo/util/concurrency/thread_name.h"
#include "mongo/util/stacktrace.h"
#include "mongo/util/time_support.h"

using namespace std;

// TODO: Win32 unicode console writing (in logger/console_appender?).
// TODO: Extra log context appending, and re-enable log_user_*.js
// TODO: Eliminate cout/cerr.

namespace mongo {

static logger::ExtraLogContextFn _appendExtraLogContext;

Status logger::registerExtraLogContextFn(logger::ExtraLogContextFn contextFn) {
    if (!contextFn)
        return Status(ErrorCodes::BadValue, "Cannot register a NULL log context function.");
    if (_appendExtraLogContext) {
        return Status(ErrorCodes::AlreadyInitialized,
                      "Cannot call registerExtraLogContextFn multiple times.");
    }
    _appendExtraLogContext = contextFn;
    return Status::OK();
}

bool rotateLogs(bool renameFiles) {
    using logger::RotatableFileManager;
    RotatableFileManager* manager = logger::globalRotatableFileManager();
    log() << "Log rotation initiated";
    RotatableFileManager::FileNameStatusPairVector result(
        manager->rotateAll(renameFiles, "." + terseCurrentTime(false)));
    for (RotatableFileManager::FileNameStatusPairVector::iterator it = result.begin();
         it != result.end();
         it++) {
        warning() << "Rotating log file " << it->first << " failed: " << it->second.toString();
    }
    return result.empty();
}

void logContext(const char* errmsg) {
    if (errmsg) {
        log() << errmsg << endl;
    }
    // NOTE: We disable long-line truncation for the stack trace, because the JSON representation of
    // the stack trace can sometimes exceed the long line limit.
    printStackTrace(log().setIsTruncatable(false).stream());
}

void setPlainConsoleLogger() {
    logger::globalLogManager()->getGlobalDomain()->clearAppenders();
    logger::globalLogManager()->getGlobalDomain()->attachAppender(
        std::make_unique<logger::ConsoleAppender<logger::MessageEventEphemeral>>(
            std::make_unique<logger::MessageEventUnadornedEncoder>()));
}

Tee* const warnings = RamLog::get("warnings");  // Things put here go in serverStatus
Tee* const startupWarningsLog = RamLog::get("startupWarnings");  // intentionally leaked

}  // namespace mongo
