// counter.h

/**
*    Copyright (C) 2008-2012 10gen Inc.
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

#include <cstdint>

#include "mongo/platform/atomic_word.h"

namespace mongo {
/**
 * A 64bit (atomic) counter.
 *
 * The constructor allows setting the start value, and increment([int]) is used to change it.
 *
 * The value can be returned using get() or the (long long) function operator.
 */
class Counter64 {
public:
    /** Atomically increment. */
    void increment(uint64_t n = 1) {
        _counter.addAndFetch(n);
    }

    /** Atomically decrement. */
    void decrement(uint64_t n = 1) {
        _counter.subtractAndFetch(n);
    }

    /** Return the current value */
    long long get() const {
        return _counter.load();
    }

    operator long long() const {
        return get();
    }

private:
    AtomicInt64 _counter;
};
}
