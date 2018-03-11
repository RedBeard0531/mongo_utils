// spin_lock.h

/**
*    Copyright (C) 2008 10gen Inc.
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

#pragma once

#ifdef _WIN32
#include "mongo/platform/windows_basic.h"
#else
#include <atomic>
#endif

#include "mongo/base/disallow_copying.h"
#include "mongo/platform/compiler.h"
#include "mongo/stdx/mutex.h"

namespace mongo {

#if defined(_WIN32)
class SpinLock {
    MONGO_DISALLOW_COPYING(SpinLock);

public:
    SpinLock() {
        InitializeCriticalSectionAndSpinCount(&_cs, 4000);
    }

    ~SpinLock() {
        DeleteCriticalSection(&_cs);
    }

    void lock() {
        EnterCriticalSection(&_cs);
    }

    void unlock() {
        LeaveCriticalSection(&_cs);
    }

private:
    CRITICAL_SECTION _cs;
};

#else

class SpinLock {
    MONGO_DISALLOW_COPYING(SpinLock);

public:
    SpinLock() = default;

    void unlock() {
        _locked.clear(std::memory_order_release);
    }

    void lock() {
        if (MONGO_likely(_tryLock()))
            return;
        _lockSlowPath();
    }

private:
    bool _tryLock() {
        bool wasLocked = _locked.test_and_set(std::memory_order_acquire);
        return !wasLocked;
    }

    void _lockSlowPath();

    // Initializes to the cleared state.
    std::atomic_flag _locked = ATOMIC_FLAG_INIT;  // NOLINT
};
#endif

using scoped_spinlock = stdx::lock_guard<SpinLock>;

}  // namespace mongo
