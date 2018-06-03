// checked_cast.h

/**
 *    Copyright (C) 2015 MongoDB Inc.
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

#include <memory>

#include "mongo/util/assert_util.h"
#include "mongo/util/debug_util.h"

namespace mongo {

/**
 * Similar to static_cast, but in debug builds uses RTTI to confirm that the cast
 * is legal at runtime.
 */
template <bool>
struct checked_cast_impl;

template <>
struct checked_cast_impl<false> {
    template <typename T, typename U>
    static T cast(const U& u) {
        return static_cast<T>(u);
    }

    template <typename T, typename U>
    static T cast(U& u) {
        return static_cast<T>(u);
    }
};

template <>
struct checked_cast_impl<true> {
    template <typename T, typename U>
    static T cast(U* u) {
        if (!u) {
            return NULL;
        }
        T t = dynamic_cast<T>(u);
        invariant(t);
        return t;
    }

    template <typename T, typename U>
    static T cast(const U& u) {
        return dynamic_cast<T>(u);
    }

    template <typename T, typename U>
    static T cast(U& u) {
        return dynamic_cast<T>(u);
    }
};

template <typename T, typename U>
T checked_cast(const U& u) {
    return checked_cast_impl<kDebugBuild>::cast<T>(u);
};

template <typename T, typename U>
T checked_cast(U& u) {
    return checked_cast_impl<kDebugBuild>::cast<T>(u);
};

/**
 * Similar to static_pointer_cast, but in debug builds uses RTTI to confirm that the cast
 * is legal at runtime.
 */
template <bool>
struct checked_pointer_cast_impl;

template <>
struct checked_pointer_cast_impl<false> {
    template <typename T, typename U>
    static std::shared_ptr<T> cast(const std::shared_ptr<U>& u) {
        return std::static_pointer_cast<T>(u);
    }
};

template <>
struct checked_pointer_cast_impl<true> {
    template <typename T, typename U>
    static std::shared_ptr<T> cast(const std::shared_ptr<U>& u) {
        if (!u) {
            return nullptr;
        }

        std::shared_ptr<T> t = std::dynamic_pointer_cast<T>(u);
        invariant(t);

        return t;
    }
};

template <typename T, typename U>
std::shared_ptr<T> checked_pointer_cast(const std::shared_ptr<U>& u) {
    return checked_pointer_cast_impl<kDebugBuild>::cast<T>(u);
};

}  // namespace mongo
