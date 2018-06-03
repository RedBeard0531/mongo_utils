/**
 *    Copyright (C) 2018 MongoDB Inc.
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

#define MONGO_LOG_DEFAULT_COMPONENT ::mongo::logger::LogComponent::kNetwork

#include "mongo/platform/basic.h"

#include "mongo/client/async_client.h"
#include "mongo/client/connection_string.h"
#include "mongo/db/client.h"
#include "mongo/db/service_context.h"
#include "mongo/stdx/thread.h"
#include "mongo/transport/session.h"
#include "mongo/transport/transport_layer.h"
#include "mongo/transport/transport_layer_asio.h"
#include "mongo/unittest/integration_test.h"
#include "mongo/unittest/unittest.h"
#include "mongo/util/fail_point_service.h"
#include "mongo/util/log.h"

#include "asio.hpp"

namespace mongo {
namespace {

TEST(TransportLayerASIO, HTTPRequestGetsHTTPError) {
    auto connectionString = unittest::getFixtureConnectionString();
    auto server = connectionString.getServers().front();

    asio::io_context ioContext;
    asio::ip::tcp::resolver resolver(ioContext);
    asio::ip::tcp::socket socket(ioContext);

    log() << "Connecting to " << server;
    auto resolverIt = resolver.resolve(server.host(), std::to_string(server.port()));
    asio::connect(socket, resolverIt);

    log() << "Sending HTTP request";
    std::string httpReq = str::stream() << "GET /\r\n"
                                           "Host: "
                                        << server << "\r\n"
                                                     "User-Agent: MongoDB Integration test\r\n"
                                                     "Accept: */*";
    asio::write(socket, asio::buffer(httpReq.data(), httpReq.size()));

    log() << "Waiting for response";
    std::array<char, 256> httpRespBuf;
    std::error_code ec;
    auto size = asio::read(socket, asio::buffer(httpRespBuf.data(), httpRespBuf.size()), ec);
    StringData httpResp(httpRespBuf.data(), size);

    log() << "Received response: \"" << httpResp << "\"";
    ASSERT_TRUE(httpResp.startsWith("HTTP/1.0 200 OK"));

// Why oh why can't ASIO unify their error codes
#ifdef _WIN32
    ASSERT_EQ(ec, asio::error::connection_reset);
#else
    ASSERT_EQ(ec, asio::error::eof);
#endif
}

// This test forces reads and writes to occur one byte at a time, verifying SERVER-34506 (the
// isJustForContinuation optimization works).
//
// Because of the file size limit, it's only an effective check on debug builds (where the future
// implementation checks the length of the future chain).
TEST(TransportLayerASIO, ShortReadsAndWritesWork) {
    const auto assertOK = [](executor::RemoteCommandResponse reply) {
        ASSERT_OK(reply.status);
        ASSERT(reply.data["ok"]) << reply.data;
    };

    auto connectionString = unittest::getFixtureConnectionString();
    auto server = connectionString.getServers().front();

    auto sc = getGlobalServiceContext();
    auto reactor = sc->getTransportLayer()->getReactor(transport::TransportLayer::kEgress);

    stdx::thread thread([&] { reactor->run(); });
    const auto threadGuard = MakeGuard([&] {
        reactor->stop();
        thread.join();
    });

    AsyncDBClient::Handle handle =
        AsyncDBClient::connect(server, transport::kGlobalSSLMode, sc, reactor).get();

    handle->initWireVersion(__FILE__, nullptr).get();

    FailPointEnableBlock fp("transportLayerASIOshortOpportunisticReadWrite");

    const executor::RemoteCommandRequest ecr{
        server, "admin", BSON("echo" << std::string(1 << 10, 'x')), BSONObj(), nullptr};

    assertOK(handle->runCommandRequest(ecr).get());

    auto client = sc->makeClient(__FILE__);
    auto opCtx = client->makeOperationContext();

    if (auto baton = sc->getTransportLayer()->makeBaton(opCtx.get())) {
        auto future = handle->runCommandRequest(ecr, baton);
        const auto batonGuard = MakeGuard([&] { baton->detach(); });

        while (!future.isReady()) {
            baton->run(nullptr, boost::none);
        }

        assertOK(future.get());
    }
}

}  // namespace
}  // namespace mongo
