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

#include <array>
#include <vector>

#include "mongo/db/service_context.h"
#include "mongo/platform/atomic_word.h"
#include "mongo/stdx/condition_variable.h"
#include "mongo/stdx/list.h"
#include "mongo/stdx/thread.h"
#include "mongo/transport/service_executor.h"
#include "mongo/transport/service_executor_task_names.h"
#include "mongo/transport/transport_layer.h"
#include "mongo/util/tick_source.h"

namespace mongo {
namespace transport {

/**
 * This is an ASIO-based adaptive ServiceExecutor. It guarantees that threads will not become stuck
 * or deadlocked longer that its configured timeout and that idle threads will terminate themselves
 * if they spend more than its configure idle threshold idle.
 */
class ServiceExecutorAdaptive : public ServiceExecutor {
public:
    struct Options {
        virtual ~Options() = default;
        // The minimum number of threads the executor will keep running to service tasks.
        virtual int reservedThreads() const = 0;

        // The amount of time each worker thread runs before considering exiting because of
        // idleness.
        virtual Milliseconds workerThreadRunTime() const = 0;

        // workerThreadRuntime() is offset by a random value between -jitter and +jitter to prevent
        // thundering herds
        virtual int runTimeJitter() const = 0;

        // The amount of time the controller thread will wait before checking for stuck threads
        // to guarantee forward progress
        virtual Milliseconds stuckThreadTimeout() const = 0;

        // The maximum allowed latency between when a task is scheduled and a thread is started to
        // service it.
        virtual Microseconds maxQueueLatency() const = 0;

        // Threads that spend less than this threshold doing work during their workerThreadRunTime
        // period will exit
        virtual int idlePctThreshold() const = 0;

        // The maximum allowable depth of recursion for tasks scheduled with the MayRecurse flag
        // before stack unwinding is forced.
        virtual int recursionLimit() const = 0;
    };

    explicit ServiceExecutorAdaptive(ServiceContext* ctx, ReactorHandle reactor);
    explicit ServiceExecutorAdaptive(ServiceContext* ctx,
                                     ReactorHandle reactor,
                                     std::unique_ptr<Options> config);

    ServiceExecutorAdaptive(ServiceExecutorAdaptive&&) = default;
    ServiceExecutorAdaptive& operator=(ServiceExecutorAdaptive&&) = default;
    virtual ~ServiceExecutorAdaptive();

    Status start() final;
    Status shutdown(Milliseconds timeout) final;
    Status schedule(Task task, ScheduleFlags flags, ServiceExecutorTaskName taskName) final;

    Mode transportMode() const final {
        return Mode::kAsynchronous;
    }

    void appendStats(BSONObjBuilder* bob) const final;

    int threadsRunning() {
        return _threadsRunning.load();
    }

private:
    class TickTimer {
    public:
        explicit TickTimer(TickSource* tickSource)
            : _tickSource(tickSource),
              _ticksPerMillisecond(_tickSource->getTicksPerSecond() / 1000),
              _start(_tickSource->getTicks()) {
            invariant(_ticksPerMillisecond > 0);
        }

        TickSource::Tick sinceStartTicks() const {
            return _tickSource->getTicks() - _start.load();
        }

        Milliseconds sinceStart() const {
            return Milliseconds{sinceStartTicks() / _ticksPerMillisecond};
        }

        void reset() {
            _start.store(_tickSource->getTicks());
        }

    private:
        TickSource* const _tickSource;
        const TickSource::Tick _ticksPerMillisecond;
        AtomicWord<TickSource::Tick> _start;
    };

    class CumulativeTickTimer {
    public:
        CumulativeTickTimer(TickSource* ts) : _timer(ts) {}

        TickSource::Tick markStopped() {
            stdx::lock_guard<stdx::mutex> lk(_mutex);
            invariant(_running);
            _running = false;
            auto curTime = _timer.sinceStartTicks();
            _accumulator += curTime;
            return curTime;
        }

