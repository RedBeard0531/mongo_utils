/**
 *    Copyright (C) 2013 10gen Inc.
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

#include "mongo/stdx/functional.h"
#include "mongo/stdx/thread.h"
#include "mongo/unittest/unittest.h"
#include "mongo/util/concurrency/spin_lock.h"
#include "mongo/util/timer.h"

namespace {

using mongo::SpinLock;
using mongo::Timer;

namespace stdx = mongo::stdx;

class LockTester {
public:
    LockTester(SpinLock* spin, int* counter) : _spin(spin), _counter(counter), _requests(0) {}

    ~LockTester() {
        delete _t;
    }

    void start(int increments) {
        _t = new stdx::thread([this, increments] { test(increments); });
    }

    void join() {
        if (_t)
            _t->join();
    }

    int requests() const {
        return _requests;
    }

private:
    SpinLock* _spin;  // not owned here
    int* _counter;    // not owned here
    int _requests;
    stdx::thread* _t;

    void test(int increments) {
        while (increments-- > 0) {
            _spin->lock();
            ++(*_counter);
            ++_requests;
            _spin->unlock();
        }
    }

    LockTester(LockTester&);
    LockTester& operator=(LockTester&);
};


TEST(Concurrency, ConcurrentIncs) {
    SpinLock spin;
    int counter = 0;

    const int threads = 64;
    const int incs = 50000;
    LockTester* testers[threads];

    Timer timer;

    for (int i = 0; i < threads; i++) {
        testers[i] = new LockTester(&spin, &counter);
    }
    for (int i = 0; i < threads; i++) {
        testers[i]->start(incs);
    }
    for (int i = 0; i < threads; i++) {
        testers[i]->join();
        ASSERT_EQUALS(testers[i]->requests(), incs);
        delete testers[i];
    }

    int ms = timer.millis();
    mongo::unittest::log() << "spinlock ConcurrentIncs time: " << ms << std::endl;

    ASSERT_EQUALS(counter, threads * incs);
}

}  // namespace
