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

#pragma once

#include <memory>
#include <string>

#include "mongo/stdx/functional.h"

namespace mongo {

class ThreadPoolInterface;

/**
 * Sets up a unit test suite named "suiteName" that runs a battery of unit
 * tests against thread pools returned by "makeThreadPool".  These tests should
 * work against any implementation of ThreadPoolInterface.
 */
void addTestsForThreadPool(const std::string& suiteName,
                           stdx::function<std::unique_ptr<ThreadPoolInterface>()> makeThreadPool);

}  // namespace mongo
