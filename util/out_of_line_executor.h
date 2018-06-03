/**
 * Copyright (C) 2018 MongoDB Inc.
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

#include "mongo/stdx/functional.h"
#include "mongo/util/future.h"

namespace mongo {

/**
 * Provides the minimal api for a simple out of line executor that can run non-cancellable
 * callbacks.
 *
 * Adds in a minimal amount of support for futures.
 *
 * The contract for scheduling work on an executor is that it never blocks the caller.  It doesn't
 * necessarily need to offer forward progress guarantees, but actual calls to schedule() should not
 * deadlock.
 *
 * As an explicit point of implementation: it will never invoke the passed callback from within the
 * scheduling call.
 */
class OutOfLineExecutor {
public:
    /**
     * Invokes the callback on the executor, as in schedule(), returning a future with its result.
     * That future may be ready by the time the caller returns, which means that continuations
     * chained on the returned future may be invoked on the caller of execute's stack.
     */
    template <typename Callback>
    Future<FutureContinuationResult<Callback>> execute(Callback&& cb) {
        auto pf = makePromiseFuture<FutureContinuationResult<Callback>>();

        schedule([ cb = std::forward<Callback>(cb), sp = pf.promise.share() ]() mutable {
            sp.setWith(std::move(cb));
        });

        return std::move(pf.future);
    }

    /**
     * Invokes the callback on the executor.  This never happens immediately on the caller's stack.
     */
    virtual void schedule(stdx::function<void()> func) = 0;

protected:
    ~OutOfLineExecutor() noexcept {}
};

}  // namespace mongo
