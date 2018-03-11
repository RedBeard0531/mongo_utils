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

#if defined(_WIN32)
#include <boost/unordered_set.hpp>
#else
#include <unordered_set>
#endif

namespace mongo {
namespace stdx {

#if defined(_WIN32)
using ::boost::unordered_set;       // NOLINT
using ::boost::unordered_multiset;  // NOLINT
#else
using ::std::unordered_set;       // NOLINT
using ::std::unordered_multiset;  // NOLINT
#endif

}  // namespace stdx
}  // namespace mongo