        void markRunning() {
            stdx::lock_guard<stdx::mutex> lk(_mutex);
            invariant(!_running);
            _timer.reset();
            _running = true;
        }

        TickSource::Tick totalTime() const {
            stdx::lock_guard<stdx::mutex> lk(_mutex);
            if (!_running)
                return _accumulator;
            return _timer.sinceStartTicks() + _accumulator;
        }

    private:
        TickTimer _timer;
        mutable stdx::mutex _mutex;
        TickSource::Tick _accumulator = 0;
        bool _running = false;
    };

    struct Metrics {
        AtomicWord<int64_t> _totalQueued{0};
        AtomicWord<int64_t> _totalExecuted{0};
        AtomicWord<TickSource::Tick> _totalSpentQueued{0};
        AtomicWord<TickSource::Tick> _totalSpentExecuting{0};
    };

    using MetricsArray =
        std::array<Metrics, static_cast<size_t>(ServiceExecutorTaskName::kMaxTaskName)>;

    enum class ThreadCreationReason { kStuckDetection, kStarvation, kReserveMinimum, kMax };
    enum class ThreadTimer { kRunning, kExecuting };

    struct ThreadState {
        ThreadState(TickSource* ts) : running(ts), executing(ts) {}

        CumulativeTickTimer running;
        TickSource::Tick executingCurRun;
        CumulativeTickTimer executing;
        MetricsArray threadMetrics;
        std::int64_t markIdleCounter = 0;
        int recursionDepth = 0;
    };

    using ThreadList = stdx::list<ThreadState>;

    void _startWorkerThread(ThreadCreationReason reason);
    static StringData _threadStartedByToString(ThreadCreationReason reason);
    void _workerThreadRoutine(int threadId, ThreadList::iterator it);
    void _controllerThreadRoutine();
    bool _isStarved() const;
    Milliseconds _getThreadJitter() const;

    void _accumulateTaskMetrics(MetricsArray* outArray, const MetricsArray& inputArray) const;
    void _accumulateAllTaskMetrics(MetricsArray* outputMetricsArray,
                                   const stdx::unique_lock<stdx::mutex>& lk) const;
    TickSource::Tick _getThreadTimerTotal(ThreadTimer which,
                                          const stdx::unique_lock<stdx::mutex>& lk) const;

    ReactorHandle _reactorHandle;

    std::unique_ptr<Options> _config;

    mutable stdx::mutex _threadsMutex;
    ThreadList _threads;
    std::array<int64_t, static_cast<size_t>(ThreadCreationReason::kMax)> _threadStartCounters;

    stdx::thread _controllerThread;

    TickSource* const _tickSource;
    AtomicWord<bool> _isRunning{false};

    // These counters are used to detect stuck threads and high task queuing.
    AtomicWord<int> _threadsRunning{0};
    AtomicWord<int> _threadsPending{0};
    AtomicWord<int> _threadsInUse{0};
    AtomicWord<int> _tasksQueued{0};
    AtomicWord<int> _deferredTasksQueued{0};
    TickTimer _lastScheduleTimer;
    AtomicWord<TickSource::Tick> _pastThreadsSpentExecuting{0};
    AtomicWord<TickSource::Tick> _pastThreadsSpentRunning{0};
    static thread_local ThreadState* _localThreadState;

    // These counters are only used for reporting in serverStatus.
    AtomicWord<int64_t> _totalQueued{0};
    AtomicWord<int64_t> _totalExecuted{0};
    AtomicWord<TickSource::Tick> _totalSpentQueued{0};

    // Threads signal this condition variable when they exit so we can gracefully shutdown
    // the executor.
    stdx::condition_variable _deathCondition;

    // Tasks should signal this condition variable if they want the thread controller to
    // track their progress and do fast stuck detection
    AtomicWord<int> _starvationCheckRequests{0};
    stdx::condition_variable _scheduleCondition;

    MetricsArray _accumulatedMetrics;
};

}  // namespace transport
}  // namespace mongo
