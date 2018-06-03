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

#include <memory>

#include "mongo/base/status.h"
#include "mongo/stdx/functional.h"
#include "mongo/transport/session.h"
#include "mongo/util/future.h"
#include "mongo/util/time_support.h"

namespace mongo {

class OperationContext;

namespace transport {

enum ConnectSSLMode { kGlobalSSLMode, kEnableSSL, kDisableSSL };

class Reactor;
using ReactorHandle = std::shared_ptr<Reactor>;

/**
 * The TransportLayer moves Messages between transport::Endpoints and the database.
 * This class owns an Acceptor that generates new endpoints from which it can
 * source Messages.
 *
 * The TransportLayer creates Session objects and maps them internally to
 * endpoints. New Sessions are passed to the database (via a ServiceEntryPoint)
 * to be run. The database must then call additional methods on the TransportLayer
 * to manage the Session in a get-Message, handle-Message, return-Message cycle.
 * It must do this on its own thread(s).
 *
 * References to the TransportLayer should be stored on service context objects.
 */
class TransportLayer {
    MONGO_DISALLOW_COPYING(TransportLayer);

public:
    static const Status SessionUnknownStatus;
    static const Status ShutdownStatus;
    static const Status TicketSessionUnknownStatus;
    static const Status TicketSessionClosedStatus;

    friend class Session;

    virtual ~TransportLayer() = default;

    virtual StatusWith<SessionHandle> connect(HostAndPort peer,
                                              ConnectSSLMode sslMode,
                                              Milliseconds timeout) = 0;

    virtual Future<SessionHandle> asyncConnect(HostAndPort peer,
                                               ConnectSSLMode sslMode,
                                               const ReactorHandle& reactor) = 0;

    /**
     * Start the TransportLayer. After this point, the TransportLayer will begin accepting active
     * sessions from new transport::Endpoints.
     */
    virtual Status start() = 0;

    /**
     * Shut the TransportLayer down. After this point, the TransportLayer will
     * end all active sessions and won't accept new transport::Endpoints. Any
     * future calls to wait() or asyncWait() will fail. This method is synchronous and
     * will not return until all sessions have ended and any network connections have been
     * closed.
     */
    virtual void shutdown() = 0;

    /**
     * Optional method for subclasses to setup their state before being ready to accept
     * connections.
     */
    virtual Status setup() = 0;

    enum WhichReactor { kIngress, kEgress, kNewReactor };
    virtual ReactorHandle getReactor(WhichReactor which) = 0;

    virtual BatonHandle makeBaton(OperationContext* opCtx) {
        return nullptr;
    }

protected:
    TransportLayer() = default;
};

class ReactorTimer {
public:
    ReactorTimer() = default;
    ReactorTimer(const ReactorTimer&) = delete;
    ReactorTimer& operator=(const ReactorTimer&) = delete;

    /*
     * The destructor calls cancel() to ensure outstanding Futures are filled.
     */
    virtual ~ReactorTimer() = default;

    /*
     * Cancel any outstanding future from waitFor/waitUntil. The future will be filled with an
     * ErrorCodes::CallbackCancelled status.
     *
     * If no future is outstanding, then this is a noop.
     */
    virtual void cancel(const BatonHandle& baton = nullptr) = 0;

    /*
     * Returns a future that will be filled with Status::OK after the timeout has ellapsed.
     *
     * Calling this implicitly calls cancel().
     */
    virtual Future<void> waitFor(Milliseconds timeout, const BatonHandle& baton = nullptr) = 0;
    virtual Future<void> waitUntil(Date_t timeout, const BatonHandle& baton = nullptr) = 0;
};

class Reactor {
public:
    Reactor(const Reactor&) = delete;
    Reactor& operator=(const Reactor&) = delete;

    virtual ~Reactor() = default;

    /*
     * Run the event loop of the reactor until stop() is called.
     */
    virtual void run() noexcept = 0;
    virtual void runFor(Milliseconds time) noexcept = 0;
    virtual void stop() = 0;

    using Task = stdx::function<void()>;

    enum ScheduleMode { kDispatch, kPost };
    virtual void schedule(ScheduleMode mode, Task task) = 0;

    template <typename Callback>
    Future<FutureContinuationResult<Callback>> execute(Callback&& cb) {
        auto pf = makePromiseFuture<FutureContinuationResult<Callback>>();
        schedule(kPost, [ cb = std::forward<Callback>(cb), sp = pf.promise.share() ]() mutable {
            sp.setWith(cb);
        });

        return std::move(pf.future);
    }

    virtual bool onReactorThread() const = 0;

    /*
     * Makes a timer tied to this reactor's event loop. Timeout callbacks will be
     * executed in a thread calling run() or runFor().
     */
    virtual std::unique_ptr<ReactorTimer> makeTimer() = 0;
    virtual Date_t now() = 0;

protected:
    Reactor() = default;
};


}  // namespace transport
}  // namespace mongo
