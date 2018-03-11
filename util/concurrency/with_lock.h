/**    Copyright 2017 MongoDB, Inc.
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

#include "mongo/stdx/mutex.h"
#include "mongo/util/assert_util.h"

#include <utility>

namespace mongo {

/**
 * WithLock is an attestation to pass as an argument to functions that must be called only while
 * holding a lock, as a rigorous alternative to an unchecked naming convention and/or stern
 * comments.  It helps prevent a common usage error.
 *
 * It may be used to modernize code from (something like) this
 *
 *     // Member _mutex MUST be held when calling this:
 *     void _clobber_inlock(OperationContext* opCtx) {
 *         _stuff = makeStuff(opCtx);
 *     }
 *
 * into
 *
 *     void _clobber(WithLock, OperationContext* opCtx) {
 *         _stuff = makeStuff(opCtx);
 *     }
 *
 * A call to such a function looks like this:
 *
 *     stdx::lock_guard<stdx::mutex> lk(_mutex);
 *     _clobber(lk, opCtx);  // instead of _clobber_inlock(opCtx)
 *
 * Note that the formal argument need not (and should not) be named unless it is needed to pass
 * the attestation along to another function:
 *
 *     void _clobber(WithLock lock, OperationContext* opCtx) {
 *         _really_clobber(lock, opCtx);
 *     }
 *
 */
struct WithLock {
    template <typename Mutex>
    WithLock(stdx::lock_guard<Mutex> const&) noexcept {}

    template <typename Mutex>
    WithLock(stdx::unique_lock<Mutex> const& lock) noexcept {
        invariant(lock.owns_lock());
    }

    // Add constructors from any other lock types here.

    // Pass by value is OK.
    WithLock(WithLock const&) noexcept {}
    WithLock(WithLock&&) noexcept {}

    // No assigning WithLocks.
    void operator=(WithLock const&) = delete;
    void operator=(WithLock&&) = delete;

    // No moving a lock_guard<> or unique_lock<> in.
    template <typename Mutex>
    WithLock(stdx::lock_guard<Mutex>&&) = delete;
    template <typename Mutex>
    WithLock(stdx::unique_lock<Mutex>&&) = delete;

    /*
     * Produces a WithLock without benefit of any actual lock, for use in cases where a lock is not
     * really needed, such as in many (but not all!) constructors.
     */
    static WithLock withoutLock() noexcept {
        return {};
    }

private:
    WithLock() noexcept = default;
};

}  // namespace mongo

namespace std {
// No moving a WithLock:
template <>
mongo::WithLock&& move<mongo::WithLock>(mongo::WithLock&&) noexcept = delete;
}  // namespace std
