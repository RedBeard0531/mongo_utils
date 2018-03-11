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

#include "mongo/util/assert_util.h"
#include "mongo/util/scopeguard.h"

namespace mongo {

StackLocator::StackLocator() {
    pthread_t self = pthread_self();
    pthread_attr_t selfAttrs;
    invariant(pthread_attr_init(&selfAttrs) == 0);
    invariant(pthread_getattr_np(self, &selfAttrs) == 0);
    ON_BLOCK_EXIT(pthread_attr_destroy, &selfAttrs);

    void* base = nullptr;
    size_t size = 0;

    auto result = pthread_attr_getstack(&selfAttrs, &base, &size);

    invariant(result == 0);
    invariant(base != nullptr);
    invariant(size != 0);

    // TODO: Assumes a downward growing stack. Note here that
    // getstack returns the stack *base*, being the bottom of the
    // stack, so we need to add size to it.
    _end = base;
    _begin = static_cast<char*>(_end) + size;
}

}  // namespace mongo
