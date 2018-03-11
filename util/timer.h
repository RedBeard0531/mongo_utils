/**
 *    Copyright (C) 2018 MongoDB Inc.
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

#include "mongo/util/time_support.h"

namespace mongo {

class TickSource;

/**
 * Time tracking object.
 */
class Timer {
public:
    /**
     * Creates a timer with the system default tick source. Should not be created before global
     * initialization completes.
     */
    Timer();

    /**
     * Creates a timer using the specified tick source. Caller retains ownership of TickSource and
     * must keep it in scope until Timer goes out of scope.
     */
    explicit Timer(TickSource* tickSource);

    long long micros() const {
        return static_cast<long long>((now() - _old) * _microsPerCount);
    }
    int millis() const {
        return static_cast<int>(micros() / 1000);
    }
    int seconds() const {
        return static_cast<int>(micros() / 1000000);
    }
    int minutes() const {
        return seconds() / 60;
    }

    Microseconds elapsed() const {
        return Microseconds{micros()};
    }

    void reset() {
        _old = now();
    }

private:
    TickSource* const _tickSource;

    // Derived value from _countsPerSecond. This represents the conversion ratio
    // from clock ticks to microseconds.
    const double _microsPerCount;

    long long now() const;

    long long _old;
};

}  // namespace mongo
