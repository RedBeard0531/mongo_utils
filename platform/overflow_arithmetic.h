/*    Copyright 2016 MongoDB, Inc.
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

#ifdef _MSC_VER
#include <safeint.h>
#endif

namespace mongo {

/**
 * Returns true if multiplying lhs by rhs would overflow. Otherwise, multiplies 64-bit signed
 * or unsigned integers lhs by rhs and stores the result in *product.
 */
inline bool mongoSignedMultiplyOverflow64(int64_t lhs, int64_t rhs, int64_t* product);
inline bool mongoUnsignedMultiplyOverflow64(uint64_t lhs, uint64_t rhs, uint64_t* product);

/**
 * Returns true if adding lhs and rhs would overflow. Otherwise, adds 64-bit signed or unsigned
 * integers lhs and rhs and stores the result in *sum.
 */
inline bool mongoSignedAddOverflow64(int64_t lhs, int64_t rhs, int64_t* sum);
inline bool mongoUnsignedAddOverflow64(uint64_t lhs, uint64_t rhs, uint64_t* sum);

/**
 * Returns true if subtracting rhs from lhs would overflow. Otherwise, subtracts 64-bit signed or
 * unsigned integers rhs from lhs and stores the result in *difference.
 */
inline bool mongoSignedSubtractOverflow64(int64_t lhs, int64_t rhs, int64_t* difference);
inline bool mongoUnsignedSubtractOverflow64(uint64_t lhs, uint64_t rhs, uint64_t* difference);


#ifdef _MSC_VER

// The microsoft SafeInt functions return true on success, false on overflow. Unfortunately the
// MSVC2015 version contains a typo that prevents the signed variants from compiling in our
// environment.
// TODO The typo was fixed in MSVC2017 so we should try again after we upgrade our toolchain.

inline bool mongoSignedMultiplyOverflow64(int64_t lhs, int64_t rhs, int64_t* product) {
    int64_t hi;
    *product = _mul128(lhs, rhs, &hi);
    if (hi == 0) {
        return *product < 0;
    }
    if (hi == -1) {
        return *product >= 0;
    }
    return true;
}

inline bool mongoUnsignedMultiplyOverflow64(uint64_t lhs, uint64_t rhs, uint64_t* product) {
    return !msl::utilities::SafeMultiply(lhs, rhs, *product);
}

inline bool mongoSignedAddOverflow64(int64_t lhs, int64_t rhs, int64_t* sum) {
    *sum = static_cast<int64_t>(static_cast<uint64_t>(lhs) + static_cast<uint64_t>(rhs));
    if (lhs >= 0 && rhs >= 0) {
        return (*sum) < 0;
    }
    if (lhs < 0 && rhs < 0) {
        return (*sum) >= 0;
    }
    return false;
}

inline bool mongoUnsignedAddOverflow64(uint64_t lhs, uint64_t rhs, uint64_t* sum) {
    return !msl::utilities::SafeAdd(lhs, rhs, *sum);
}

inline bool mongoSignedSubtractOverflow64(int64_t lhs, int64_t rhs, int64_t* difference) {
    *difference = static_cast<int64_t>(static_cast<uint64_t>(lhs) - static_cast<uint64_t>(rhs));
    if (lhs >= 0 && rhs < 0) {
        return (*difference) < 0;
    }
    if (lhs < 0 && rhs >= 0) {
        return (*difference >= 0);
    }
    return false;
}

inline bool mongoUnsignedSubtractOverflow64(uint64_t lhs, uint64_t rhs, uint64_t* difference) {
    return !msl::utilities::SafeSubtract(lhs, rhs, *difference);
}

#else

// On GCC and CLANG we can use __builtin functions to perform these calculations. These return true
// on overflow and false on success.

inline bool mongoSignedMultiplyOverflow64(long lhs, long rhs, long* product) {
    return __builtin_mul_overflow(lhs, rhs, product);
}

inline bool mongoSignedMultiplyOverflow64(long long lhs, long long rhs, long long* product) {
    return __builtin_mul_overflow(lhs, rhs, product);
}

inline bool mongoUnsignedMultiplyOverflow64(unsigned long lhs,
                                            unsigned long rhs,
                                            unsigned long* product) {
    return __builtin_mul_overflow(lhs, rhs, product);
}

inline bool mongoUnsignedMultiplyOverflow64(unsigned long long lhs,
                                            unsigned long long rhs,
                                            unsigned long long* product) {
    return __builtin_mul_overflow(lhs, rhs, product);
}

inline bool mongoSignedAddOverflow64(long lhs, long rhs, long* sum) {
    return __builtin_add_overflow(lhs, rhs, sum);
}

inline bool mongoSignedAddOverflow64(long long lhs, long long rhs, long long* sum) {
    return __builtin_add_overflow(lhs, rhs, sum);
}

inline bool mongoUnsignedAddOverflow64(unsigned long lhs, unsigned long rhs, unsigned long* sum) {
    return __builtin_add_overflow(lhs, rhs, sum);
}

inline bool mongoUnsignedAddOverflow64(unsigned long long lhs,
                                       unsigned long long rhs,
                                       unsigned long long* sum) {
    return __builtin_add_overflow(lhs, rhs, sum);
}

inline bool mongoSignedSubtractOverflow64(long lhs, long rhs, long* difference) {
    return __builtin_sub_overflow(lhs, rhs, difference);
}

inline bool mongoSignedSubtractOverflow64(long long lhs, long long rhs, long long* difference) {
    return __builtin_sub_overflow(lhs, rhs, difference);
}

inline bool mongoUnsignedSubtractOverflow64(unsigned long lhs,
                                            unsigned long rhs,
                                            unsigned long* sum) {
    return __builtin_sub_overflow(lhs, rhs, sum);
}

inline bool mongoUnsignedSubtractOverflow64(unsigned long long lhs,
                                            unsigned long long rhs,
                                            unsigned long long* sum) {
    return __builtin_sub_overflow(lhs, rhs, sum);
}

#endif

}  // namespace mongo
