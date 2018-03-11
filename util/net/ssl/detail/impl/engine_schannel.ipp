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
#include "mongo/util/net/ssl/detail/engine.hpp"
#include "mongo/util/net/ssl/error.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {
namespace ssl {
namespace detail {


engine::engine(SCHANNEL_CRED* context)
    : _pCred(context),
      _hcxt{0, 0},
      _hcred{0, 0},
      _inBuffer(kDefaultBufferSize),
      _outBuffer(kDefaultBufferSize),
      _extraBuffer(kDefaultBufferSize),
      _handshakeManager(
          &_hcxt, &_hcred, _serverName, &_inBuffer, &_outBuffer, &_extraBuffer, _pCred),
      _readManager(&_hcxt, &_hcred, &_inBuffer, &_extraBuffer),
      _writeManager(&_hcxt, &_outBuffer) {}

engine::~engine() {
    DeleteSecurityContext(&_hcxt);
    FreeCredentialsHandle(&_hcred);
}

PCtxtHandle engine::native_handle() {
    return &_hcxt;
}

engine::want ssl_want_to_engine(ssl_want want) {
    static_assert(static_cast<int>(ssl_want::want_input_and_retry) ==
                      static_cast<int>(engine::want_input_and_retry),
                  "bad");
    static_assert(static_cast<int>(ssl_want::want_output_and_retry) ==
                      static_cast<int>(engine::want_output_and_retry),
                  "bad");
    static_assert(
        static_cast<int>(ssl_want::want_nothing) == static_cast<int>(engine::want_nothing), "bad");
    static_assert(static_cast<int>(ssl_want::want_output) == static_cast<int>(engine::want_output),
                  "bad");

    return static_cast<engine::want>(want);
}

engine::want engine::handshake(stream_base::handshake_type type, asio::error_code& ec) {
    // ASIO will call handshake once more after we send out the last data
    // so we need to tell them we are done with data to send.
    if (_state != EngineState::NeedsHandshake) {
        return want::want_nothing;
    }

    _handshakeManager.setMode((type == asio::ssl::stream_base::client)
                                  ? SSLHandshakeManager::HandshakeMode::Client
                                  : SSLHandshakeManager::HandshakeMode::Server);
    SSLHandshakeManager::HandshakeState state;
    auto w = _handshakeManager.nextHandshake(ec, &state);
    if (w == ssl_want::want_nothing || state == SSLHandshakeManager::HandshakeState::Done) {
        _state = EngineState::InProgress;
    }

    return ssl_want_to_engine(w);
}

engine::want engine::shutdown(asio::error_code& ec) {
    return ssl_want_to_engine(_handshakeManager.beginShutdown(ec));
}

engine::want engine::write(const asio::const_buffer& data,
                           asio::error_code& ec,
                           std::size_t& bytes_transferred) {
    if (data.size() == 0) {
        ec = asio::error_code();
        return engine::want_nothing;
    }

    if (_state == EngineState::NeedsHandshake || _state == EngineState::InShutdown) {
        // Why are we trying to write before the handshake is done?
        ASIO_ASSERT(false);
        return want::want_nothing;
    } else {
        return ssl_want_to_engine(
            _writeManager.writeUnencryptedData(data.data(), data.size(), bytes_transferred, ec));
    }
}

engine::want engine::read(const asio::mutable_buffer& data,
                          asio::error_code& ec,
                          std::size_t& bytes_transferred) {
    if (data.size() == 0) {
        ec = asio::error_code();
        return engine::want_nothing;
    }


    if (_state == EngineState::NeedsHandshake) {
        // Why are we trying to read before the handshake is done?
        ASIO_ASSERT(false);
        return want::want_nothing;
    } else {
        SSLReadManager::DecryptState decryptState;
        auto want = ssl_want_to_engine(_readManager.readDecryptedData(
            data.data(), data.size(), ec, bytes_transferred, &decryptState));
        if (ec) {
            return want;
        }

        if (decryptState != SSLReadManager::DecryptState::Continue) {
            if (decryptState == SSLReadManager::DecryptState::Shutdown) {
                _state = EngineState::InShutdown;

                return ssl_want_to_engine(_handshakeManager.beginShutdown(ec));
            }
        }

        return want;
    }
}

asio::mutable_buffer engine::get_output(const asio::mutable_buffer& data) {
    std::size_t length;
    _outBuffer.readInto(data.data(), data.size(), length);

    return asio::buffer(data, length);
}

asio::const_buffer engine::put_input(const asio::const_buffer& data) {
    if (_state == EngineState::NeedsHandshake) {
        _handshakeManager.writeEncryptedData(data.data(), data.size());
    } else {
        _readManager.writeData(data.data(), data.size());
    }

    return asio::buffer(data + data.size());
}

void engine::set_server_name(const std::wstring name) {
    _serverName = name;
}

const asio::error_code& engine::map_error_code(asio::error_code& ec) const {
    return ec;
}

#include "asio/detail/pop_options.hpp"

}  // namespace detail
}  // namespace ssl
}  // namespace asio
