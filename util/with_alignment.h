/**
 * Copyright (C) 2017 MongoDB Inc.
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

#include <algorithm>

#include "mongo/stdx/new.h"

namespace mongo {

/**
 * Creates a new type with the same interface as T, but having the
 * given alignment. Note that if the given alignment is less than
 * alignof(T), the program is ill-formed. To ensure that your program
 * is well formed, see WithAligmentAtLeast, which will not reduce
 * alignment below the natural alignment of T.
 */
template <typename T, size_t alignment>
struct alignas(alignment) WithAlignment : T {
    using T::T;
};

/**
 * Creates a new type with the same interface as T, but having an
 * alignment greater than or equal to the given alignment. To ensure
 * that the program remains well formed, the alignment will not be
 * reduced below the natural alignment for T.
 */
template <typename T, size_t alignment>
using WithAlignmentAtLeast = WithAlignment<T, std::max(alignof(T), alignment)>;

/**
 * Creates a new type with the same interface as T but guaranteed to
 * be aligned to at least the size of a cache line.
 */
template <typename T>
using CacheAligned = WithAlignmentAtLeast<T, stdx::hardware_destructive_interference_size>;

}  // namespace mongo
