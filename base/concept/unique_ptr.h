/*-
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

#include "mongo/base/concept/convertible_to.h"

namespace mongo {
namespace concept {
/**
 * The `UniquePtr` Concept models a movable owning pointer of an object.
 * `std::unique_ptr< T >` is a model of `mongo::concept::UniquePtr< T >`.
 */
template <typename T>
struct UniquePtr {
    /** The `UniquePtr< T >` must retire its pointer to `T` on destruction. */
    ~UniquePtr() noexcept;

    UniquePtr(UniquePtr&& p);
    UniquePtr& operator=(UniquePtr&& p);

    UniquePtr();
    UniquePtr(T* p);

    ConvertibleTo<T*> operator->() const;
    T& operator*() const;

    explicit operator bool() const;

    ConvertibleTo<T*> get() const;

    void reset() noexcept;
    void reset(ConvertibleTo<T*>);
};

/*! A `UniquePtr` object must be equality comparable. */
template <typename T>
bool operator==(const UniquePtr<T>& lhs, const UniquePtr<T>& rhs);

/*! A `UniquePtr` object must be inequality comparable. */
template <typename T>
bool operator!=(const UniquePtr<T>& lhs, const UniquePtr<T>& rhs);
}  // namespace concept
}  // namespace mongo
