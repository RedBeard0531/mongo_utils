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

#include "mongo/config.h"

#if defined(MONGO_CONFIG_HAVE_MEMSET_S)
#define __STDC_WANT_LIB_EXT1__ 1
#endif

#include "mongo/platform/basic.h"

#include <cstring>

#include "mongo/util/assert_util.h"

namespace mongo {

void secureZeroMemory(void* mem, size_t size) {
    if (mem == nullptr) {
        fassert(28751, size == 0);
        return;
    }

#if defined(_WIN32)
    // Windows provides a simple function for zeroing memory
    SecureZeroMemory(mem, size);
#elif defined(MONGO_CONFIG_HAVE_MEMSET_S)
    // Some C11 libraries provide a variant of memset which is guaranteed to not be optimized away
    fassert(28752, memset_s(mem, size, 0, size) == 0);
#else
    // fall back to using volatile pointer
    // using volatile to disable compiler optimizations
    volatile char* p = reinterpret_cast<volatile char*>(mem);  // NOLINT
    while (size--) {
        *p++ = 0;
    }
#endif
}

}  // namespace mongo
