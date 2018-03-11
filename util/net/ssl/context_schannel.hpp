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

#include "asio/buffer.hpp"
#include "asio/io_context.hpp"
#include "mongo/util/net/ssl/context_base.hpp"
#include <string>

#include "asio/detail/push_options.hpp"

namespace asio {
namespace ssl {

class context : public context_base, private noncopyable {
public:
    /// The native handle type of the SSL context.
    typedef SCHANNEL_CRED* native_handle_type;

    /// Constructor.
    ASIO_DECL explicit context(method m);

#if defined(ASIO_HAS_MOVE) || defined(GENERATING_DOCUMENTATION)
    /// Move-construct a context from another.
    /**
    * This constructor moves an SSL context from one object to another.
    *
    * @param other The other context object from which the move will occur.
    *
    * @note Following the move, the following operations only are valid for the
    * moved-from object:
    * @li Destruction.
    * @li As a target for move-assignment.
    */
    ASIO_DECL context(context&& other);

    /// Move-assign a context from another.
    /**
    * This assignment operator moves an SSL context from one object to another.
    *
    * @param other The other context object from which the move will occur.
    *
    * @note Following the move, the following operations only are valid for the
    * moved-from object:
    * @li Destruction.
    * @li As a target for move-assignment.
    */
    ASIO_DECL context& operator=(context&& other);
#endif  // defined(ASIO_HAS_MOVE) || defined(GENERATING_DOCUMENTATION)

    /// Destructor.
    ASIO_DECL ~context();

    /// Get the underlying implementation in the native type.
    /**
    * This function may be used to obtain the underlying implementation of the
    * context. This is intended to allow access to context functionality that is
    * not otherwise provided.
    */
    ASIO_DECL native_handle_type native_handle();

private:
    SCHANNEL_CRED _cred;

    // The underlying native implementation.
    native_handle_type handle_;
};

#include "asio/detail/pop_options.hpp"

#if defined(ASIO_HEADER_ONLY)
#include "mongo/util/net/ssl/impl/context_schannel.ipp"
#endif  // defined(ASIO_HEADER_ONLY)

}  // namespace ssl
}  // namespace asio
