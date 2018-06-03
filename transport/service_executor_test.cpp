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

#define MONGO_LOG_DEFAULT_COMPONENT ::mongo::logger::LogComponent::kDefault;

#include "mongo/platform/basic.h"

#include "boost/optional.hpp"

#include "mongo/db/service_context_noop.h"
#include "mongo/transport/service_executor_adaptive.h"
#include "mongo/transport/service_executor_synchronous.h"
#include "mongo/transport/service_executor_task_names.h"
#include "mongo/unittest/unittest.h"
#include "mongo/util/log.h"
#include "mongo/util/scopeguard.h"

#include <asio.hpp>

namespace mongo {
namespace {
using namespace transport;

struct TestOptions : public ServiceExecutorAdaptive::Options {
    int reservedThreads() const final {
        return 1;
    }

    Milliseconds workerThreadRunTime() const final {
        return Milliseconds{1000};
    }

    int runTimeJitter() const final {
        return 0;
    }

    Milliseconds stuckThreadTimeout() const final {
        return Milliseconds{100};
    }

    Microseconds maxQueueLatency() const final {
        return duration_cast<Microseconds>(Milliseconds{5});
    }

    int idlePctThreshold() const final {
        return 0;
    }

    int recursionLimit() const final {
        return 0;
    }
};

/* This implements the portions of the transport::Reactor based on ASIO, but leaves out
 * the methods not needed by ServiceExecutors.
 *
 * TODO Maybe use TransportLayerASIO's Reactor?
 */
class ASIOReactor : public transport::Reactor {
public:
    ASIOReactor() : _ioContext() {}

    void run() noexcept final {
        MONGO_UNREACHABLE;
    }

    void runFor(Milliseconds time) noexcept final {
        asio::io_context::work work(_ioContext);

        try {
            _ioContext.run_for(time.toSystemDuration());
        } catch (...) {
            severe() << "Uncaught exception in reactor: " << exceptionToStatus();
            fassertFailed(50476);
        }
    }

    void stop() final {
        _ioContext.stop();
    }

    std::unique_ptr<ReactorTimer> makeTimer() final {
        MONGO_UNREACHABLE;
    }

    Date_t now() final {
        MONGO_UNREACHABLE;
    }

    void schedule(ScheduleMode mode, Task task) final {
        if (mode == kDispatch) {
            _ioContext.dispatch(std::move(task));
        } else {
            _ioContext.post(std::move(task));
        }
    }

    bool onReactorThread() const final {
        return false;
    }

    operator asio::io_context&() {
        return _ioContext;
    }

private:
    asio::io_context _ioContext;
};

class ServiceExecutorAdaptiveFixture : public unittest::Test {
protected:
    void setUp() override {
        auto scOwned = stdx::make_unique<ServiceContextNoop>();
        setGlobalServiceContext(std::move(scOwned));

        auto configOwned = stdx::make_unique<TestOptions>();
        executorConfig = configOwned.get();
        executor = stdx::make_unique<ServiceExecutorAdaptive>(
            getGlobalServiceContext(), std::make_shared<ASIOReactor>(), std::move(configOwned));
    }

    ServiceExecutorAdaptive::Options* executorConfig;
    std::unique_ptr<ServiceExecutorAdaptive> executor;
    std::shared_ptr<asio::io_context> asioIOCtx;
};

class ServiceExecutorSynchronousFixture : public unittest::Test {
protected:
    void setUp() override {
        auto scOwned = stdx::make_unique<ServiceContextNoop>();
        setGlobalServiceContext(std::move(scOwned));

        executor = stdx::make_unique<ServiceExecutorSynchronous>(getGlobalServiceContext());
    }

    std::unique_ptr<ServiceExecutorSynchronous> executor;
};

void scheduleBasicTask(ServiceExecutor* exec, bool expectSuccess) {
    stdx::condition_variable cond;
    stdx::mutex mutex;
    auto task = [&cond, &mutex] {
        stdx::unique_lock<stdx::mutex> lk(mutex);
        cond.notify_all();
    };

    stdx::unique_lock<stdx::mutex> lk(mutex);
    auto status = exec->schedule(
        std::move(task), ServiceExecutor::kEmptyFlags, ServiceExecutorTaskName::kSSMStartSession);
    if (expectSuccess) {
        ASSERT_OK(status);
        cond.wait(lk);
    } else {
        ASSERT_NOT_OK(status);
    }
}

TEST_F(ServiceExecutorAdaptiveFixture, BasicTaskRuns) {
    ASSERT_OK(executor->start());
    auto guard = MakeGuard([this] { ASSERT_OK(executor->shutdown(Milliseconds{500})); });

    scheduleBasicTask(executor.get(), true);
}

TEST_F(ServiceExecutorAdaptiveFixture, ScheduleFailsBeforeStartup) {
    scheduleBasicTask(executor.get(), false);
}

TEST_F(ServiceExecutorSynchronousFixture, BasicTaskRuns) {
    ASSERT_OK(executor->start());
    auto guard = MakeGuard([this] { ASSERT_OK(executor->shutdown(Milliseconds{500})); });

    scheduleBasicTask(executor.get(), true);
}

TEST_F(ServiceExecutorSynchronousFixture, ScheduleFailsBeforeStartup) {
    scheduleBasicTask(executor.get(), false);
}


}  // namespace
}  // namespace mongo
