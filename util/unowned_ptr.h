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
#include <type_traits>

namespace mongo {

/**
 * A "smart" pointer that explicitly indicates a lack of ownership.
 * It will implicitly convert from any compatible pointer type except auto_ptr.
 *
 * Note that like other pointer types const applies to the pointer not the pointee:
 * - const unowned_ptr<T>  =>  T* const
 * - unowned_ptr<const T>  =>  const T*
 */
template <typename T>
struct unowned_ptr {
    unowned_ptr() = default;

    //
    // Implicit conversions from compatible pointer types
    //

    // Removes conversions from overload resolution if the underlying pointer types aren't
    // convertible. This makes this class behave more like a bare pointer.
    template <typename U>
    using IfConvertibleFrom = typename std::enable_if<std::is_convertible<U*, T*>::value>::type;

    // Needed for NULL since it won't match U* constructor.
    unowned_ptr(T* p) : _p(p) {}

    template <typename U, typename = IfConvertibleFrom<U>>
    unowned_ptr(U* p) : _p(p) {}

    template <typename U, typename = IfConvertibleFrom<U>>
    unowned_ptr(const unowned_ptr<U>& p) : _p(p) {}

    template <typename U, typename Deleter, typename = IfConvertibleFrom<U>>
    unowned_ptr(const std::unique_ptr<U, Deleter>& p) : _p(p.get()) {}

    template <typename U, typename = IfConvertibleFrom<U>>
    unowned_ptr(const std::shared_ptr<U>& p) : _p(p.get()) {}

    //
    // Modifiers
    //

    void reset(unowned_ptr p = nullptr) {
        _p = p.get();
    }
    void swap(unowned_ptr& other) {
        std::swap(_p, other._p);
    }

    //
    // Accessors
    //

    T* get() const {
        return _p;
    }
    operator T*() const {
        return _p;
    }

    //
    // Pointer syntax
    //

    T* operator->() const {
        return _p;
    }
    T& operator*() const {
        return *_p;
    }

private:
    T* _p = nullptr;
};

}  // namespace mongo
