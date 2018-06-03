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

#if MONGO_CONFIG_SSL_PROVIDER == SSL_PROVIDER_WINDOWS

#include "mongo/util/net/ssl/detail/engine_schannel.hpp"

#elif MONGO_CONFIG_SSL_PROVIDER == SSL_PROVIDER_OPENSSL

#include "mongo/util/net/ssl/detail/engine_openssl.hpp"

#elif MONGO_CONFIG_SSL_PROVIDER == SSL_PROVIDER_APPLE

#include "mongo/util/net/ssl/detail/engine_apple.hpp"

#else
#error "Unknown SSL Provider"
#endif
