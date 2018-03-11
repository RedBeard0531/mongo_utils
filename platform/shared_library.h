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
#pragma once

#include <boost/filesystem.hpp>
#include <memory>

#include "mongo/base/status_with.h"

namespace mongo {

/**
 * Loads shared library or DLL at runtime
 * Provides functionality to resolve symols and functions at runtime.
 * Note: shared library is released by destructor
 */
class SharedLibrary {
public:
    /**
     * Releases reference to shared library on destruction.
     *
     * May unload the shared library.
     * May invalidate all symbol pointers, depends on OS implementation.
     */
    ~SharedLibrary();

    /*
     * Loads the shared library
     *
     * Returns a handle to a SharedLibrary on success otherwise StatusWith contains the
     * appropriate error.
     */
    static StatusWith<std::unique_ptr<SharedLibrary>> create(
        const boost::filesystem::path& full_path);

    /**
     * Retrieves the public symbol of a shared library specified in the name parameter.
     *
     * Returns a pointer to the symbol if it exists with Status::OK,
     * returns NULL if the symbol does not exist with Status::OK,
     * otherwise returns an error if the underlying OS infrastructure returns an error.
     */
    StatusWith<void*> getSymbol(StringData name);

    /**
     * A generic function version of getSymbol, see notes in getSymbol for more information
     * Callers should use getFunctionAs.
     */
    StatusWith<void (*)()> getFunction(StringData name);

    /**
     * A type-safe version of getFunction, see notes in getSymbol for more information
     */
    template <typename FuncT>
    StatusWith<FuncT> getFunctionAs(StringData name) {
        StatusWith<void (*)()> s = getFunction(name);

        if (!s.isOK()) {
            return StatusWith<FuncT>(s.getStatus());
        }

        return StatusWith<FuncT>(reinterpret_cast<FuncT>(s.getValue()));
    }

private:
    SharedLibrary(void* handle);

private:
    void* const _handle;
};

}  // namespace mongo
