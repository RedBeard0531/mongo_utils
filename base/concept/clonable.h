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

#include "mongo/base/concept/constructible.h"
#include "mongo/base/concept/unique_ptr.h"

namespace mongo {
namespace concept {
/*!
 * Objects conforming to the Clonable concept can be dynamically copied, using `this->clone()`.
 * The Clonable concept does not specify the return type of the `clone()` function.
 */
struct Clonable {
    /*! Clonable objects must be safe to destroy, by pointer. */
    virtual ~Clonable() noexcept = 0;

    /*! Clonable objects can be cloned without knowing the actual dynamic type. */
    Constructible<UniquePtr<Clonable>> clone() const;
};
}  // namespace concept
}  // namespace mongo
