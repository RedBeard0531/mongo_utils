/**
*    Copyright (C) 2017 MongoDB Inc.
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

#include <type_traits>

namespace mongo {

template <typename Enum>
struct EnableBitMaskOperators {
    static const bool enable = false;
};

template <typename Enum>
typename std::enable_if<EnableBitMaskOperators<Enum>::enable, Enum>::type operator&(Enum lhs,
                                                                                    Enum rhs) {
    using underlying = typename std::underlying_type<Enum>::type;
    return static_cast<Enum>(static_cast<underlying>(lhs) & static_cast<underlying>(rhs));
}

template <typename Enum>
typename std::enable_if<EnableBitMaskOperators<Enum>::enable, Enum>::type operator|(Enum lhs,
                                                                                    Enum rhs) {
    using underlying = typename std::underlying_type<Enum>::type;
    return static_cast<Enum>(static_cast<underlying>(lhs) | static_cast<underlying>(rhs));
}

template <typename Enum>
typename std::enable_if<EnableBitMaskOperators<Enum>::enable, Enum>::type operator^(Enum lhs,
                                                                                    Enum rhs) {
    using underlying = typename std::underlying_type<Enum>::type;
    return static_cast<Enum>(static_cast<underlying>(lhs) ^ static_cast<underlying>(rhs));
}

template <typename Enum>
typename std::enable_if<EnableBitMaskOperators<Enum>::enable, Enum>::type operator~(Enum rhs) {
    return static_cast<Enum>(~static_cast<typename std::underlying_type<Enum>::type>(rhs));
}

template <typename Enum>
typename std::enable_if<EnableBitMaskOperators<Enum>::enable, Enum&>::type operator|=(Enum& lhs,
                                                                                      Enum rhs) {
    using underlying = typename std::underlying_type<Enum>::type;
    lhs = static_cast<Enum>(static_cast<underlying>(lhs) | static_cast<underlying>(rhs));
    return lhs;
}

template <typename Enum>
typename std::enable_if<EnableBitMaskOperators<Enum>::enable, Enum&>::type operator&=(Enum& lhs,
                                                                                      Enum rhs) {
    using underlying = typename std::underlying_type<Enum>::type;
    lhs = static_cast<Enum>(static_cast<underlying>(lhs) & static_cast<underlying>(rhs));
    return lhs;
}

template <typename Enum>
typename std::enable_if<EnableBitMaskOperators<Enum>::enable, Enum&>::type operator^=(Enum& lhs,
                                                                                      Enum rhs) {
    using underlying = typename std::underlying_type<Enum>::type;
    lhs = static_cast<Enum>(static_cast<underlying>(lhs) ^ static_cast<underlying>(rhs));
    return lhs;
}

}  // namespace mongo

#define ENABLE_BITMASK_OPERATORS(x)                                                               \
                                                                                                  \
    template <>                                                                                   \
                                                                                                  \
    struct EnableBitMaskOperators<x> {                                                            \
        static_assert(std::is_enum<typename x>::value, "template parameter is not an enum type"); \
        static const bool enable = true;                                                          \
    };
