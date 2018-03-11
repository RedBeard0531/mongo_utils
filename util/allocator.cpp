// allocator.cpp

/*    Copyright 2014 MongoDB Inc.
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

#include <cstdlib>

#include "mongo/util/signal_handlers_synchronous.h"

namespace mongo {

void* mongoMalloc(size_t size) {
    void* x = std::malloc(size);
    if (x == NULL) {
        reportOutOfMemoryErrorAndExit();
    }
    return x;
}

void* mongoRealloc(void* ptr, size_t size) {
    void* x = std::realloc(ptr, size);
    if (x == NULL) {
        reportOutOfMemoryErrorAndExit();
    }
    return x;
}

}  // namespace mongo
