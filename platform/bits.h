// bits.h

/*    Copyright 2012 10gen Inc.
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

#if defined(_MSC_VER)
#include <intrin.h>
#endif

namespace mongo {

/**
 * Returns the number of leading 0-bits in num. Returns 64 if num is 0.
 */
inline int countLeadingZeros64(unsigned long long num);

/**
 * Returns the number of trailing 0-bits in num. Returns 64 if num is 0.
 */
inline int countTrailingZeros64(unsigned long long num);


#if defined(__GNUC__)
int countLeadingZeros64(unsigned long long num) {
    if (num == 0)
        return 64;
    return __builtin_clzll(num);
}

int countTrailingZeros64(unsigned long long num) {
    if (num == 0)
        return 64;
    return __builtin_ctzll(num);
}
#elif defined(_MSC_VER) && defined(_WIN64)
int countLeadingZeros64(unsigned long long num) {
    unsigned long out;
    if (_BitScanReverse64(&out, num))
        return 63 ^ out;
    return 64;
}

int countTrailingZeros64(unsigned long long num) {
    unsigned long out;
    if (_BitScanForward64(&out, num))
        return out;
    return 64;
}
#elif defined(_MSC_VER) && defined(_WIN32)
int countLeadingZeros64(unsigned long long num) {
    unsigned long out;
    if (_BitScanReverse(&out, static_cast<unsigned long>(num >> 32)))
        return 31 ^ out;
    if (_BitScanReverse(&out, static_cast<unsigned long>(num)))
        return 63 ^ out;
    return 64;
}

int countTrailingZeros64(unsigned long long num) {
    unsigned long out;
    if (_BitScanForward(&out, static_cast<unsigned long>(num)))
        return out;
    if (_BitScanForward(&out, static_cast<unsigned long>(num >> 32)))
        return out + 32;
    return 64;
}
#else
#error "No bit-ops definitions for your platform"
#endif
}
