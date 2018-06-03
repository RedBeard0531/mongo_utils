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

#include <memory>

#include "mongo/stdx/functional.h"
#include "mongo/transport/transport_layer.h"
#include "mongo/util/future.h"
#include "mongo/util/time_support.h"

namespace mongo {

class OperationContext;

namespace transport {

class TransportLayer;
class Session;
class ReactorTimer;

/**
 * A Baton is basically a networking reactor, with limited functionality and no forward progress
 * guarantees.  Rather than asynchronously running tasks through one, the baton records the intent
 * of those tasks and defers waiting and execution to a later call to run();
 *
 * Baton's provide a mechanism to allow consumers of a transport layer to execute IO themselves,
 * rather than having this occur on another thread.  This can improve performance by minimizing
 * context switches, as well as improving the readability of stack traces by grounding async
 * execution on top of a regular client call stack.
 */
class Baton {
public:
    virtual ~Baton() = default;

    /**
     * Detaches a baton from an associated opCtx.
     */
    virtual void detach() = 0;

    /**
     * Executes a callback on the baton via schedule.  Returns a future which will execute on the
     * baton runner.
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
     * Executes a callback on the baton.
     */
    virtual void schedule(stdx::function<void()> func) = 0;

    /**
     * Adds a session, returning a future which activates on read/write-ability of the session.
     */
    enum class Type {
        In,
        Out,
    };
    virtual Future<void> addSession(Session& session, Type type) = 0;

    /**
     * Adds a timer, returning a future which activates after a duration.
     */
    virtual Future<void> waitFor(const ReactorTimer& timer, Milliseconds timeout) = 0;

    /**
     * Adds a timer, returning a future which activates after a deadline.
     */
    virtual Future<void> waitUntil(const ReactorTimer& timer, Date_t expiration) = 0;

    /**
     * Cancels waiting on a session.
     *
     * Returns true if the session was in the baton to be cancelled.
     */
    virtual bool cancelSession(Session& session) = 0;

    /**
     * Cancels waiting on a timer
     *
     * Returns true if the timer was in the baton to be cancelled.
     */
    virtual bool cancelTimer(const ReactorTimer& timer) = 0;

    /**
     * Runs the baton.  This blocks, waiting for networking events or timeouts, and fulfills
     * promises and executes scheduled work.
     *
     * Returns false if the optional deadline has passed
     */
    virtual bool run(OperationContext* opCtx, boost::optional<Date_t> deadline) = 0;
};

using BatonHandle = std::shared_ptr<Baton>;

}  // namespace transport
}  // namespace mongo
