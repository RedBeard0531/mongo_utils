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

#include <array>
#include <cstdint>
#include <limits>

#include "mongo/base/string_data.h"
#include "mongo/unittest/unittest.h"
#include "mongo/util/itoa.h"

namespace {
using namespace mongo;

TEST(ItoA, StringDataEquality) {
    ASSERT_EQ(ItoA::kBufSize - 1, std::to_string(std::numeric_limits<std::uint64_t>::max()).size());

    for (auto testCase : {uint64_t(1),
                          uint64_t(12),
                          uint64_t(133),
                          uint64_t(1446),
                          uint64_t(17789),
                          uint64_t(192923),
                          uint64_t(2389489),
                          uint64_t(29313479),
                          uint64_t(1928127389),
                          std::numeric_limits<std::uint64_t>::max() - 1,
                          std::numeric_limits<std::uint64_t>::max()}) {
        ItoA itoa{testCase};
        ASSERT_EQ(std::to_string(testCase), StringData(itoa));
    }
}

}  // namespace
