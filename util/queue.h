// @file queue.h

/*    Copyright 2009 10gen Inc.
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
#include <limits>
#include <queue>

#include "mongo/base/disallow_copying.h"
#include "mongo/stdx/chrono.h"
#include "mongo/stdx/condition_variable.h"
#include "mongo/stdx/functional.h"
#include "mongo/stdx/mutex.h"

namespace mongo {

/**
 * Simple blocking queue with optional max size (by count or custom sizing function).
 * A custom sizing function can optionally be given.  By default the getSize function
 * returns 1 for each item, resulting in size equaling the number of items queued.
 *
 * Note that use of this class is deprecated.  This class only works with a single consumer and
 * a single producer.
 */
template <typename T>
class BlockingQueue {
    MONGO_DISALLOW_COPYING(BlockingQueue);

public:
    using GetSizeFn = stdx::function<size_t(const T&)>;

    BlockingQueue() : BlockingQueue(std::numeric_limits<std::size_t>::max()) {}
    BlockingQueue(size_t size) : BlockingQueue(size, [](const T&) { return 1; }) {}
    BlockingQueue(size_t size, GetSizeFn f) : _maxSize(size), _getSize(f) {}

    void pushEvenIfFull(T const& t) {
        stdx::unique_lock<stdx::mutex> lk(_lock);
        pushImpl_inlock(t, _getSize(t));
    }

    void push(T const& t) {
        stdx::unique_lock<stdx::mutex> lk(_lock);
        _clearing = false;
        size_t tSize = _getSize(t);
        _waitForSpace_inlock(tSize, lk);
        pushImpl_inlock(t, tSize);
    }

    /**
     * Caller must ensure the BlockingQueue hasSpace before pushing since this function won't block.
     *
     * NOTE: Should only be used in a single producer case.
     */
    template <typename Container>
    void pushAllNonBlocking(const Container& objs) {
        pushAllNonBlocking(std::begin(objs), std::end(objs));
    }

    template <typename Iterator>
    void pushAllNonBlocking(Iterator begin, Iterator end) {
        if (begin == end) {
            return;
        }

        stdx::unique_lock<stdx::mutex> lk(_lock);
        const auto startedEmpty = _queue.empty();
        _clearing = false;

        auto pushOne = [this](const T& obj) {
            size_t tSize = _getSize(obj);
            _queue.push(obj);
            _currentSize += tSize;
        };
        std::for_each(begin, end, pushOne);

        if (startedEmpty) {
            _cvNoLongerEmpty.notify_one();
        }
    }

    /**
     * Returns when enough space is available.
     *
     * NOTE: Should only be used in a single producer case.
     */
    void waitForSpace(size_t size) {
        stdx::unique_lock<stdx::mutex> lk(_lock);
        _waitForSpace_inlock(size, lk);
    }

    bool empty() const {
        stdx::lock_guard<stdx::mutex> lk(_lock);
        return _queue.empty();
    }

    /**
     * The size as measured by the size function. Default to counting each item
     */
    size_t size() const {
        stdx::lock_guard<stdx::mutex> lk(_lock);
        return _currentSize;
    }

    /**
     * The max size for this queue
     */
    size_t maxSize() const {
        return _maxSize;
    }

    /**
     * The number/count of items in the queue ( _queue.size() )
     */
    size_t count() const {
        stdx::lock_guard<stdx::mutex> lk(_lock);
        return _queue.size();
    }

    void clear() {
        stdx::lock_guard<stdx::mutex> lk(_lock);
        _clearing = true;
        _queue = std::queue<T>();
        _currentSize = 0;
        _cvNoLongerFull.notify_one();
        _cvNoLongerEmpty.notify_one();
    }

    bool tryPop(T& t) {
        stdx::lock_guard<stdx::mutex> lk(_lock);
        if (_queue.empty())
            return false;

        t = _queue.front();
        _queue.pop();
        _currentSize -= _getSize(t);
        _cvNoLongerFull.notify_one();

        return true;
    }

    T blockingPop() {
        stdx::unique_lock<stdx::mutex> lk(_lock);
        _clearing = false;
        while (_queue.empty() && !_clearing)
            _cvNoLongerEmpty.wait(lk);
        if (_clearing) {
            return T{};
        }

        T t = _queue.front();
        _queue.pop();
        _currentSize -= _getSize(t);
        _cvNoLongerFull.notify_one();

        return t;
    }


    /**
     * blocks waiting for an object until maxSecondsToWait passes
     * if got one, return true and set in t
     * otherwise return false and t won't be changed
     */
    bool blockingPop(T& t, int maxSecondsToWait) {
        using namespace stdx::chrono;
        const auto deadline = system_clock::now() + seconds(maxSecondsToWait);
        stdx::unique_lock<stdx::mutex> lk(_lock);
        _clearing = false;
        while (_queue.empty() && !_clearing) {
            if (stdx::cv_status::timeout == _cvNoLongerEmpty.wait_until(lk, deadline))
                return false;
        }

        if (_clearing) {
            return false;
        }
        t = _queue.front();
        _queue.pop();
        _currentSize -= _getSize(t);
        _cvNoLongerFull.notify_one();
        return true;
    }

    // Obviously, this should only be used when you have
    // only one consumer
    bool blockingPeek(T& t, int maxSecondsToWait) {
        using namespace stdx::chrono;
        const auto deadline = system_clock::now() + seconds(maxSecondsToWait);
        stdx::unique_lock<stdx::mutex> lk(_lock);
        _clearing = false;
        while (_queue.empty() && !_clearing) {
            if (stdx::cv_status::timeout == _cvNoLongerEmpty.wait_until(lk, deadline))
                return false;
        }
        if (_clearing) {
            return false;
        }
        t = _queue.front();
        return true;
    }

    // Obviously, this should only be used when you have
    // only one consumer
    bool peek(T& t) {
        stdx::unique_lock<stdx::mutex> lk(_lock);
        if (_queue.empty()) {
            return false;
        }

        t = _queue.front();
        return true;
    }

    /**
     * Returns the item most recently added to the queue or nothing if the queue is empty.
     */
    boost::optional<T> lastObjectPushed() const {
        stdx::unique_lock<stdx::mutex> lk(_lock);
        if (_queue.empty()) {
            return {};
        }

        return {_queue.back()};
    }

private:
    /**
     * Returns when enough space is available.
     */
    void _waitForSpace_inlock(size_t size, stdx::unique_lock<stdx::mutex>& lk) {
        while (_currentSize + size > _maxSize) {
            _cvNoLongerFull.wait(lk);
        }
    }

    void pushImpl_inlock(const T& obj, size_t objSize) {
        _clearing = false;
        _queue.push(obj);
        _currentSize += objSize;
        if (_queue.size() == 1)  // We were empty.
            _cvNoLongerEmpty.notify_one();
    }

    mutable stdx::mutex _lock;
    std::queue<T> _queue;
    const size_t _maxSize;
    size_t _currentSize = 0;
    GetSizeFn _getSize;
    bool _clearing = false;

    stdx::condition_variable _cvNoLongerFull;
    stdx::condition_variable _cvNoLongerEmpty;
};
}
