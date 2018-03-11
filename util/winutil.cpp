// winutil.cpp

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

#if defined(_WIN32)

#include "mongo/platform/basic.h"

#include "mongo/util/winutil.h"

#include "mongo/base/error_codes.h"
#include "mongo/base/status.h"

namespace mongo {

StatusWith<boost::optional<DWORD>> windows::getDWORDRegistryKey(const CString& group,
                                                                const CString& key) {
    CRegKey regkey;
    if (ERROR_SUCCESS != regkey.Open(HKEY_LOCAL_MACHINE, group, KEY_READ)) {
        return Status(ErrorCodes::InternalError, "Unable to access windows registry");
    }

    DWORD val;
    const auto res = regkey.QueryDWORDValue(key, val);
    if (ERROR_INVALID_DATA == res) {
        return Status(ErrorCodes::TypeMismatch,
                      "Invalid data type in windows registry, expected DWORD");
    }

    if (ERROR_SUCCESS != res) {
        return boost::none;
    }

    return val;
}

}  // namespace mongo

#endif
