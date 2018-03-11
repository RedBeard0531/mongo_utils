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

#include "mongo/platform/basic.h"

#include "mongo/util/net/socket_exception.h"

#include "mongo/util/mongoutils/str.h"

namespace mongo {

namespace {

std::string getStringType(SocketErrorKind kind) {
    switch (kind) {
        case SocketErrorKind::CLOSED:
            return "CLOSED";
        case SocketErrorKind::RECV_ERROR:
            return "RECV_ERROR";
        case SocketErrorKind::SEND_ERROR:
            return "SEND_ERROR";
        case SocketErrorKind::RECV_TIMEOUT:
            return "RECV_TIMEOUT";
        case SocketErrorKind::SEND_TIMEOUT:
            return "SEND_TIMEOUT";
        case SocketErrorKind::FAILED_STATE:
            return "FAILED_STATE";
        case SocketErrorKind::CONNECT_ERROR:
            return "CONNECT_ERROR";
        default:
            return "UNKNOWN";  // should never happen
    }
}

}  // namespace

Status makeSocketError(SocketErrorKind kind, const std::string& server, const std::string& extra) {
    StringBuilder ss;
    ss << "socket exception [" << getStringType(kind) << "]";

    if (!server.empty())
        ss << " server [" << server << "]";

    if (!extra.empty())
        ss << ' ' << extra;

    return Status(ErrorCodes::SocketException, ss.str());
}


}  // namespace mongo
