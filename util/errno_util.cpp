/*    Copyright 2017 10gen Inc.
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

#include "mongo/util/errno_util.h"

#include <sstream>

#ifndef _WIN32
#include <cstring>  // For strerror_r
#include <errno.h>  // For errno
#endif

#include "mongo/util/scopeguard.h"
#include "mongo/util/text.h"

namespace mongo {

namespace {
const char kUnknownMsg[] = "Unknown error";
const int kBuflen = 256;  // strerror strings in non-English locales can be large.
}  // namespace

std::string errnoWithDescription(int errNumber) {
#if defined(_WIN32)
    if (errNumber == -1)
        errNumber = GetLastError();
#else
    if (errNumber < 0)
        errNumber = errno;
#endif

    char buf[kBuflen];
    char* msg{nullptr};

#if defined(__GNUC__) && defined(_GNU_SOURCE)
    msg = strerror_r(errNumber, buf, kBuflen);
#elif defined(_WIN32)

    LPWSTR errorText = nullptr;
    FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER |
                       FORMAT_MESSAGE_IGNORE_INSERTS,
                   nullptr,
                   errNumber,
                   0,
                   reinterpret_cast<LPWSTR>(&errorText),  // output
                   0,                                     // minimum size for output buffer
                   nullptr);

    if (errorText) {
        ON_BLOCK_EXIT([&errorText] { LocalFree(errorText); });
        std::string utf8ErrorText = toUtf8String(errorText);
        auto size = utf8ErrorText.find_first_of("\r\n");
        if (size == std::string::npos) {  // not found
            size = utf8ErrorText.length();
        }

        if (size >= kBuflen) {
            size = kBuflen - 1;
        }

        memcpy(buf, utf8ErrorText.c_str(), size);
        buf[size] = '\0';
        msg = buf;
    } else if (strerror_s(buf, kBuflen, errNumber) != 0) {
        msg = buf;
    }
#else /* XSI strerror_r */
    if (strerror_r(errNumber, buf, kBuflen) == 0) {
        msg = buf;
    }
#endif

    if (!msg) {
        return {kUnknownMsg};
    }

    return {msg};
}

std::pair<int, std::string> errnoAndDescription() {
#if defined(_WIN32)
    int errNumber = GetLastError();
#else
    int errNumber = errno;
#endif
    return {errNumber, errnoWithDescription(errNumber)};
}

std::string errnoWithPrefix(StringData prefix) {
    const auto suffix = errnoWithDescription();
    std::stringstream ss;
    if (!prefix.empty())
        ss << prefix << ": ";
    ss << suffix;
    return ss.str();
}

}  // namespace mongo
