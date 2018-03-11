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

#include "mongo/platform/stack_locator.h"

#include <thread.h>

#include "mongo/util/assert_util.h"

namespace mongo {

StackLocator::StackLocator() {
    stack_t stack;
    invariant(thr_stksegment(&stack) == 0);

    invariant(stack.ss_sp != nullptr);
    invariant(stack.ss_size != 0);

    // TODO: Assumes stack grows downward on Solaris
    _begin = stack.ss_sp;
    _end = static_cast<char*>(_begin) - stack.ss_size;
}

}  // namespace mongo
