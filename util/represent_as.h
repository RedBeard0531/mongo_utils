/**
 *    Copyright (C) 2016 MongoDB Inc.
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

#include <cmath>
#include <limits>
#include <type_traits>

#include <boost/optional.hpp>

#include "mongo/base/static_assert.h"
#include "mongo/stdx/type_traits.h"

namespace mongo {

namespace detail {

/**
 * The following three methods are conversion helpers that allow us to promote
 * all numerical input to three top-level types: int64_t, uint64_t, and double.
 */

// Floating point numbers -> double
template <typename T>
typename stdx::enable_if_t<std::is_floating_point<T>::value, double> upconvert(T t) {
    MONGO_STATIC_ASSERT(sizeof(double) >= sizeof(T));
    return static_cast<double>(t);
}

// Signed integral types -> int64_t
template <typename T>
typename stdx::enable_if_t<std::is_integral<T>::value && std::is_signed<T>::value, int64_t>
upconvert(T t) {
    MONGO_STATIC_ASSERT(sizeof(int64_t) >= sizeof(T));
    return static_cast<int64_t>(t);
}

// Unsigned integral types -> uint64_t
template <typename T>
typename stdx::enable_if_t<std::is_integral<T>::value && !std::is_signed<T>::value, uint64_t>
upconvert(T t) {
    MONGO_STATIC_ASSERT(sizeof(uint64_t) >= sizeof(T));
    return static_cast<uint64_t>(t);
}

/**
 * Compare two values of the same type. Return -1 if a < b, 0 if they are equal, and
 * 1 if a > b.
 */
template <typename T>
int identityCompare(T a, T b) {
    if (a == b) {
        return 0;
    }
    return (a < b) ? -1 : 1;
}

inline int signedCompare(int64_t a, int64_t b) {
    return identityCompare(a, b);
}

inline int signedCompare(double a, double b) {
    return identityCompare(a, b);
}

inline int signedCompare(uint64_t a, uint64_t b) {
    return identityCompare(a, b);
}

/**
 * Compare unsigned and signed integers.
 */
inline int signedCompare(int64_t a, uint64_t b) {
    if (a < 0) {
        return -1;
    }

    auto aUnsigned = static_cast<uint64_t>(a);
    return signedCompare(aUnsigned, b);
}

inline int signedCompare(uint64_t a, int64_t b) {
    return -signedCompare(b, a);
}

/**
 * Compare doubles and signed integers.
 */
inline int signedCompare(double a, int64_t b) {
    // Casting int64_ts to doubles will round them
    // and give the wrong result, so convert doubles to
    // int64_ts if we can, then do the comparison.
    if (a < -std::ldexp(1, 63)) {
        return -1;
    } else if (a >= std::ldexp(1, 63)) {
        return 1;
    }

    auto aAsInt64 = static_cast<int64_t>(a);
    return signedCompare(aAsInt64, b);
}

inline int signedCompare(int64_t a, double b) {
    return -signedCompare(b, a);
}

/**
 * Compare doubles and unsigned integers.
 */
inline int signedCompare(double a, uint64_t b) {
    if (a < 0) {
        return -1;
    }

    // Casting uint64_ts to doubles will round them
    // and give the wrong result, so convert doubles to
    // uint64_ts if we can, then do the comparison.
    if (a >= std::ldexp(1, 64)) {
        return 1;
    }

    auto aAsUInt64 = static_cast<uint64_t>(a);
    return signedCompare(aAsUInt64, b);
}

inline int signedCompare(uint64_t a, double b) {
    return -signedCompare(b, a);
}

/**
 * For any t and u of types T and U, promote t and u to one of the
 * top-level numerical types (int64_t, uint64_t, and double) and
 * compare them.
 *
 * Return -1 if t < u, 0 if they are equal, 1 if t > u.
 */
template <typename T, typename U>
int compare(T t, U u) {
    return signedCompare(upconvert(t), upconvert(u));
}

}  // namespace detail

/**
 * Given a number of some type Input and a desired numerical type Output,
 * this method represents the input number in the output type if possible.
 * If the given number cannot be exactly represented in the output type,
 * this method returns a disengaged optional.
 *
 * ex:
 *     auto v1 = representAs<int>(2147483647); // v1 holds 2147483647
 *     auto v2 = representAs<int>(2147483648); // v2 is disengaged
 *     auto v3 = representAs<int>(10.3);       // v3 is disengaged
 */
template <typename Output, typename Input>
boost::optional<Output> representAs(Input number) {
    if (std::is_same<Input, Output>::value) {
        return {static_cast<Output>(number)};
    }

    // If number is NaN and Output can also represent NaN, return NaN
    // Note: We need to specifically handle NaN here because of the way
    // detail::compare is implemented.
    {
        // We use ADL here to allow types, such as Decimal, to supply their
        // own definitions of isnan(). If the Input type does not define a
        // custom isnan(), then we fall back to using std::isnan().
        using std::isnan;
        if (std::is_floating_point<Input>::value && isnan(number)) {
            if (std::is_floating_point<Output>::value) {
                return {static_cast<Output>(number)};
            }
        }
    }

    // If Output is integral and number is a non-integral floating point value,
    // return a disengaged optional.
    if (std::is_floating_point<Input>::value && std::is_integral<Output>::value) {
        if (!(std::trunc(number) == number)) {
            return {};
        }
    }

    const auto floor = std::numeric_limits<Output>::lowest();
    const auto ceiling = std::numeric_limits<Output>::max();

    // If number is out-of-bounds for Output type, fail.
    if ((detail::compare(number, floor) < 0) || (detail::compare(number, ceiling) > 0)) {
        return {};
    }

    // Our number is within bounds, safe to perform a static_cast.
    auto numberOut = static_cast<Output>(number);

    // Some integers cannot be exactly represented as floating point numbers.
    // To check, we cast back to the input type if we can, and compare.
    if (std::is_integral<Input>::value && std::is_floating_point<Output>::value) {
        const auto inputFloor = std::numeric_limits<Input>::lowest();
        const auto inputCeiling = std::numeric_limits<Input>::max();

        // If it is not safe to cast back to the Input type, fail.
        if ((detail::compare(numberOut, inputFloor) < 0) ||
            (detail::compare(numberOut, inputCeiling) > 0)) {
            return {};
        }

        if (number != static_cast<Input>(numberOut)) {
            return {};
        }
    }

    return {static_cast<Output>(numberOut)};
}

}  // namespace mongo
