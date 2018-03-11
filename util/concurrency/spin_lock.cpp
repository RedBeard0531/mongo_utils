// spin_lock.cpp

/*    Copyright 2009 10gen Inc.
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

#if !defined(_WIN32)

#include "mongo/platform/basic.h"

#include "mongo/platform/pause.h"
#include "mongo/util/concurrency/spin_lock.h"

#include <sched.h>
#include <time.h>


namespace mongo {


void SpinLock::_lockSlowPath() {
    /**
     * this is designed to perform close to the default spin lock
     * the reason for the mild insanity is to prevent horrible performance
     * when contention spikes
     * it allows spinlocks to be used in many more places
     * which is good because even with this change they are about 8x faster on linux
     */

    for (int i = 0; i < 1000; i++) {
        if (_tryLock())
            return;

        MONGO_YIELD_CORE_FOR_SMT();
    }

    for (int i = 0; i < 1000; i++) {
        if (_tryLock())
            return;
        sched_yield();
    }

    struct timespec t;
    t.tv_sec = 0;
    t.tv_nsec = 5000000;

    while (!_tryLock()) {
        nanosleep(&t, NULL);
    }
}

}  // namespace mongo

#endif
