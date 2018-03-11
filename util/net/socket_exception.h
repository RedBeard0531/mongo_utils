/**
 *    Copyright 2016 MongoDB Inc.
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

#include "mongo/util/assert_util.h"

#include <string>

namespace mongo {
enum class SocketErrorKind {
    CLOSED,
    RECV_ERROR,
    SEND_ERROR,
    RECV_TIMEOUT,
    SEND_TIMEOUT,
    FAILED_STATE,
    CONNECT_ERROR
};

/**
 * Returns a Status with ErrorCodes::SocketException with a correctly formed message.
 */
Status makeSocketError(SocketErrorKind kind,
                       const std::string& server,
                       const std::string& extra = "");

// Using a macro to preserve file/line info from call site.
#define throwSocketError(...)                          \
    do {                                               \
        uassertStatusOK(makeSocketError(__VA_ARGS__)); \
        MONGO_UNREACHABLE;                             \
    } while (false)

using NetworkException = ExceptionForCat<ErrorCategory::NetworkError>;

}  // namespace mongo
