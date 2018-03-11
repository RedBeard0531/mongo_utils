/**
 * Copyright (C) 2018 MongoDB Inc.
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

#include "asio/detail/config.hpp"

#include "asio/detail/throw_error.hpp"
#include "asio/error.hpp"
#include "mongo/util/net/ssl/context.hpp"
#include "mongo/util/net/ssl/error.hpp"
#include <cstring>

#include "asio/detail/push_options.hpp"

namespace asio {
namespace ssl {


context::context(context::method m) : handle_(&_cred) {
    memset(&_cred, 0, sizeof(_cred));
}

#if defined(ASIO_HAS_MOVE) || defined(GENERATING_DOCUMENTATION)
context::context(context&& other) {
    handle_ = other.handle_;
    other.handle_ = 0;
}

context& context::operator=(context&& other) {
    context tmp(ASIO_MOVE_CAST(context)(*this));
    handle_ = other.handle_;
    other.handle_ = 0;
    return *this;
}
#endif  // defined(ASIO_HAS_MOVE) || defined(GENERATING_DOCUMENTATION)

context::~context() {}

context::native_handle_type context::native_handle() {
    return handle_;
}

}  // namespace ssl
}  // namespace asio

#include "asio/detail/pop_options.hpp"
