/**
 *    Copyright (C) 2018 MongoDB Inc.
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

#include "mongo/unittest/unittest.h"
#include "mongo/util/icu.h"

namespace mongo {
namespace {

struct testCases {
    std::string original;
    std::string normalized;
    bool success;
};

TEST(ICUTest, saslPrep) {
    const testCases tests[] = {
        // U+0065 LATIN SMALL LETTER E + U+0301 COMBINING ACUTE ACCENT
        // U+00E9 LATIN SMALL LETTER E WITH ACUTE
        {"\x65\xCC\x81", "\xC3\xA9", true},

        // Test values from RFC4013 Section 3.
        // #1 SOFT HYPHEN mapped to nothing.
        {"I\xC2\xADX", "IX", true},
        // #2 no transformation
        {"user", "user", true},
        // #3 case preserved, will not match #2
        {"USER", "USER", true},
        // #4 output is NFKC, input in ISO 8859-1
        {"\xC2\xAA", "a", true},
        // #5 output is NFKC, will match #1
        {"\xE2\x85\xA8", "IX", true},
        // #6 Error - prohibited character
        {"\x07", "(invalid)", false},
        // #7 Error - bidirectional check
        {"\xD8\xA7\x31", "(invalid)", false},
    };

    for (const auto test : tests) {
        auto ret = saslPrep(test.original);
        ASSERT_EQ(ret.isOK(), test.success);
        if (test.success) {
            ASSERT_OK(ret);
            ASSERT_EQ(ret.getValue(), test.normalized);
        } else {
            ASSERT_NOT_OK(ret);
        }
    }
}

}  // namespace
}  // namespace mongo
