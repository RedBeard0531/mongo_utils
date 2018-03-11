/*    Copyright 2016 MongoDB, Inc.
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

#include <limits>

#include "mongo/platform/overflow_arithmetic.h"
#include "mongo/unittest/unittest.h"

namespace mongo {
namespace {


#define assertArithOverflow(TYPE, FN, LHS, RHS, EXPECT_OVERFLOW, EXPECTED_RESULT)            \
    do {                                                                                     \
        const bool expectOverflow = EXPECT_OVERFLOW;                                         \
        TYPE result;                                                                         \
        ASSERT_EQ(expectOverflow, FN(LHS, RHS, &result)) << #FN "(" #LHS ", " #RHS;          \
        if (!expectOverflow) {                                                               \
            ASSERT_EQ(TYPE(EXPECTED_RESULT), TYPE(result)) << #FN "(" #LHS ", " #RHS " - >"; \
        }                                                                                    \
    } while (false)

#define assertSignedMultiplyNoOverflow(LHS, RHS, EXPECTED) \
    assertArithOverflow(int64_t, mongoSignedMultiplyOverflow64, LHS, RHS, false, EXPECTED)
#define assertSignedMultiplyWithOverflow(LHS, RHS) \
    assertArithOverflow(int64_t, mongoSignedMultiplyOverflow64, LHS, RHS, true, 0)

#define assertUnsignedMultiplyNoOverflow(LHS, RHS, EXPECTED) \
    assertArithOverflow(uint64_t, mongoUnsignedMultiplyOverflow64, LHS, RHS, false, EXPECTED)
#define assertUnsignedMultiplyWithOverflow(LHS, RHS) \
    assertArithOverflow(uint64_t, mongoUnsignedMultiplyOverflow64, LHS, RHS, true, 0)

#define assertSignedAddNoOverflow(LHS, RHS, EXPECTED) \
    assertArithOverflow(int64_t, mongoSignedAddOverflow64, LHS, RHS, false, EXPECTED)
#define assertSignedAddWithOverflow(LHS, RHS) \
    assertArithOverflow(int64_t, mongoSignedAddOverflow64, LHS, RHS, true, 0)

#define assertUnsignedAddNoOverflow(LHS, RHS, EXPECTED) \
    assertArithOverflow(uint64_t, mongoUnsignedAddOverflow64, LHS, RHS, false, EXPECTED)
#define assertUnsignedAddWithOverflow(LHS, RHS) \
    assertArithOverflow(uint64_t, mongoUnsignedAddOverflow64, LHS, RHS, true, 0)

#define assertSignedSubtractNoOverflow(LHS, RHS, EXPECTED) \
    assertArithOverflow(int64_t, mongoSignedSubtractOverflow64, LHS, RHS, false, EXPECTED)
#define assertSignedSubtractWithOverflow(LHS, RHS) \
    assertArithOverflow(int64_t, mongoSignedSubtractOverflow64, LHS, RHS, true, 0)

#define assertUnsignedSubtractNoOverflow(LHS, RHS, EXPECTED) \
    assertArithOverflow(uint64_t, mongoUnsignedSubtractOverflow64, LHS, RHS, false, EXPECTED)
#define assertUnsignedSubtractWithOverflow(LHS, RHS) \
    assertArithOverflow(uint64_t, mongoUnsignedSubtractOverflow64, LHS, RHS, true, 0)

TEST(OverflowArithmetic, SignedMultiplicationTests) {
    using limits = std::numeric_limits<int64_t>;
    assertSignedMultiplyNoOverflow(0, limits::max(), 0);
    assertSignedMultiplyNoOverflow(0, limits::min(), 0);
    assertSignedMultiplyNoOverflow(1, limits::max(), limits::max());
    assertSignedMultiplyNoOverflow(1, limits::min(), limits::min());
    assertSignedMultiplyNoOverflow(-1, limits::max(), limits::min() + 1);
    assertSignedMultiplyNoOverflow(1000, 57, 57000);
    assertSignedMultiplyNoOverflow(1000, -57, -57000);
    assertSignedMultiplyNoOverflow(-1000, -57, 57000);
    assertSignedMultiplyNoOverflow(0x3fffffffffffffff, 2, 0x7ffffffffffffffe);
    assertSignedMultiplyNoOverflow(0x3fffffffffffffff, -2, -0x7ffffffffffffffe);
    assertSignedMultiplyNoOverflow(-0x3fffffffffffffff, -2, 0x7ffffffffffffffe);

    assertSignedMultiplyWithOverflow(-1, limits::min());
    assertSignedMultiplyWithOverflow(2, limits::max());
    assertSignedMultiplyWithOverflow(-2, limits::max());
    assertSignedMultiplyWithOverflow(2, limits::min());
    assertSignedMultiplyWithOverflow(-2, limits::min());
    assertSignedMultiplyWithOverflow(limits::min(), limits::max());
    assertSignedMultiplyWithOverflow(limits::max(), limits::max());
    assertSignedMultiplyWithOverflow(limits::min(), limits::min());
    assertSignedMultiplyWithOverflow(1LL << 62, 8);
    assertSignedMultiplyWithOverflow(-(1LL << 62), 8);
    assertSignedMultiplyWithOverflow(-(1LL << 62), -8);
}

TEST(OverflowArithmetic, UnignedMultiplicationTests) {
    using limits = std::numeric_limits<uint64_t>;
    assertUnsignedMultiplyNoOverflow(0, limits::max(), 0);
    assertUnsignedMultiplyNoOverflow(1, limits::max(), limits::max());
    assertUnsignedMultiplyNoOverflow(1000, 57, 57000);
    assertUnsignedMultiplyNoOverflow(0x3fffffffffffffff, 2, 0x7ffffffffffffffe);
    assertUnsignedMultiplyNoOverflow(0x7fffffffffffffff, 2, 0xfffffffffffffffe);

    assertUnsignedMultiplyWithOverflow(2, limits::max());
    assertUnsignedMultiplyWithOverflow(limits::max(), limits::max());
    assertUnsignedMultiplyWithOverflow(1LL << 62, 8);
    assertUnsignedMultiplyWithOverflow(0x7fffffffffffffff, 4);
}

TEST(OverflowArithmetic, SignedAdditionTests) {
    using limits = std::numeric_limits<int64_t>;
    assertSignedAddNoOverflow(0, limits::max(), limits::max());
    assertSignedAddNoOverflow(-1, limits::max(), limits::max() - 1);
    assertSignedAddNoOverflow(1, limits::max() - 1, limits::max());
    assertSignedAddNoOverflow(0, limits::min(), limits::min());
    assertSignedAddNoOverflow(1, limits::min(), limits::min() + 1);
    assertSignedAddNoOverflow(-1, limits::min() + 1, limits::min());
    assertSignedAddNoOverflow(limits::max(), limits::min(), -1);
    assertSignedAddNoOverflow(1, 1, 2);
    assertSignedAddNoOverflow(-1, -1, -2);

    assertSignedAddWithOverflow(limits::max(), 1);
    assertSignedAddWithOverflow(limits::max(), limits::max());
    assertSignedAddWithOverflow(limits::min(), -1);
    assertSignedAddWithOverflow(limits::min(), limits::min());
}

TEST(OverflowArithmetic, UnsignedAdditionTests) {
    using limits = std::numeric_limits<uint64_t>;
    assertUnsignedAddNoOverflow(0, limits::max(), limits::max());
    assertUnsignedAddNoOverflow(1, limits::max() - 1, limits::max());
    assertUnsignedAddNoOverflow(1, 1, 2);

    assertUnsignedAddWithOverflow(limits::max(), 1);
    assertUnsignedAddWithOverflow(limits::max(), limits::max());
}

TEST(OverflowArithmetic, SignedSubtractionTests) {
    using limits = std::numeric_limits<int64_t>;
    assertSignedSubtractNoOverflow(limits::max(), 0, limits::max());
    assertSignedSubtractNoOverflow(limits::max(), 1, limits::max() - 1);
    assertSignedSubtractNoOverflow(limits::max() - 1, -1, limits::max());
    assertSignedSubtractNoOverflow(limits::min(), 0, limits::min());
    assertSignedSubtractNoOverflow(limits::min(), -1, limits::min() + 1);
    assertSignedSubtractNoOverflow(limits::min() + 1, 1, limits::min());
    assertSignedSubtractNoOverflow(limits::max(), limits::max(), 0);
    assertSignedSubtractNoOverflow(limits::min(), limits::min(), 0);
    assertSignedSubtractNoOverflow(0, 0, 0);
    assertSignedSubtractNoOverflow(1, 1, 0);
    assertSignedSubtractNoOverflow(0, 1, -1);

    assertSignedSubtractWithOverflow(0, limits::min());
    assertSignedSubtractWithOverflow(limits::max(), -1);
    assertSignedSubtractWithOverflow(limits::max(), limits::min());
    assertSignedSubtractWithOverflow(limits::min(), 1);
    assertSignedSubtractWithOverflow(limits::min(), limits::max());
}

TEST(OverflowArithmetic, UnsignedSubtractionTests) {
    using limits = std::numeric_limits<uint64_t>;
    assertUnsignedSubtractNoOverflow(limits::max(), 0, limits::max());
    assertUnsignedSubtractNoOverflow(limits::max(), 1, limits::max() - 1);
    assertUnsignedSubtractNoOverflow(limits::max(), limits::max(), 0);
    assertUnsignedSubtractNoOverflow(0, 0, 0);
    assertUnsignedSubtractNoOverflow(1, 1, 0);

    assertUnsignedSubtractWithOverflow(0, 1);
    assertUnsignedSubtractWithOverflow(0, limits::max());
}

}  // namespace
}  // namespace mongo
