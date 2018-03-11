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

#include "mongo/base/secure_allocator.h"

#include <array>

#include "mongo/unittest/unittest.h"

namespace mongo {

TEST(SecureAllocator, SecureVector) {
    SecureAllocatorDefaultDomain::SecureVector<int> vec;

    vec->push_back(1);
    vec->push_back(2);

    ASSERT_EQUALS(1, (*vec)[0]);
    ASSERT_EQUALS(2, (*vec)[1]);

    vec->resize(2000, 3);
    ASSERT_EQUALS(3, (*vec)[2]);
}

TEST(SecureAllocator, SecureString) {
    SecureAllocatorDefaultDomain::SecureString str;

    str->resize(2000, 'x');
    ASSERT_EQUALS(0, str->compare(*SecureAllocatorDefaultDomain::SecureString(2000, 'x')));

    SecureAllocatorDefaultDomain::SecureString str2(str);
    ASSERT_NOT_EQUALS(&*str, &*str2);
    str2 = str;
    ASSERT_NOT_EQUALS(&*str, &*str2);

    auto strPtr = &*str;
    auto str2Ptr = &*str2;
    SecureAllocatorDefaultDomain::SecureString str3(std::move(str));
    ASSERT_EQUALS(strPtr, &*str3);
    str3 = std::move(str2);
    ASSERT_EQUALS(str2Ptr, &*str3);
}

// Verify that we can make a good number of secure objects.  Under the initial secure allocator
// design (page per object), you couldn't make more than 8-50 objects before running out of lockable
// pages.
TEST(SecureAllocator, ManySecureBytes) {
    std::array<SecureAllocatorDefaultDomain::SecureHandle<char>, 4096> chars;
    std::vector<SecureAllocatorDefaultDomain::SecureHandle<char>> e_chars(4096, 'e');
}

TEST(SecureAllocator, NonDefaultConstructibleWorks) {
    struct Foo {
        Foo(int) {}
        Foo() = delete;
    };

    SecureAllocatorDefaultDomain::SecureHandle<Foo> foo(10);
}

TEST(SecureAllocator, allocatorCanBeDisabled) {
    static size_t pegInvokationCountLast;
    static size_t pegInvokationCount;
    pegInvokationCountLast = 0;
    pegInvokationCount = 0;
    struct UnsecureAllocatorTrait {
        static bool peg() {
            pegInvokationCount++;

            return false;
        }
    };
    using UnsecureAllocatorDomain = SecureAllocatorDomain<UnsecureAllocatorTrait>;

    {
        std::vector<UnsecureAllocatorDomain::SecureHandle<char>> more_e_chars(4096, 'e');
        ASSERT_GT(pegInvokationCount, pegInvokationCountLast);
        pegInvokationCountLast = pegInvokationCount;

        UnsecureAllocatorDomain::SecureString str;
        ASSERT_GT(pegInvokationCount, pegInvokationCountLast);
        pegInvokationCountLast = pegInvokationCount;

        str->resize(2000, 'x');
        ASSERT_GT(pegInvokationCount, pegInvokationCountLast);
        pegInvokationCountLast = pegInvokationCount;

        ASSERT_EQUALS(0, str->compare(*UnsecureAllocatorDomain::SecureString(2000, 'x')));
        ASSERT_GT(pegInvokationCount, pegInvokationCountLast);
        pegInvokationCountLast = pegInvokationCount;
    }

    ASSERT_GT(pegInvokationCount, pegInvokationCountLast);
}

}  // namespace mongo
