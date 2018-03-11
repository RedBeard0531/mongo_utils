/*    Copyright 2015 MongoDB Inc.
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

#include "mongo/unittest/death_test.h"
#include "mongo/unittest/unittest.h"
#include "mongo/util/assert_util.h"
#include "mongo/util/secure_zero_memory.h"

namespace mongo {

TEST(SecureZeroMemoryTest, zeroZeroLengthNull) {
    void* ptr = nullptr;
    secureZeroMemory(ptr, 0);
    ASSERT_TRUE(true);
}

DEATH_TEST(SecureZeroMemoryTest, zeroNonzeroLengthNull, "Fatal Assertion") {
    void* ptr = nullptr;
    secureZeroMemory(ptr, 1000);
}

TEST(SecureZeroMemoryTest, dataZeroed) {
    static const size_t dataSize = 100;
    std::uint8_t data[dataSize];

    // Populate array
    for (size_t i = 0; i < dataSize; ++i) {
        data[i] = i;
    }

    // Zero array
    secureZeroMemory(data, dataSize);

    // Check contents
    for (size_t i = 0; i < dataSize; ++i) {
        ASSERT_FALSE(data[i]);
    }
}

}  // namespace mongo
