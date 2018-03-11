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

#include <type_traits>

#include "mongo/config.h"

#if defined(MONGO_CONFIG_HAVE_STD_ENABLE_IF_T)

namespace mongo {
namespace stdx {

using ::std::enable_if_t;

}  // namespace stdx
}  // namespace mongo

#else

namespace mongo {
namespace stdx {

template <bool B, class T = void>
using enable_if_t = typename std::enable_if<B, T>::type;

}  // namespace stdx
}  // namespace mongo
#endif

#if __cplusplus >= 201703

namespace mongo {
namespace stdx {

using std::void_t;

}  // namespace stdx
}  // namespace mongo

#else

namespace mongo {
namespace stdx {

template <typename...>
struct make_void {
    using type = void;
};

template <typename... Args>
using void_t = typename make_void<Args...>::type;

}  // namespace stdx
}  // namespace mongo
#endif
