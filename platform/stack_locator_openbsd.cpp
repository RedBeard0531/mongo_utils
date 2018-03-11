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

#include <pthread.h>
#include <pthread_np.h>
#include <sys/signal.h>

#include "mongo/util/assert_util.h"
#include "mongo/util/scopeguard.h"

namespace mongo {

StackLocator::StackLocator() {
    stack_t stack_info;

    pthread_t self = pthread_self();

    invariant(pthread_stackseg_np(self, &stack_info) == 0);

    _begin = stack_info.ss_sp;

    // TODO: Assumes stack grows downward on OpenBSD
    _end = static_cast<char*>(_begin) - stack_info.ss_size;
}

}  // namespace mongo
