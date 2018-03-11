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

#include <algorithm>
#include <string>
#include <vector>

#include "mongo/base/simple_string_data_comparator.h"
#include "mongo/base/string_data.h"
#include "mongo/unittest/unittest.h"

namespace mongo {

using std::string;

TEST(Construction, Empty) {
    StringData strData;
    ASSERT_EQUALS(strData.size(), 0U);
    ASSERT_TRUE(strData.rawData() == NULL);
}

TEST(Construction, FromStdString) {
    std::string base("aaa");
    StringData strData(base);
    ASSERT_EQUALS(strData.size(), base.size());
    ASSERT_EQUALS(strData.toString(), base);
}

TEST(Construction, FromCString) {
    std::string base("aaa");
    StringData strData(base.c_str());
    ASSERT_EQUALS(strData.size(), base.size());
    ASSERT_EQUALS(strData.toString(), base);
}

TEST(Construction, FromNullCString) {
    char* c = NULL;
    StringData strData(c);
    ASSERT_EQUALS(strData.size(), 0U);
    ASSERT_TRUE(strData.rawData() == NULL);
}

TEST(Construction, FromUserDefinedLiteral) {
    const auto strData = "cc\0c"_sd;
    ASSERT_EQUALS(strData.size(), 4U);
    ASSERT_EQUALS(strData.toString(), string("cc\0c", 4));
}

TEST(Construction, FromUserDefinedRawLiteral) {
    const auto strData = R"("")"_sd;
    ASSERT_EQUALS(strData.size(), 2U);
    ASSERT_EQUALS(strData.toString(), string("\"\"", 2));
}

TEST(Construction, FromEmptyUserDefinedLiteral) {
    const auto strData = ""_sd;
    ASSERT_EQUALS(strData.size(), 0U);
    ASSERT_EQUALS(strData.toString(), string(""));
}

TEST(Comparison, BothEmpty) {
    StringData empty("");
    ASSERT_TRUE(empty == empty);
    ASSERT_FALSE(empty != empty);
    ASSERT_FALSE(empty > empty);
    ASSERT_TRUE(empty >= empty);
    ASSERT_FALSE(empty < empty);
    ASSERT_TRUE(empty <= empty);
}

TEST(Comparison, BothNonEmptyOnSize) {
    StringData a("a");
    StringData aa("aa");
    ASSERT_FALSE(a == aa);
    ASSERT_TRUE(a != aa);
    ASSERT_FALSE(a > aa);
    ASSERT_FALSE(a >= aa);
    ASSERT_TRUE(a >= a);
    ASSERT_TRUE(a < aa);
    ASSERT_TRUE(a <= aa);
    ASSERT_TRUE(a <= a);
}

TEST(Comparison, BothNonEmptyOnContent) {
    StringData a("a");
    StringData b("b");
    ASSERT_FALSE(a == b);
    ASSERT_TRUE(a != b);
    ASSERT_FALSE(a > b);
    ASSERT_FALSE(a >= b);
    ASSERT_TRUE(a < b);
    ASSERT_TRUE(a <= b);
}

TEST(Comparison, MixedEmptyAndNot) {
    StringData empty("");
    StringData a("a");
    ASSERT_FALSE(a == empty);
    ASSERT_TRUE(a != empty);
    ASSERT_TRUE(a > empty);
    ASSERT_TRUE(a >= empty);
    ASSERT_FALSE(a < empty);
    ASSERT_FALSE(a <= empty);
}

TEST(Find, Char1) {
    ASSERT_EQUALS(string::npos, StringData("foo").find('a'));
    ASSERT_EQUALS(0U, StringData("foo").find('f'));
    ASSERT_EQUALS(1U, StringData("foo").find('o'));
}

TEST(Find, Str1) {
    ASSERT_EQUALS(string::npos, StringData("foo").find("asdsadasda"));
    ASSERT_EQUALS(string::npos, StringData("foo").find("a"));
    ASSERT_EQUALS(string::npos, StringData("foo").find("food"));
    ASSERT_EQUALS(string::npos, StringData("foo").find("ooo"));

    ASSERT_EQUALS(0U, StringData("foo").find("f"));
    ASSERT_EQUALS(0U, StringData("foo").find("fo"));
    ASSERT_EQUALS(0U, StringData("foo").find("foo"));
    ASSERT_EQUALS(1U, StringData("foo").find("o"));
    ASSERT_EQUALS(1U, StringData("foo").find("oo"));

    ASSERT_EQUALS(string("foo").find(""), StringData("foo").find(""));
}

// Helper function for Test(Hasher, Str1)
template <int SizeofSizeT>
void SDHasher_check(void);

template <>
void SDHasher_check<4>(void) {
    const auto& strCmp = SimpleStringDataComparator::kInstance;
    ASSERT_EQUALS(strCmp.hash(""), static_cast<size_t>(0));
    ASSERT_EQUALS(strCmp.hash("foo"), static_cast<size_t>(4138058784ULL));
    ASSERT_EQUALS(strCmp.hash("pizza"), static_cast<size_t>(3587803311ULL));
    ASSERT_EQUALS(strCmp.hash("mongo"), static_cast<size_t>(3724335885ULL));
    ASSERT_EQUALS(strCmp.hash("murmur"), static_cast<size_t>(1945310157ULL));
}

template <>
void SDHasher_check<8>(void) {
    const auto& strCmp = SimpleStringDataComparator::kInstance;
    ASSERT_EQUALS(strCmp.hash(""), static_cast<size_t>(0));
    ASSERT_EQUALS(strCmp.hash("foo"), static_cast<size_t>(16316970633193145697ULL));
    ASSERT_EQUALS(strCmp.hash("pizza"), static_cast<size_t>(12165495155477134356ULL));
    ASSERT_EQUALS(strCmp.hash("mongo"), static_cast<size_t>(2861051452199491487ULL));
    ASSERT_EQUALS(strCmp.hash("murmur"), static_cast<size_t>(18237957392784716687ULL));
}

TEST(Hasher, Str1) {
    SDHasher_check<sizeof(size_t)>();
}

TEST(Rfind, Char1) {
    ASSERT_EQUALS(string::npos, StringData("foo").rfind('a'));

    ASSERT_EQUALS(0U, StringData("foo").rfind('f'));
    ASSERT_EQUALS(0U, StringData("foo").rfind('f', 3));
    ASSERT_EQUALS(0U, StringData("foo").rfind('f', 2));
    ASSERT_EQUALS(0U, StringData("foo").rfind('f', 1));
    ASSERT_EQUALS(string::npos, StringData("foo", 0).rfind('f'));

    ASSERT_EQUALS(2U, StringData("foo").rfind('o'));
    ASSERT_EQUALS(2U, StringData("foo", 3).rfind('o'));
    ASSERT_EQUALS(1U, StringData("foo", 2).rfind('o'));
    ASSERT_EQUALS(string::npos, StringData("foo", 1).rfind('o'));
    ASSERT_EQUALS(string::npos, StringData("foo", 0).rfind('o'));
}

// this is to verify we match std::string
void SUBSTR_TEST_HELP(StringData big, StringData small, size_t start, size_t len) {
    ASSERT_EQUALS(small.toString(), big.toString().substr(start, len));
    ASSERT_EQUALS(small, StringData(big).substr(start, len));
}
void SUBSTR_TEST_HELP(StringData big, StringData small, size_t start) {
    ASSERT_EQUALS(small.toString(), big.toString().substr(start));
    ASSERT_EQUALS(small, StringData(big).substr(start));
}

// [12] is number of args to substr
#define SUBSTR_1_TEST_HELP(big, small, start)                                              \
    ASSERT_EQUALS(StringData(small).toString(), StringData(big).toString().substr(start)); \
    ASSERT_EQUALS(StringData(small), StringData(big).substr(start));

#define SUBSTR_2_TEST_HELP(big, small, start, len)                                              \
    ASSERT_EQUALS(StringData(small).toString(), StringData(big).toString().substr(start, len)); \
    ASSERT_EQUALS(StringData(small), StringData(big).substr(start, len));

TEST(Substr, Simple1) {
    SUBSTR_1_TEST_HELP("abcde", "abcde", 0);
    SUBSTR_2_TEST_HELP("abcde", "abcde", 0, 10);
    SUBSTR_2_TEST_HELP("abcde", "abcde", 0, 5);
    SUBSTR_2_TEST_HELP("abcde", "abc", 0, 3);
    SUBSTR_1_TEST_HELP("abcde", "cde", 2);
    SUBSTR_2_TEST_HELP("abcde", "cde", 2, 5);
    SUBSTR_2_TEST_HELP("abcde", "cde", 2, 3);
    SUBSTR_2_TEST_HELP("abcde", "cd", 2, 2);
    SUBSTR_2_TEST_HELP("abcde", "cd", 2, 2);
    SUBSTR_1_TEST_HELP("abcde", "", 5);
    SUBSTR_2_TEST_HELP("abcde", "", 5, 0);
    SUBSTR_2_TEST_HELP("abcde", "", 5, 10);

    // make sure we don't blow past the end of the StringData
    SUBSTR_1_TEST_HELP(StringData("abcdeXXX", 5), "abcde", 0);
    SUBSTR_2_TEST_HELP(StringData("abcdeXXX", 5), "abcde", 0, 10);
    SUBSTR_1_TEST_HELP(StringData("abcdeXXX", 5), "de", 3);
    SUBSTR_2_TEST_HELP(StringData("abcdeXXX", 5), "de", 3, 7);
    SUBSTR_1_TEST_HELP(StringData("abcdeXXX", 5), "", 5);
    SUBSTR_2_TEST_HELP(StringData("abcdeXXX", 5), "", 5, 1);
}

TEST(equalCaseInsensitiveTest, Simple1) {
    ASSERT(StringData("abc").equalCaseInsensitive("abc"));
    ASSERT(StringData("abc").equalCaseInsensitive("ABC"));
    ASSERT(StringData("ABC").equalCaseInsensitive("abc"));
    ASSERT(StringData("ABC").equalCaseInsensitive("ABC"));
    ASSERT(StringData("ABC").equalCaseInsensitive("AbC"));
    ASSERT(!StringData("ABC").equalCaseInsensitive("AbCd"));
    ASSERT(!StringData("ABC").equalCaseInsensitive("AdC"));
}

TEST(StartsWith, Simple) {
    ASSERT(StringData("").startsWith(""));
    ASSERT(!StringData("").startsWith("x"));
    ASSERT(StringData("abcde").startsWith(""));
    ASSERT(StringData("abcde").startsWith("a"));
    ASSERT(StringData("abcde").startsWith("ab"));
    ASSERT(StringData("abcde").startsWith("abc"));
    ASSERT(StringData("abcde").startsWith("abcd"));
    ASSERT(StringData("abcde").startsWith("abcde"));
    ASSERT(!StringData("abcde").startsWith("abcdef"));
    ASSERT(!StringData("abcde").startsWith("abdce"));
    ASSERT(StringData("abcde").startsWith(StringData("abcdeXXXX").substr(0, 4)));
    ASSERT(!StringData("abcde").startsWith(StringData("abdef").substr(0, 4)));
    ASSERT(!StringData("abcde").substr(0, 3).startsWith("abcd"));
}

TEST(EndsWith, Simple) {
    // ASSERT(StringData("").endsWith(""));
    ASSERT(!StringData("").endsWith("x"));
    // ASSERT(StringData("abcde").endsWith(""));
    ASSERT(StringData("abcde").endsWith(StringData("e", 0)));
    ASSERT(StringData("abcde").endsWith("e"));
    ASSERT(StringData("abcde").endsWith("de"));
    ASSERT(StringData("abcde").endsWith("cde"));
    ASSERT(StringData("abcde").endsWith("bcde"));
    ASSERT(StringData("abcde").endsWith("abcde"));
    ASSERT(!StringData("abcde").endsWith("0abcde"));
    ASSERT(!StringData("abcde").endsWith("abdce"));
    ASSERT(StringData("abcde").endsWith(StringData("bcdef").substr(0, 4)));
    ASSERT(!StringData("abcde").endsWith(StringData("bcde", 3)));
    ASSERT(!StringData("abcde").substr(0, 3).endsWith("cde"));
}

TEST(ConstIterator, StdCopy) {
    std::vector<char> chars;
    auto data = "This is some raw data."_sd;

    chars.resize(data.size());
    std::copy(data.begin(), data.end(), chars.begin());

    for (size_t i = 0; i < data.size(); ++i) {
        ASSERT_EQUALS(data[i], chars[i]);
    }
}

TEST(ConstIterator, StdReverseCopy) {
    std::vector<char> chars;
    auto data = "This is some raw data."_sd;

    chars.resize(data.size());
    std::reverse_copy(data.begin(), data.end(), chars.begin());

    const char rawDataExpected[] = ".atad war emos si sihT";

    for (size_t i = 0; i < data.size(); ++i) {
        ASSERT_EQUALS(rawDataExpected[i], chars[i]);
    }
}

TEST(ConstIterator, StdReplaceCopy) {
    std::vector<char> chars;
    auto data = "This is some raw data."_sd;

    chars.resize(data.size());
    std::replace_copy(data.begin(), data.end(), chars.begin(), ' ', '_');

    const char rawDataExpected[] = "This_is_some_raw_data.";

    for (size_t i = 0; i < data.size(); ++i) {
        ASSERT_EQUALS(rawDataExpected[i], chars[i]);
    }
}

}  // namespace mongo
