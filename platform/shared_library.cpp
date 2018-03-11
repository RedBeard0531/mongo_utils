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

#include "mongo/platform/basic.h"

#include "mongo/platform/shared_library.h"

namespace mongo {

typedef StatusWith<void (*)()> StatusWithFunc;

SharedLibrary::SharedLibrary(void* handle) : _handle(handle) {}

StatusWithFunc SharedLibrary::getFunction(StringData name) {
    StatusWith<void*> s = getSymbol(name);

    if (!s.isOK()) {
        return StatusWithFunc(s.getStatus());
    }

    return StatusWithFunc(reinterpret_cast<void (*)()>(s.getValue()));
}

}  // namespace mongo
