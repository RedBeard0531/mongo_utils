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

#include <string>
#include <vector>

#include "mongo/base/status_with.h"
#include "mongo/base/string_data.h"
#include "mongo/unittest/unittest.h"
#include "mongo/util/assert_util.h"

namespace {

using mongo::makeStatusWith;
using mongo::StatusWith;

TEST(StatusWith, makeStatusWith) {
    using mongo::StringData;

    auto s1 = makeStatusWith<int>(3);
    ASSERT_TRUE(s1.isOK());
    ASSERT_EQUALS(uassertStatusOK(s1), 3);

    auto s2 = makeStatusWith<std::vector<int>>();
    ASSERT_TRUE(s2.isOK());
    ASSERT_EQUALS(uassertStatusOK(s2).size(), 0u);

    std::vector<int> i = {1, 2, 3};
    auto s3 = makeStatusWith<std::vector<int>>(i.begin(), i.end());
    ASSERT_TRUE(s3.isOK());
    ASSERT_EQUALS(uassertStatusOK(s3).size(), 3u);

    auto s4 = makeStatusWith<std::string>("foo");

    ASSERT_TRUE(s4.isOK());
    ASSERT_EQUALS(uassertStatusOK(s4), std::string{"foo"});
    const char* foo = "barbaz";
    auto s5 = makeStatusWith<StringData>(foo, std::size_t{6});
    ASSERT_TRUE(s5.isOK());

    // make sure CV qualifiers trigger correct overload
    const StatusWith<StringData>& s6 = s5;
    ASSERT_EQUALS(uassertStatusOK(s6), foo);
    StatusWith<StringData>& s7 = s5;
    ASSERT_EQUALS(uassertStatusOK(s7), foo);
    ASSERT_EQUALS(uassertStatusOK(std::move(s5)), foo);

    // Check that we use T(...) and not T{...}
    // ASSERT_EQUALS requires an ostream overload for vector<int>
    ASSERT_TRUE(makeStatusWith<std::vector<int>>(1, 2) == std::vector<int>{2});
}

TEST(StatusWith, nonDefaultConstructible) {
    class NoDefault {
        NoDefault() = delete;

    public:
        NoDefault(int x) : _x{x} {};
        int _x{0};
    };

    auto swND = makeStatusWith<NoDefault>(1);
    ASSERT_TRUE(swND.getValue()._x = 1);

    auto swNDerror = StatusWith<NoDefault>(mongo::ErrorCodes::BadValue, "foo");
    ASSERT_FALSE(swNDerror.isOK());
}

TEST(StatusWith, ignoreTest) {
    auto function = []() -> StatusWith<bool> { return false; };

    // Compile only test:
    function().getStatus().ignore();
}

}  // namespace
