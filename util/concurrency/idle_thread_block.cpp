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

#include "mongo/util/assert_util.h"
#include "mongo/util/concurrency/idle_thread_block.h"

namespace mongo {
namespace for_debuggers {
// This needs external linkage to ensure that debuggers can use it.
thread_local const char* idleThreadLocation = nullptr;
}
using for_debuggers::idleThreadLocation;

void IdleThreadBlock::beginIdleThreadBlock(const char* location) {
    invariant(!idleThreadLocation);
    idleThreadLocation = location;
}

void IdleThreadBlock::endIdleThreadBlock() {
    invariant(idleThreadLocation);
    idleThreadLocation = nullptr;
}
}
