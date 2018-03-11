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

#pragma once

#include "mongo/base/disallow_copying.h"
#include "mongo/stdx/functional.h"

namespace mongo {

class Status;

/**
 * Interface for a thread pool.
 */
class ThreadPoolInterface {
    MONGO_DISALLOW_COPYING(ThreadPoolInterface);

public:
    using Task = stdx::function<void()>;

    /**
     * Destroys a thread pool.
     *
     * The destructor may block if join() has not previously been called and returned.
     * It is fatal to destroy the pool while another thread is blocked on join().
     */
    virtual ~ThreadPoolInterface() = default;

    /**
     * Starts the thread pool. May be called at most once.
     */
    virtual void startup() = 0;

    /**
     * Signals the thread pool to shut down.  Returns promptly.
     *
     * After this call, the thread will return an error for subsequent calls to schedule().
     *
     * May be called by a task executing in the thread pool.  Call join() after calling shutdown()
     * to block until all tasks scheduled on the pool complete.
     */
    virtual void shutdown() = 0;

    /**
     * Blocks until the thread pool has fully shut down. Call at most once, and never from a task
     * inside the pool.
     */
    virtual void join() = 0;

    /**
     * Schedules "task" to run in the thread pool.
     *
     * Returns OK on success, ShutdownInProgress if shutdown() has already executed.
     *
     * It is safe to call this before startup(), but the scheduled task will not execute
     * until after startup() is called.
     */
    virtual Status schedule(Task task) = 0;

protected:
    ThreadPoolInterface() = default;
};

}  // namespace mongo
