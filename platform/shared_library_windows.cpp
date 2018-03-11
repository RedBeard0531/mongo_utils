/**
 *    Copyright (C) 2015 MongoDB Inc.
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

#include "mongo/platform/shared_library.h"

#include <boost/filesystem.hpp>

#include "mongo/util/assert_util.h"
#include "mongo/util/log.h"
#include "mongo/util/mongoutils/str.h"
#include "mongo/util/text.h"

namespace mongo {

SharedLibrary::~SharedLibrary() {
    if (_handle) {
        if (FreeLibrary(static_cast<HMODULE>(_handle)) == 0) {
            DWORD lasterror = GetLastError();
            LOG(2) << "Load library close failed: " << errnoWithDescription(lasterror);
        }
    }
}

StatusWith<std::unique_ptr<SharedLibrary>> SharedLibrary::create(
    const boost::filesystem::path& full_path) {
    LOG(1) << "Loading library: " << toUtf8String(full_path.c_str());

    HMODULE handle = LoadLibraryW(full_path.c_str());
    if (handle == nullptr) {
        return StatusWith<std::unique_ptr<SharedLibrary>>(ErrorCodes::InternalError,
                                                          str::stream() << "Load library failed: "
                                                                        << errnoWithDescription());
    }

    return StatusWith<std::unique_ptr<SharedLibrary>>(
        std::unique_ptr<SharedLibrary>(new SharedLibrary(handle)));
}

StatusWith<void*> SharedLibrary::getSymbol(StringData name) {
    // StringData is not assued to be null-terminated
    std::string symbolName = name.toString();

    void* function = GetProcAddress(static_cast<HMODULE>(_handle), symbolName.c_str());

    if (function == nullptr) {
        DWORD gle = GetLastError();
        if (gle != ERROR_PROC_NOT_FOUND) {
            return StatusWith<void*>(ErrorCodes::InternalError,
                                     str::stream() << "GetProcAddress failed for symbol: "
                                                   << errnoWithDescription());
        }
    }

    return StatusWith<void*>(function);
}

}  // namespace mongo
