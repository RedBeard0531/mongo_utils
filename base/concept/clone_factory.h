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

#include "mongo/base/concept/assignable.h"
#include "mongo/base/concept/constructible.h"
#include "mongo/base/concept/unique_ptr.h"

namespace mongo {
namespace concept {
/*!
 * Objects conforming to the `CloneFactory` concept are function-like constructs which return
 * objects that are dynamically allocated copies of their inputs.
 * These copies can be made without knowing the actual dynamic type.  The `CloneFactory` type itself
 * must be `Assignable`, in that it can be used with automatically generated copy constructors and
 * copy assignment operators.
 */
template <typename T>
struct CloneFactory : Assignable {
    Constructible<UniquePtr<T>> operator()(const T*) const;
};
}  // namespace concept
}  // namespace mongo
