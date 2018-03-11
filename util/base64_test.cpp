/**
 *    Copyright (C) 2017 MongoDB Inc.
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
#include "mongo/util/base64.h"

namespace mongo {
namespace {

TEST(Base64Test, transcode) {
    const struct {
        std::string plain;
        std::string encoded;
    } tests[] = {
        {"", ""},
        {"a", "YQ=="},
        {"aa", "YWE="},
        {"aaa", "YWFh"},
        {"aaaa", "YWFhYQ=="},

        {"A", "QQ=="},
        {"AA", "QUE="},
        {"AAA", "QUFB"},
        {"AAAA", "QUFBQQ=="},

        {"The quick brown fox jumped over the lazy dog.",
         "VGhlIHF1aWNrIGJyb3duIGZveCBqdW1wZWQgb3ZlciB0aGUgbGF6eSBkb2cu"},
        {std::string("\0\1\2\3\4\5\6\7", 8), "AAECAwQFBgc="},
        {std::string("\0\277\1\276\2\275", 6), "AL8BvgK9"},
    };

    for (auto const& t : tests) {
        ASSERT_TRUE(base64::validate(t.encoded));

        ASSERT_EQUALS(base64::encode(t.plain), t.encoded);
        ASSERT_EQUALS(base64::decode(t.encoded), t.plain);
    }
}

TEST(Base64Test, parseFail) {
    const struct {
        std::string encoded;
        int code;
    } tests[] = {
        {"BadLength", 10270},
        {"Has Whitespace==", 40537},
        {"Hasbadchar$=", 40537},
        {"Hasbadchar\xFF=", 40537},
        {"Hasbadcahr\t=", 40537},
        {"too=soon", 40538},
    };

    for (auto const& t : tests) {
        ASSERT_FALSE(base64::validate(t.encoded));

        try {
            base64::decode(t.encoded);
            ASSERT_TRUE(false);
        } catch (const AssertionException& e) {
            ASSERT_EQ(e.code(), t.code);
        }
    }
}

}  // namespace
}  // namespace mongo
