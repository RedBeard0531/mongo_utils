// @file winutil.cpp : Windows related utility functions
//
// /**
// *    Copyright (C) 2008 10gen Inc.
// *
//  *   Licensed under the Apache License, Version 2.0 (the "License");
//  *   you may not use this file except in compliance with the License.
//  *   You may obtain a copy of the License at
//  *
//  *       http://www.apache.org/licenses/LICENSE-2.0
//  *
//  *   Unless required by applicable law or agreed to in writing, software
//  *   distributed under the License is distributed on an "AS IS" BASIS,
//  *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  *   See the License for the specific language governing permissions and
//  *   limitations under the License.
// */
//

#pragma once

#if defined(_WIN32)
#include "text.h"
#include <atlbase.h>
#include <atlstr.h>
#include <boost/optional.hpp>
#include <sstream>
#include <string>
#include <windows.h>

#include <mongo/base/status_with.h>

namespace mongo {
namespace windows {

inline std::string GetErrMsg(DWORD err) {
    LPTSTR errMsg;
    ::FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                    NULL,
                    err,
                    0,
                    (LPTSTR)&errMsg,
                    0,
                    NULL);
    std::string errMsgStr = toUtf8String(errMsg);
    ::LocalFree(errMsg);
    // FormatMessage() appends a newline to the end of error messages, we trim it because std::endl
    // flushes the buffer.
    errMsgStr = errMsgStr.erase(errMsgStr.length() - 2);
    std::ostringstream output;
    output << errMsgStr << " (" << err << ")";

    return output.str();
}

/**
 * Retrieve a DWORD value from the Local Machine Windows Registry for element:
 * group\key
 * e.g. HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\Tcpip\Parameters\KeepAliveTime
 *
 * On success, returns:
 *   boost::none if the key does not exist.
 *   The value read from the registry.
 *
 * On failure, returns:
 *   ErrorCodes::InternalError - Unable to access the registry group.
 *   ErrorCodes::TypeMismatch - Key exists, but is of the wrong type.
 */
StatusWith<boost::optional<DWORD>> getDWORDRegistryKey(const CString& group, const CString& key);

}  // namespace windows
}  // namespace mongo

#endif
