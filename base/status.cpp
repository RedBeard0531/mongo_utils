/*    Copyright 2012 10gen Inc.
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

#include "mongo/base/status.h"
#include "mongo/db/jsobj.h"
#include "mongo/util/log.h"
#include "mongo/util/mongoutils/str.h"

#include <ostream>
#include <sstream>

namespace mongo {

Status::ErrorInfo::ErrorInfo(ErrorCodes::Error code,
                             StringData reason,
                             std::shared_ptr<const ErrorExtraInfo> extra)
    : code(code), reason(reason.toString()), extra(std::move(extra)) {}

Status::ErrorInfo* Status::ErrorInfo::create(ErrorCodes::Error code,
                                             StringData reason,
                                             std::shared_ptr<const ErrorExtraInfo> extra) {
    if (code == ErrorCodes::OK)
        return nullptr;
    if (extra) {
        // The public API prevents getting in to this state.
        invariant(ErrorCodes::shouldHaveExtraInfo(code));
    } else if (ErrorCodes::shouldHaveExtraInfo(code)) {
        // This is possible if code calls a 2-argument Status constructor with a code that should
        // have extra info.
        if (kDebugBuild) {
            // Make it easier to find this issue by fatally failing in debug builds.
            severe() << "Code " << code << " is supposed to have extra info";
            fassertFailed(40680);
        }

        // In release builds, replace the error code. This maintains the invariant that all Statuses
        // for a code that is supposed to hold extra info hold correctly-typed extra info, without
        // crashing the server.
        return new ErrorInfo{ErrorCodes::Error(40671),
                             str::stream() << "Missing required extra info for error code " << code,
                             std::move(extra)};
    }
    return new ErrorInfo{code, reason, std::move(extra)};
}


Status::Status(ErrorCodes::Error code,
               StringData reason,
               std::shared_ptr<const ErrorExtraInfo> extra)
    : _error(ErrorInfo::create(code, reason, std::move(extra))) {
    ref(_error);
}

Status::Status(ErrorCodes::Error code, const std::string& reason) : Status(code, reason, nullptr) {}
Status::Status(ErrorCodes::Error code, const char* reason)
    : Status(code, StringData(reason), nullptr) {}
Status::Status(ErrorCodes::Error code, StringData reason) : Status(code, reason, nullptr) {}

Status::Status(ErrorCodes::Error code, StringData reason, const BSONObj& extraInfoHolder)
    : Status(OK()) {
    if (auto parser = ErrorExtraInfo::parserFor(code)) {
        try {
            *this = Status(code, reason, parser(extraInfoHolder));
        } catch (const DBException& ex) {
            *this = ex.toStatus(str::stream() << "Error parsing extra info for " << code);
        }
    } else {
        *this = Status(code, reason);
    }
}

Status::Status(ErrorCodes::Error code, const mongoutils::str::stream& reason)
    : Status(code, std::string(reason)) {}

Status Status::withReason(StringData newReason) const {
    return isOK() ? OK() : Status(code(), newReason, _error->extra);
}

Status Status::withContext(StringData reasonPrefix) const {
    return isOK() ? OK() : withReason(reasonPrefix + causedBy(reason()));
}

std::ostream& operator<<(std::ostream& os, const Status& status) {
    return os << status.codeString() << " " << status.reason();
}

template <typename Allocator>
StringBuilderImpl<Allocator>& operator<<(StringBuilderImpl<Allocator>& sb, const Status& status) {
    sb << status.codeString();
    if (!status.isOK()) {
        try {
            if (auto extra = status.extraInfo()) {
                BSONObjBuilder bob;
                extra->serialize(&bob);
                sb << bob.obj();
            }
        } catch (const DBException&) {
            // This really shouldn't happen but it would be really annoying if it broke error
            // logging in production.
            if (kDebugBuild) {
                severe() << "Error serializing extra info for " << status.code()
                         << " in Status::toString()";
                std::terminate();
            }
        }
        sb << ": " << status.reason();
    }
    return sb;
}
template StringBuilder& operator<<(StringBuilder& sb, const Status& status);

std::string Status::toString() const {
    StringBuilder sb;
    sb << *this;
    return sb.str();
}

}  // namespace mongo
