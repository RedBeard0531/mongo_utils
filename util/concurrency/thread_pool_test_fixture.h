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

#include <memory>

#include "mongo/unittest/unittest.h"

#include "mongo/util/concurrency/thread_pool_interface.h"

namespace mongo {

/**
 * Test fixture for tests that require a ThreadPool backed by a NetworkInterfaceMock.
 */
class ThreadPoolTest : public unittest::Test {
protected:
    ~ThreadPoolTest() override = default;

    ThreadPoolInterface& getThreadPool() {
        return *_threadPool;
    }

    void deleteThreadPool() {
        _threadPool.reset();
    }

    /**
     * Initializes both the NetworkInterfaceMock and ThreadPool but does not start the threadPool.
     */
    void setUp() override;

    /**
     * Destroys the replication threadPool.
     *
     * Shuts down and joins the running threadPool.
     */
    void tearDown() override;

private:
    virtual std::unique_ptr<ThreadPoolInterface> makeThreadPool() = 0;

    std::unique_ptr<ThreadPoolInterface> _threadPool;
};

}  // namespace mongo
