// @file mutex.h

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

#pragma once

#ifdef _WIN32
#include "mongo/platform/windows_basic.h"
#else
#include <pthread.h>
#endif

#include "mongo/base/disallow_copying.h"
#include "mongo/util/assert_util.h"

namespace mongo {

/** The concept with SimpleMutex is that it is a basic lock/unlock
 *  with no special functionality (such as try and try
 *  timeout).  Thus it can be implemented using OS-specific
 *  facilities in all environments (if desired).  On Windows,
 *  the implementation below is faster than boost mutex.
*/
#if defined(_WIN32)

class SimpleMutex {
    MONGO_DISALLOW_COPYING(SimpleMutex);

public:
    SimpleMutex() {
        InitializeCriticalSection(&_cs);
    }

    ~SimpleMutex() {
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

class SimpleMutex {
    MONGO_DISALLOW_COPYING(SimpleMutex);

public:
    SimpleMutex() {
        verify(pthread_mutex_init(&_lock, 0) == 0);
    }

    ~SimpleMutex() {
        verify(pthread_mutex_destroy(&_lock) == 0);
    }

    void lock() {
        verify(pthread_mutex_lock(&_lock) == 0);
    }

    void unlock() {
        verify(pthread_mutex_unlock(&_lock) == 0);
    }

private:
    pthread_mutex_t _lock;
};
#endif

}  // namespace mongo
