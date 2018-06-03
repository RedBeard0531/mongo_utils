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

#define MONGO_LOG_DEFAULT_COMPONENT ::mongo::logger::LogComponent::kDefault

#include "mongo/platform/basic.h"

#include "mongo/base/init.h"
#include "mongo/db/service_context.h"
#include "mongo/transport/transport_layer_asio.h"

namespace mongo {
namespace {
// Linking with this file will configure an egress-only TransportLayer on a ServiceContextNoop.
// Use this for unit/integration tests that require only egress networking.
MONGO_INITIALIZER_WITH_PREREQUISITES(ConfigureEgressTransportLayer, ("ServiceContext"))
(InitializerContext* context) {
    auto sc = getGlobalServiceContext();
    invariant(!sc->getTransportLayer());

    transport::TransportLayerASIO::Options opts;
    opts.mode = transport::TransportLayerASIO::Options::kEgress;
    sc->setTransportLayer(std::make_unique<transport::TransportLayerASIO>(opts, nullptr));
    auto status = sc->getTransportLayer()->setup();
    if (!status.isOK()) {
        return status;
    }

    return sc->getTransportLayer()->start();
}

}  // namespace
}  // namespace
