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

#include "mongo/config.h"

#include <cstddef>
#include <new>

namespace mongo {
namespace stdx {

#if __cplusplus < 201703L

#if defined(MONGO_CONFIG_MAX_EXTENDED_ALIGNMENT)
static_assert(MONGO_CONFIG_MAX_EXTENDED_ALIGNMENT >= sizeof(uint64_t), "Bad extended alignment");
constexpr std::size_t hardware_destructive_interference_size = MONGO_CONFIG_MAX_EXTENDED_ALIGNMENT;
#else
constexpr std::size_t hardware_destructive_interference_size = alignof(std::max_align_t);
#endif

constexpr auto hardware_constructive_interference_size = hardware_destructive_interference_size;

#else

using std::hardware_constructive_interference_size;
using std::hardware_destructive_interference_size;

#endif

}  // namespace stdx
}  // namespace mongo
