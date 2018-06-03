/**
 *    Copyright (C) 2012 10gen Inc.
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

#include <boost/optional.hpp>
#include <iostream>
#include <vector>

#include "mongo/unittest/unittest.h"
#include "mongo/util/processinfo.h"

using mongo::ProcessInfo;
using boost::optional;

namespace mongo_test {
TEST(ProcessInfo, SysInfoIsInitialized) {
    ProcessInfo processInfo;
    if (processInfo.supported()) {
        ASSERT_FALSE(processInfo.getOsType().empty());
    }
}

TEST(ProcessInfo, NonZeroPageSize) {
    if (ProcessInfo::blockCheckSupported()) {
        ASSERT_GREATER_THAN(ProcessInfo::getPageSize(), 0u);
    }
}

TEST(ProcessInfo, GetNumAvailableCores) {
#if defined(__APPLE__) || defined(__linux__) || (defined(__sun) && defined(__SVR4)) || \
    defined(_WIN32)
    unsigned long numAvailCores = ProcessInfo::getNumAvailableCores();
    ASSERT_GREATER_THAN(numAvailCores, 0u);
    ASSERT_LESS_THAN_OR_EQUALS(numAvailCores, ProcessInfo::getNumCores());
#endif
}

TEST(ProcessInfo, GetNumCoresReturnsNonZeroNumberOfProcessors) {
    ASSERT_GREATER_THAN(ProcessInfo::getNumCores(), 0u);
}
}
