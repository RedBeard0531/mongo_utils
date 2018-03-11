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

#include <chrono>
#include <ctime>
#include <exception>
#include <thread>
#include <type_traits>

namespace mongo {
namespace stdx {

/**
 * We're wrapping std::thread here, rather than aliasing it, because we'd like
 * a std::thread that's identical in all ways to the original, but terminates
 * if a new thread cannot be allocated.  We'd like this behavior because we
 * rarely if ever try/catch thread creation, and don't have a strategy for
 * retrying.  Therefore, all throwing does is remove context as to which part
 * of the system failed thread creation (as the exception itself is caught at
 * the top of the stack).
 *
 * We're putting this in stdx, rather than having it as some kind of
 * mongo::Thread, because the signature and use of the type is otherwise
 * completely identical.  Rather than migrate all callers, it was deemed
 * simpler to make the in place adjustment and retain it in stdx.
 *
 * We implement this with private inheritance to minimize the overhead of our
 * wrapping and to simplify the implementation.
 */
class thread : private ::std::thread {  // NOLINT
public:
    using ::std::thread::native_handle_type;  // NOLINT
    using ::std::thread::id;                  // NOLINT

    thread() noexcept : ::std::thread::thread() {}  // NOLINT

    thread(const thread&) = delete;

    thread(thread&& other) noexcept
        : ::std::thread::thread(static_cast<::std::thread&&>(std::move(other))) {}  // NOLINT

    /**
     * As of C++14, the Function overload for std::thread requires that this constructor only
     * participate in overload resolution if std::decay_t<Function> is not the same type as thread.
     * That prevents this overload from intercepting calls that might generate implicit conversions
     * before binding to other constructors (specifically move/copy constructors).
     */
    template <
        class Function,
        class... Args,
        typename std::enable_if<!std::is_same<thread, typename std::decay<Function>::type>::value,
                                int>::type = 0>
    explicit thread(Function&& f, Args&&... args) try:
        ::std::thread::thread(std::forward<Function>(f), std::forward<Args>(args)...) {}  // NOLINT
    catch (...) {
        std::terminate();
    }

    thread& operator=(const thread&) = delete;

    thread& operator=(thread&& other) noexcept {
        return static_cast<thread&>(
            ::std::thread::operator=(static_cast<::std::thread&&>(std::move(other))));  // NOLINT
    };

    using ::std::thread::joinable;              // NOLINT
    using ::std::thread::get_id;                // NOLINT
    using ::std::thread::native_handle;         // NOLINT
    using ::std::thread::hardware_concurrency;  // NOLINT

    using ::std::thread::join;    // NOLINT
    using ::std::thread::detach;  // NOLINT

    void swap(thread& other) noexcept {
        ::std::thread::swap(static_cast<::std::thread&>(other));  // NOLINT
    }
};

inline void swap(thread& lhs, thread& rhs) noexcept {
    lhs.swap(rhs);
}

namespace this_thread {
using std::this_thread::get_id;  // NOLINT
using std::this_thread::yield;   // NOLINT

#ifdef _WIN32
using std::this_thread::sleep_for;    // NOLINT
using std::this_thread::sleep_until;  // NOLINT
#else
template <class Rep, class Period>
inline void sleep_for(const std::chrono::duration<Rep, Period>& sleep_duration) {  // NOLINT
    if (sleep_duration <= sleep_duration.zero())
        return;

    const auto seconds =
        std::chrono::duration_cast<std::chrono::seconds>(sleep_duration);  // NOLINT
    const auto nanoseconds =
        std::chrono::duration_cast<std::chrono::nanoseconds>(sleep_duration - seconds);  // NOLINT
    struct timespec sleepVal = {static_cast<std::time_t>(seconds.count()),
                                static_cast<long>(nanoseconds.count())};
    struct timespec remainVal;
    while (nanosleep(&sleepVal, &remainVal) == -1 && errno == EINTR) {
        sleepVal = remainVal;
    }
}

template <class Clock, class Duration>
void sleep_until(const std::chrono::time_point<Clock, Duration>& sleep_time) {  // NOLINT
    const auto now = Clock::now();
    sleep_for(sleep_time - now);
}
#endif
}  // namespace this_thread

}  // namespace stdx
}  // namespace mongo
