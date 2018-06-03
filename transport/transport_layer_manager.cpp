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

#include "mongo/platform/basic.h"

#include "mongo/transport/transport_layer_manager.h"

#include "mongo/base/status.h"
#include "mongo/db/server_options.h"
#include "mongo/db/service_context.h"
#include "mongo/stdx/memory.h"
#include "mongo/transport/service_executor_adaptive.h"
#include "mongo/transport/service_executor_synchronous.h"
#include "mongo/transport/session.h"
#include "mongo/transport/transport_layer_asio.h"
#include "mongo/util/net/ssl_types.h"
#include "mongo/util/time_support.h"
#include <limits>

#include <iostream>

namespace mongo {
namespace transport {

TransportLayerManager::TransportLayerManager() = default;

template <typename Callable>
void TransportLayerManager::_foreach(Callable&& cb) const {
    {
        stdx::lock_guard<stdx::mutex> lk(_tlsMutex);
        for (auto&& tl : _tls) {
            cb(tl.get());
        }
    }
}

StatusWith<SessionHandle> TransportLayerManager::connect(HostAndPort peer,
                                                         ConnectSSLMode sslMode,
                                                         Milliseconds timeout) {
    return _tls.front()->connect(peer, sslMode, timeout);
}

Future<SessionHandle> TransportLayerManager::asyncConnect(HostAndPort peer,
                                                          ConnectSSLMode sslMode,
                                                          const ReactorHandle& reactor) {
    return _tls.front()->asyncConnect(peer, sslMode, reactor);
}

ReactorHandle TransportLayerManager::getReactor(WhichReactor which) {
    return _tls.front()->getReactor(which);
}

// TODO Right now this and setup() leave TLs started if there's an error. In practice the server
// exits with an error and this isn't an issue, but we should make this more robust.
Status TransportLayerManager::start() {
    for (auto&& tl : _tls) {
        auto status = tl->start();
        if (!status.isOK()) {
            _tls.clear();
            return status;
        }
    }

    return Status::OK();
}

void TransportLayerManager::shutdown() {
    _foreach([](TransportLayer* tl) { tl->shutdown(); });
}

// TODO Same comment as start()
Status TransportLayerManager::setup() {
    for (auto&& tl : _tls) {
        auto status = tl->setup();
        if (!status.isOK()) {
            _tls.clear();
            return status;
        }
    }

    return Status::OK();
}

Status TransportLayerManager::addAndStartTransportLayer(std::unique_ptr<TransportLayer> tl) {
    auto ptr = tl.get();
    {
        stdx::lock_guard<stdx::mutex> lk(_tlsMutex);
        _tls.emplace_back(std::move(tl));
    }
    return ptr->start();
}

std::unique_ptr<TransportLayer> TransportLayerManager::makeAndStartDefaultEgressTransportLayer() {
    transport::TransportLayerASIO::Options opts(&serverGlobalParams);
    opts.mode = transport::TransportLayerASIO::Options::kEgress;

    auto ret = stdx::make_unique<transport::TransportLayerASIO>(opts, nullptr);
    uassertStatusOK(ret->setup());
    uassertStatusOK(ret->start());
    return std::unique_ptr<TransportLayer>(std::move(ret));
}

std::unique_ptr<TransportLayer> TransportLayerManager::createWithConfig(
    const ServerGlobalParams* config, ServiceContext* ctx) {
    std::unique_ptr<TransportLayer> transportLayer;
    auto sep = ctx->getServiceEntryPoint();

    transport::TransportLayerASIO::Options opts(config);
    if (config->serviceExecutor == "adaptive") {
        opts.transportMode = transport::Mode::kAsynchronous;
    } else if (config->serviceExecutor == "synchronous") {
        opts.transportMode = transport::Mode::kSynchronous;
    } else {
        MONGO_UNREACHABLE;
    }

    auto transportLayerASIO = stdx::make_unique<transport::TransportLayerASIO>(opts, sep);

    if (config->serviceExecutor == "adaptive") {
        auto reactor = transportLayerASIO->getReactor(TransportLayer::kIngress);
        ctx->setServiceExecutor(
            stdx::make_unique<ServiceExecutorAdaptive>(ctx, std::move(reactor)));
    } else if (config->serviceExecutor == "synchronous") {
        ctx->setServiceExecutor(stdx::make_unique<ServiceExecutorSynchronous>(ctx));
    }
    transportLayer = std::move(transportLayerASIO);

    std::vector<std::unique_ptr<TransportLayer>> retVector;
    retVector.emplace_back(std::move(transportLayer));
    return stdx::make_unique<TransportLayerManager>(std::move(retVector));
}

}  // namespace transport
}  // namespace mongo
