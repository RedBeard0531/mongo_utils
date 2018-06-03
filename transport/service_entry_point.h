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

#include "mongo/base/disallow_copying.h"
#include "mongo/db/dbmessage.h"
#include "mongo/transport/session.h"

namespace mongo {

/**
 * This is the entrypoint from the transport layer into mongod or mongos.
 *
 * The ServiceEntryPoint accepts new Sessions from the TransportLayer, and is
 * responsible for running these Sessions in a get-Message, run-Message,
 * reply-with-Message loop.  It may not do this on the TransportLayer’s thread.
 */
class ServiceEntryPoint {
    MONGO_DISALLOW_COPYING(ServiceEntryPoint);

public:
    /**
    * Stats for sessions open.
    */
    struct Stats {
        /**
        * Returns the number of sessions currently open.
        */
        size_t numOpenSessions = 0;

        /**
        * Returns the total number of sessions that have ever been created.
        */
        size_t numCreatedSessions = 0;

        /**
        * Returns the number of available sessions we could still open. Only relevant
        * when we are operating under a transport::Session limit (for example, in the
        * legacy implementation, we respect a maximum number of connections). If there
        * is no session limit, returns std::numeric_limits<int>::max().
        */
        size_t numAvailableSessions = 0;
    };

    virtual ~ServiceEntryPoint() = default;

    /**
     * Begin running a new Session. This method returns immediately.
     */
    virtual void startSession(transport::SessionHandle session) = 0;

    /**
     * End all sessions that do not match the mask in tags.
     */
    virtual void endAllSessions(transport::Session::TagMask tags) = 0;

    /**
    * Shuts down the service entry point.
    */
    virtual bool shutdown(Milliseconds timeout) = 0;

    /**
    * Returns high-level stats about current sessions.
    */
    virtual Stats sessionStats() const = 0;

    /**
    * Returns the number of sessions currently open.
    */
    virtual size_t numOpenSessions() const = 0;

    /**
     * Processes a request and fills out a DbResponse.
     */
    virtual DbResponse handleRequest(OperationContext* opCtx, const Message& request) = 0;

protected:
    ServiceEntryPoint() = default;
};

}  // namespace mongo
