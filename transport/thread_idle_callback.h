/**
 *    Copyright (C) 2016 MongoDB Inc.
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
 * Type of callback functions that can be invoked when markThreadIdle() runs. These functions *must
 * not throw*.
 */
typedef void (*ThreadIdleCallback)();

/**
 * Informs the registered listener that this thread believes it may go idle for an extended period.
 * The caller should avoid calling markThreadIdle at a high rate, as it can both be moderately
 * costly itself and in terms of distributed overhead for subsequent malloc/free calls.
 */
void markThreadIdle();

/**
 * Allows for registering callbacks for when threads go idle and become active. This is used by
 * TCMalloc to return freed memory to its central freelist at appropriate points, so it won't happen
 * during critical sections while holding locks. Calling this is not thread-safe.
 */
void registerThreadIdleCallback(ThreadIdleCallback callback);

}  // namespace mongo
