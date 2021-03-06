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

#include "mongo/platform/basic.h"

#include "mongo/util/net/ssl_manager.h"

#include "mongo/base/init.h"

namespace mongo {
namespace {
MONGO_INITIALIZER(SSLManager)(InitializerContext*) {
    // we need a no-op initializer so that we can depend on SSLManager as a prerequisite in
    // non-SSL builds.
    return Status::OK();
}
}  // namespace
}  // namespace mongo
