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

#pragma once

#include <cstddef>

namespace mongo {

/**
 * Compare two arrays of bytes for equality in constant time.
 *
 * This means that the function runs for the same amount of time even if they differ. Unlike memcmp,
 * this function does not exit on the first difference.
 *
 * Returns true if the two arrays are equal.
 */
bool consttimeMemEqual(volatile const unsigned char* s1,  // NOLINT - using volatile to
                       volatile const unsigned char* s2,  // NOLINT - disable compiler optimizations
                       size_t length);

}  // namespace mongo
