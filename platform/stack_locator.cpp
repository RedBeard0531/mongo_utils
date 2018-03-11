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

#include "mongo/platform/basic.h"

#include "mongo/platform/stack_locator.h"

#include "mongo/util/assert_util.h"

namespace mongo {

boost::optional<std::size_t> StackLocator::available() const {
    if (!begin() || !end())
        return boost::none;

    // Technically, it is undefined behavior to compare or subtract
    // two pointers that do not point into the same
    // aggregate. However, we know that these are both pointers within
    // the same stack, and it seems unlikely that the compiler will
    // see that it can elide the comparison here.

    const auto cbegin = reinterpret_cast<const char*>(begin());
    const auto cthis = reinterpret_cast<const char*>(this);
    const auto cend = reinterpret_cast<const char*>(end());

    // TODO: Assumes that stack grows downward
    invariant(cthis <= cbegin);
    invariant(cthis > cend);

    std::size_t avail = cthis - cend;

    return avail;
}

boost::optional<size_t> StackLocator::size() const {
    if (!begin() || !end())
        return boost::none;

    const auto cbegin = reinterpret_cast<const char*>(begin());
    const auto cend = reinterpret_cast<const char*>(end());

    // TODO: Assumes that stack grows downward
    invariant(cbegin > cend);

    return static_cast<size_t>(cbegin - cend);
}

}  // namespace mongo
