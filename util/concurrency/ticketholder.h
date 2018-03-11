/*    Copyright 2015 MongoDB Inc.
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

#if defined(__linux__)
#include <semaphore.h>
#endif

#include "mongo/base/disallow_copying.h"
#include "mongo/db/operation_context.h"
#include "mongo/stdx/condition_variable.h"
#include "mongo/stdx/mutex.h"
#include "mongo/util/concurrency/mutex.h"
#include "mongo/util/time_support.h"

namespace mongo {

class TicketHolder {
    MONGO_DISALLOW_COPYING(TicketHolder);

public:
    explicit TicketHolder(int num);
    ~TicketHolder();

    bool tryAcquire();

    /**
     * Attempts to acquire a ticket. Blocks until a ticket is acquired or the OperationContext
     * 'opCtx' is killed, throwing an AssertionException.
     * If 'opCtx' is not provided or equal to nullptr, the wait is not interruptible.
     */
    void waitForTicket(OperationContext* opCtx);
    void waitForTicket() {
        waitForTicket(nullptr);
    }

    /**
     * Attempts to acquire a ticket within a deadline, 'until'. Returns 'true' if a ticket is
     * acquired and 'false' if the deadline is reached, but the operation is retryable. Throws an
     * AssertionException if the OperationContext 'opCtx' is killed and no waits for tickets can
     * proceed.
     * If 'opCtx' is not provided or equal to nullptr, the wait is not interruptible.
     */
    bool waitForTicketUntil(OperationContext* opCtx, Date_t until);
    bool waitForTicketUntil(Date_t until) {
        return waitForTicketUntil(nullptr, until);
    }
    void release();

    Status resize(int newSize);

    int available() const;

    int used() const;

    int outof() const;

private:
#if defined(__linux__)
    mutable sem_t _sem;

    // You can read _outof without a lock, but have to hold _resizeMutex to change.
    AtomicInt32 _outof;
    stdx::mutex _resizeMutex;
#else
    bool _tryAcquire();

    AtomicInt32 _outof;
    int _num;
    stdx::mutex _mutex;
    stdx::condition_variable _newTicket;
#endif
};

class ScopedTicket {
public:
    ScopedTicket(TicketHolder* holder) : _holder(holder) {
        _holder->waitForTicket();
    }

    ~ScopedTicket() {
        _holder->release();
    }

private:
    TicketHolder* _holder;
};

class TicketHolderReleaser {
    MONGO_DISALLOW_COPYING(TicketHolderReleaser);

public:
    TicketHolderReleaser() {
        _holder = NULL;
    }

    explicit TicketHolderReleaser(TicketHolder* holder) {
        _holder = holder;
    }

    ~TicketHolderReleaser() {
        if (_holder) {
            _holder->release();
        }
    }

    bool hasTicket() const {
        return _holder != NULL;
    }

    void reset(TicketHolder* holder = NULL) {
        if (_holder) {
            _holder->release();
        }
        _holder = holder;
    }

private:
    TicketHolder* _holder;
};
}  // namespace mongo
