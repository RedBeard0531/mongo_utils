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

#if MONGO_CONFIG_SSL_PROVIDER == SSL_PROVIDER_APPLE

#include <CoreFoundation/CoreFoundation.h>
#include <Security/Security.h>
#include <memory>
#include <string>

namespace asio {
namespace ssl {
namespace apple {

namespace {
template <typename T>
struct CFReleaser {
    void operator()(T ptr) {
        if (ptr) {
            ::CFRelease(ptr);
        }
    }
};
}  // namespace

/**
 * CoreFoundation types are internally refcounted using CFRetain/CFRelease.
 * Values received from a method using the word "Copy" typically follow "The Copy Rule"
 * which requires that the caller explicitly invoke CFRelease on the obtained value.
 * Values received from a method using the word "Get" typically follow "The Get Rule"
 * which requires that the caller DOES NOT attempt to release any references,
 * though it may invoke CFRetain to hold on to the object for longer.
 *
 * Use of the CFUniquePtr type assumes that a value was wither obtained from a "Copy"
 * method, or that it has been explicitly retained.
 */
template <typename T>
using CFUniquePtr = std::unique_ptr<typename std::remove_pointer<T>::type, CFReleaser<T>>;

/**
 * Equivalent of OpenSSL's SSL_CTX type.
 * Allows loading SecIdentity and SecCertificate chains
 * separate from an SSLContext instance.
 *
 * Unlike OpenSSL, Secure Transport sets protocol range on
 * each connection instance separately, so just stash them aside
 * in the same place for now.
 */
struct Context {
    Context() = default;
    explicit Context(::SSLProtocol p) : protoMin(p), protoMax(p) {}
    Context& operator=(const Context& src) {
        protoMin = src.protoMin;
        protoMax = src.protoMax;
        if (src.certs) {
            ::CFRetain(src.certs.get());
        }
        certs.reset(src.certs.get());
        return *this;
    }

    ::SSLProtocol protoMin = kTLSProtocol1;
    ::SSLProtocol protoMax = kTLSProtocol12;
    CFUniquePtr<::CFArrayRef> certs;
};

}  // namespace apple
}  // namespace ssl
}  // namespace asio

#endif
