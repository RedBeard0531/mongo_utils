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

#include <vector>

#include "mongo/base/status.h"
#include "mongo/stdx/mutex.h"
#include "mongo/transport/session.h"
#include "mongo/transport/transport_layer.h"
#include "mongo/util/time_support.h"

namespace mongo {
struct ServerGlobalParams;
class ServiceContext;

namespace transport {

/**
 * This TransportLayerManager is a TransportLayer implementation that holds other
 * TransportLayers. Mongod and Mongos can treat this like the "only" TransportLayer
 * and not be concerned with which other TransportLayer implementations it holds
 * underneath.
 */
class TransportLayerManager final : public TransportLayer {
    MONGO_DISALLOW_COPYING(TransportLayerManager);

public:
    TransportLayerManager(std::vector<std::unique_ptr<TransportLayer>> tls)
        : _tls(std::move(tls)) {}
    TransportLayerManager();

    StatusWith<SessionHandle> connect(HostAndPort peer,
                                      ConnectSSLMode sslMode,
                                      Milliseconds timeout) override;
    Future<SessionHandle> asyncConnect(HostAndPort peer,
                                       ConnectSSLMode sslMode,
                                       const ReactorHandle& reactor) override;

    Status start() override;
    void shutdown() override;
    Status setup() override;

    ReactorHandle getReactor(WhichReactor which) override;

    // TODO This method is not called anymore, but may be useful to add new TransportLayers
    // to the manager after it's been created.
    Status addAndStartTransportLayer(std::unique_ptr<TransportLayer> tl);

    /*
     * This initializes a TransportLayerManager with the global configuration of the server.
     *
     * To setup networking in mongod/mongos, create a TransportLayerManager with this function,
     * then call
     * tl->setup();
     * serviceContext->setTransportLayer(std::move(tl));
     * serviceContext->getTransportLayer->start();
     */
    static std::unique_ptr<TransportLayer> createWithConfig(const ServerGlobalParams* config,
                                                            ServiceContext* ctx);

    static std::unique_ptr<TransportLayer> makeAndStartDefaultEgressTransportLayer();

    BatonHandle makeBaton(OperationContext* opCtx) override {
        stdx::lock_guard<stdx::mutex> lk(_tlsMutex);
        // TODO: figure out what to do about managers with more than one transport layer.
        invariant(_tls.size() == 1);
        return _tls[0]->makeBaton(opCtx);
    }

private:
    template <typename Callable>
    void _foreach(Callable&& cb) const;

    mutable stdx::mutex _tlsMutex;
    std::vector<std::unique_ptr<TransportLayer>> _tls;
};

}  // namespace transport
}  // namespace mongo
