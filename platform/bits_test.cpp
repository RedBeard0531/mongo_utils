// bits_test.cpp

/*    Copyright 2012 10gen Inc.
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

#include "mongo/platform/bits.h"

#include "mongo/unittest/unittest.h"

namespace mongo {

TEST(BitsTest_CountZeros, Constants) {
    ASSERT_EQUALS(countLeadingZeros64(0ull), 64);
    ASSERT_EQUALS(countTrailingZeros64(0ull), 64);

    ASSERT_EQUALS(countLeadingZeros64(0x1234ull), 64 - 13);
    ASSERT_EQUALS(countTrailingZeros64(0x1234ull), 2);

    ASSERT_EQUALS(countLeadingZeros64(0x1234ull << 32), 32 - 13);
    ASSERT_EQUALS(countTrailingZeros64(0x1234ull << 32), 2 + 32);

    ASSERT_EQUALS(countLeadingZeros64((0x1234ull << 32) | 0x1234ull), 32 - 13);
    ASSERT_EQUALS(countTrailingZeros64((0x1234ull << 32) | 0x1234ull), 2);
}

TEST(BitsTest_CountZeros, EachBit) {
    for (int i = 0; i < 64; i++) {
        unsigned long long x = 1ULL << i;
        ASSERT_EQUALS(countLeadingZeros64(x), 64 - 1 - i);
        ASSERT_EQUALS(countTrailingZeros64(x), i);
    }
}
}
