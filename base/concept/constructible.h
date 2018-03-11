/*-
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

#include <type_traits>

#include "mongo/stdx/type_traits.h"

namespace mongo {
namespace concept {

/**
 * The Constructable trait indicates whether `T` is constructible from `Constructible`.
 *
 * RETURNS: true if `T{ std::declval< Constructible >() }` is a valid expression and false
 * otherwise.
 */
template <typename T, typename Constructible, typename = void>
struct is_constructible : std::false_type {};

template <typename T, typename Constructible>
struct is_constructible<T,
                        Constructible,
                        stdx::void_t<decltype(T{std::declval<Constructible<T>>()})>>
    : std::true_type {};

/**
 * The Constructable concept models a type which can be passed to a single-argument constructor of
 * `T`.
 * This is not possible to describe in the type `Constructible`.
 *
 * The expression: `T{ std::declval< Constructible< T > >() }` should be valid.
 *
 * This concept is more broadly applicable than `ConvertibleTo`.  `ConvertibleTo` uses implicit
 * conversion, whereas `Constructible` uses direct construction.
 */
template <typename T>
struct Constructible {};
}  // namespace concept
}  // namespace mongo
