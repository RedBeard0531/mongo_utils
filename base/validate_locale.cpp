/*    Copyright 2014 MongoDB, Inc.
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

#include <stdexcept>

#include <boost/filesystem/path.hpp>
#include <codecvt>
#include <locale>

#include "mongo/base/init.h"
#include "mongo/util/mongoutils/str.h"

namespace mongo {

MONGO_INITIALIZER_GENERAL(ValidateLocale, MONGO_NO_PREREQUISITES, MONGO_DEFAULT_PREREQUISITES)
(InitializerContext*) {
    try {
        // Validate that boost can correctly load the user's locale
        boost::filesystem::path("/").has_root_directory();
    } catch (const std::runtime_error& e) {
        return Status(
            ErrorCodes::BadValue,
            str::stream()
                << "Invalid or no user locale set. "
#ifndef _WIN32
                << " Please ensure LANG and/or LC_* environment variables are set correctly. "
#endif
                << e.what());
    }

#ifdef _WIN32
    // Make boost filesystem treat all strings as UTF-8 encoded instead of CP_ACP.
    std::locale loc(std::locale(""), new std::codecvt_utf8_utf16<wchar_t>);
    boost::filesystem::path::imbue(loc);
#endif

    return Status::OK();
}

}  // namespace mongo
