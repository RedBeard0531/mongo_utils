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

#include <boost/optional.hpp>

#include "mongo/db/operation_context.h"
#include "mongo/stdx/condition_variable.h"
#include "mongo/stdx/mutex.h"
#include "mongo/util/assert_util.h"
#include "mongo/util/duration.h"
#include "mongo/util/time_support.h"

namespace mongo {

/**
 * Allows waiting for a result returned from an asynchronous operation.
 */
template <class T>
class Notification {
public:
    Notification() = default;

    /**
     * Creates a notification object, which has already been set. Calls to any of the getters will
     * return immediately.
     */
    explicit Notification(T value) : _value(value) {}

    /**
     * Returns true if the notification has been set (i.e., the call to get/waitFor would not
     * block).
     */
    explicit operator bool() {
        stdx::unique_lock<stdx::mutex> lock(_mutex);
        return !!_value;
    }

    /**
     * If the notification has been set, returns immediately. Otherwise blocks until it becomes set.
     * If the wait is interrupted, throws an exception.
     */
    T& get(OperationContext* opCtx) {
        stdx::unique_lock<stdx::mutex> lock(_mutex);
        opCtx->waitForConditionOrInterrupt(_condVar, lock, [this]() -> bool { return !!_value; });
        return _value.get();
    }

    /**
     * If the notification has been set, returns immediately. Otherwise blocks until it becomes set.
     * This variant of get cannot be interrupted.
     */
    T& get() {
        stdx::unique_lock<stdx::mutex> lock(_mutex);
        while (!_value) {
            _condVar.wait(lock);
        }

        return _value.get();
    }

    /**
     * Sets the notification result and wakes up any threads, which might be blocked in the wait
     * call. Must only be called once for the lifetime of the notification.
     */
    void set(T value) {
        stdx::lock_guard<stdx::mutex> lock(_mutex);
        invariant(!_value);
        _value = std::move(value);
        _condVar.notify_all();
    }

    /**
     * If the notification is set, returns immediately. Otherwise, blocks until it either becomes
     * set or the waitTimeout expires, whichever comes first. Returns true if the notification is
     * set (in which case a subsequent call to get is guaranteed to not block) or false otherwise.
     * If the wait is interrupted, throws an exception.
     */
    bool waitFor(OperationContext* opCtx, Milliseconds waitTimeout) {
        stdx::unique_lock<stdx::mutex> lock(_mutex);
        return opCtx->waitForConditionOrInterruptFor(
            _condVar, lock, waitTimeout, [&]() { return !!_value; });
    }

private:
    stdx::mutex _mutex;
    stdx::condition_variable _condVar;

    // Protected by mutex and only moves from not-set to set once
    boost::optional<T> _value{boost::none};
};

template <>
class Notification<void> {
public:
    explicit operator bool() {
        return _notification.operator bool();
    }

    void get(OperationContext* opCtx) {
        _notification.get(opCtx);
    }

    void get() {
        _notification.get();
    }

    void set() {
        _notification.set(true);
    }

    bool waitFor(OperationContext* opCtx, Milliseconds waitTimeout) {
        return _notification.waitFor(opCtx, waitTimeout);
    }

private:
    Notification<bool> _notification;
};

}  // namespace mongo
