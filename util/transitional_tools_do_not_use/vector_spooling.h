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

#include <algorithm>
#include <iterator>
#include <memory>
#include <vector>

namespace mongo {
namespace transitional_tools_do_not_use {
template <typename T>
inline std::vector<T*> unspool_vector(const std::vector<std::unique_ptr<T>>& v) {
    std::vector<T*> result;
    result.reserve(v.size());
    std::transform(
        v.begin(), v.end(), std::back_inserter(result), [](const auto& p) { return p.get(); });
    return result;
}

template <typename T>
inline std::vector<std::unique_ptr<T>> spool_vector(const std::vector<T*>& v) noexcept {
    std::vector<std::unique_ptr<T>> result;
    result.reserve(v.size());
    std::transform(v.begin(), v.end(), std::back_inserter(result), [](const auto& p) {
        return std::unique_ptr<T>{p};
    });
    return result;
}

template <typename T>
inline std::vector<T*> leak_vector(std::vector<std::unique_ptr<T>>& v) noexcept {
    std::vector<T*> result;
    result.reserve(v.size());
    std::transform(
        v.begin(), v.end(), std::back_inserter(result), [](auto& p) { return p.release(); });
    return result;
}
}  // namespace transitional_tools_do_not_use
}  // namespace mongo
