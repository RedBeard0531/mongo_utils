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
#include <dlfcn.h>

#include "mongo/stdx/memory.h"
#include "mongo/util/assert_util.h"
#include "mongo/util/log.h"
#include "mongo/util/mongoutils/str.h"

namespace mongo {

SharedLibrary::~SharedLibrary() {
    if (_handle) {
        if (dlclose(_handle) != 0) {
            LOG(2) << "Load Library close failed " << dlerror();
        }
    }
}

StatusWith<std::unique_ptr<SharedLibrary>> SharedLibrary::create(
    const boost::filesystem::path& full_path) {
    LOG(1) << "Loading library: " << full_path.c_str();

    void* handle = dlopen(full_path.c_str(), RTLD_NOW | RTLD_GLOBAL);
    if (handle == nullptr) {
        return Status(ErrorCodes::InternalError,
                      str::stream() << "Load library failed: " << dlerror());
    }

    return StatusWith<std::unique_ptr<SharedLibrary>>(
        std::unique_ptr<SharedLibrary>(new SharedLibrary(handle)));
}

StatusWith<void*> SharedLibrary::getSymbol(StringData name) {
    // Clear dlerror() before calling dlsym,
    // see man dlerror(3) or dlerror(3p) on any POSIX system for details
    // Ignore return
    dlerror();

    // StringData is not assued to be null-terminated
    std::string symbolName = name.toString();

    void* symbol = dlsym(_handle, symbolName.c_str());

    char* error_msg = dlerror();
    if (error_msg != nullptr) {
        return StatusWith<void*>(ErrorCodes::InternalError,
                                 str::stream() << "dlsym failed for symbol " << name
                                               << " with error message: "
                                               << error_msg);
    }

    return StatusWith<void*>(symbol);
}

}  // namespace mongo
