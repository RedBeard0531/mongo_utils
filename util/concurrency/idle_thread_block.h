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

#pragma once

#include <boost/preprocessor/stringize.hpp>

#include "mongo/base/disallow_copying.h"

namespace mongo {

/**
 * Marks a thread as idle while in scope. Prefer to use the macro below.
 *
 * Our debugger scripts can hide idle threads when dumping all stacks. You should mark threads as
 * idle when printing the stack would just be unhelpful noise. IdleThreadBlocks are not allowed to
 * nest. Each thread should generally have at most one possible place where it it is considered
 * idle.
 */
class IdleThreadBlock {
    MONGO_DISALLOW_COPYING(IdleThreadBlock);

public:
    IdleThreadBlock(const char* location) {
        beginIdleThreadBlock(location);
    }
    ~IdleThreadBlock() {
        endIdleThreadBlock();
    }

    // These should not be called by mongo C++ code. They are only public to allow exposing this
    // functionality to a C api.
    static void beginIdleThreadBlock(const char* location);
    static void endIdleThreadBlock();
};

/**
 * Marks a thread idle for the rest of the current scope and passes file:line as the location.
 */
#define MONGO_IDLE_THREAD_BLOCK IdleThreadBlock markIdle(__FILE__ ":" BOOST_PP_STRINGIZE(__LINE__))

}  // namespace mongo
