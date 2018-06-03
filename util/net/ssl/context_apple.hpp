/**
 *    Copyright 2018 MongoDB Inc.
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
#include "asio/detail/noncopyable.hpp"
#include "mongo/util/net/ssl/apple.hpp"
#include "mongo/util/net/ssl/context_base.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {
namespace ssl {

class context : public context_base, private noncopyable {
public:
    using native_handle_type = apple::Context*;

    ASIO_DECL explicit context(method m) {
        _context.protoMin = _mapProto(m);
        _context.protoMax = _context.protoMax;
    }

    ASIO_DECL context(context&& other) = default;
    ASIO_DECL context& operator=(context&& other) = default;

    ASIO_DECL native_handle_type native_handle() {
        return &_context;
    }

private:
    static ::SSLProtocol _mapProto(method m) {
        switch (m) {
            case context::tlsv1:
            case context::tlsv1_client:
            case context::tlsv1_server:
                return ::kTLSProtocol1;
            case context::tlsv11:
            case context::tlsv11_client:
            case context::tlsv11_server:
                return ::kTLSProtocol11;
            case context::tlsv12:
            case context::tlsv12_client:
            case context::tlsv12_server:
            default:
                return ::kTLSProtocol12;
        }
    }

    apple::Context _context;
};

}  // namespace ssl
}  // namespace asio

#include "asio/detail/pop_options.hpp"
