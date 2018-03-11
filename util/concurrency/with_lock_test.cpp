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

#define MONGO_LOG_DEFAULT_COMPONENT ::mongo::logger::LogComponent::kDefault

#include "mongo/platform/basic.h"

#include "mongo/stdx/mutex.h"
#include "mongo/unittest/unittest.h"
#include "mongo/util/concurrency/with_lock.h"
#include "mongo/util/log.h"

#include <iostream>

namespace mongo {

namespace {

struct Beerp {
    explicit Beerp(int i) {
        _blerp(WithLock::withoutLock(), i);
    }
    Beerp(stdx::lock_guard<stdx::mutex> const& lk, int i) {
        _blerp(lk, i);
    }
    int bleep(char n) {
        stdx::lock_guard<stdx::mutex> lk(_m);
        return _bloop(lk, n - '0');
    }
    int bleep(int i) {
        stdx::unique_lock<stdx::mutex> lk(_m);
        return _bloop(lk, i);
    }

private:
    int _bloop(WithLock lk, int i) {
        return _blerp(lk, i);
    }
    int _blerp(WithLock, int i) {
        log() << i << " bleep" << (i == 1 ? "\n" : "s\n");
        return i;
    }
    stdx::mutex _m;
};

TEST(WithLockTest, OverloadSet) {
    Beerp b(0);
    ASSERT_EQ(1, b.bleep('1'));
    ASSERT_EQ(2, b.bleep(2));

    stdx::mutex m;
    stdx::lock_guard<stdx::mutex> lk(m);
    Beerp(lk, 3);
}

}  // namespace
}  // namespace mongo
