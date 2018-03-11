/*    Copyright 2017 MongoDB Inc.
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

#include "mongo/util/secure_compare_memory.h"

namespace mongo {

bool consttimeMemEqual(volatile const unsigned char* s1,  // NOLINT - using volatile to
                       volatile const unsigned char* s2,  // NOLINT - disable compiler optimizations
                       size_t length) {
    unsigned int ret = 0;

    for (size_t i = 0; i < length; ++i) {
        ret |= s1[i] ^ s2[i];
    }

    return (1 & ((ret - 1) >> 8));
}

}  // namespace mongo
