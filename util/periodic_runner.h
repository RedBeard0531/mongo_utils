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

#include <string>

#include "mongo/base/disallow_copying.h"
#include "mongo/stdx/functional.h"
#include "mongo/util/time_support.h"

namespace mongo {

class Client;

/**
 * An interface for objects that run work items at specified intervals.
 *
 * Implementations may use whatever internal threading and eventing
 * model they wish. Implementations may choose when to stop running
 * scheduled jobs (for example, some implementations may stop running
 * when the server is in global shutdown).
 *
 * The runner will create client objects that it passes to jobs to use.
 */
class PeriodicRunner {
public:
    using Job = stdx::function<void(Client* client)>;

    struct PeriodicJob {
        PeriodicJob(std::string name, Job callable, Milliseconds period)
            : name(std::move(name)), job(std::move(callable)), interval(period) {}

        /**
         * name of the job
         */
        std::string name;

        /**
         * A task to be run at regular intervals by the runner.
         */
        Job job;

        /**
         * An interval at which the job should be run.
         */
        Milliseconds interval;
    };

    virtual ~PeriodicRunner();

    /**
     * Schedules a job to be run at periodic intervals.
     *
     * If the runner is not running when a job is scheduled, that job should
     * be saved so that it may run in the future once startup() is called.
     */
    virtual void scheduleJob(PeriodicJob job) = 0;

    /**
     * Starts up this periodic runner.
     *
     * This method may safely be called multiple times, either with or without
     * calls to shutdown() in between.
     */
    virtual void startup() = 0;

    /**
     * Shuts down this periodic runner. Stops all jobs from running.
     *
     * This method may safely be called multiple times, either with or without
     * calls to startup() in between. Any jobs that have been scheduled on this
     * runner should no longer execute once shutdown() is called.
     */
    virtual void shutdown() = 0;
};

}  // namespace mongo
