/*    Copyright 2015 10gen Inc.
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

namespace mongo {

/**
 * Wrapper around several platform specific methods for zeroing memory
 * Memory zeroing is complicated by the fact that compilers will try to optimize it away, as the
 * memory frequently will not be later read.
 *
 * This function will, if available, perform a platform specific operation to zero memory. If no
 * platform specific operation is available, memory will be zeroed using volatile pointers.
 */
void secureZeroMemory(void* ptr, size_t size);


}  // namespace mongo
