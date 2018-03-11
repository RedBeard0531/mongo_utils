/**
 * Copyright (C) 2015 MongoDB Inc.
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

#include <iostream>

#include "mongo/platform/atomic_proxy.h"
#include "mongo/unittest/unittest.h"

namespace mongo {
namespace {

template <typename AtomicProxyType>
void testAtomicProxyBasicOperations() {
    typedef typename AtomicProxyType::value_type WordType;
    AtomicProxyType w;

    ASSERT_EQUALS(WordType(0), w.load());

    w.store(1);
    ASSERT_EQUALS(WordType(1), w.load());
}


TEST(AtomicProxyTests, BasicOperationsDouble) {
    testAtomicProxyBasicOperations<AtomicDouble>();

    AtomicDouble d(3.14159);
    ASSERT_EQUALS(3.14159, d.load());
}

}  // namespace
}  // namespace mongo
