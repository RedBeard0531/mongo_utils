/**
 *    Copyright (C) 2017 MongoDB Inc.
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

#include "mongo/client/dbclientinterface.h"
#include "mongo/rpc/get_status_from_command_result.h"
#include "mongo/unittest/integration_test.h"
#include "mongo/unittest/unittest.h"
#include "mongo/util/net/op_msg.h"
#include "mongo/util/scopeguard.h"


namespace mongo {

TEST(OpMsg, UnknownRequiredFlagClosesConnection) {
    std::string errMsg;
    auto conn = std::unique_ptr<DBClientBase>(
        unittest::getFixtureConnectionString().connect("integration_test", errMsg));
    uassert(ErrorCodes::SocketException, errMsg, conn);

    auto request = OpMsgRequest::fromDBAndBody("admin", BSON("ping" << 1)).serialize();
    OpMsg::setFlag(&request, 1u << 15);  // This should be the last required flag to be assigned.

    Message reply;
    ASSERT(!conn->call(request, reply, /*assertOK*/ false));
}

TEST(OpMsg, UnknownOptionalFlagIsIgnored) {
    std::string errMsg;
    auto conn = std::unique_ptr<DBClientBase>(
        unittest::getFixtureConnectionString().connect("integration_test", errMsg));
    uassert(ErrorCodes::SocketException, errMsg, conn);

    auto request = OpMsgRequest::fromDBAndBody("admin", BSON("ping" << 1)).serialize();
    OpMsg::setFlag(&request, 1u << 31);  // This should be the last optional flag to be assigned.

    Message reply;
    ASSERT(conn->call(request, reply));
    uassertStatusOK(getStatusFromCommandResult(
        conn->parseCommandReplyMessage(conn->getServerAddress(), reply)->getCommandReply()));
}

TEST(OpMsg, FireAndForgetInsertWorks) {
    std::string errMsg;
    auto conn = std::unique_ptr<DBClientBase>(
        unittest::getFixtureConnectionString().connect("integration_test", errMsg));
    uassert(ErrorCodes::SocketException, errMsg, conn);

    conn->dropCollection("test.collection");

    conn->runFireAndForgetCommand(OpMsgRequest::fromDBAndBody("test", fromjson(R"({
        insert: "collection",
        writeConcern: {w: 0},
        documents: [
            {a: 1}
        ]
    })")));

    ASSERT_EQ(conn->count("test.collection"), 1u);
}

TEST(OpMsg, CloseConnectionOnFireAndForgetNotMasterError) {
    const auto connStr = unittest::getFixtureConnectionString();

    // This test only works against a replica set.
    if (connStr.type() != ConnectionString::SET) {
        return;
    }

    bool foundSecondary = false;
    for (auto host : connStr.getServers()) {
        DBClientConnection conn;
        uassertStatusOK(conn.connect(host, "integration_test"));
        bool isMaster;
        ASSERT(conn.isMaster(isMaster));
        if (isMaster)
            continue;
        foundSecondary = true;

        auto request = OpMsgRequest::fromDBAndBody("test", fromjson(R"({
            insert: "collection",
            writeConcern: {w: 0},
            documents: [
                {a: 1}
            ]
        })")).serialize();

        // Round-trip command fails with NotMaster error. Note that this failure is in command
        // dispatch which ignores w:0.
        Message reply;
        ASSERT(conn.call(request, reply, /*assertOK*/ true, nullptr));
        ASSERT_EQ(
            getStatusFromCommandResult(
                conn.parseCommandReplyMessage(conn.getServerAddress(), reply)->getCommandReply()),
            ErrorCodes::NotMaster);

        // Fire-and-forget closes connection when it sees that error. Note that this is using call()
        // rather than say() so that we get an error back when the connection is closed. Normally
        // using call() if kMoreToCome set results in blocking forever.
        OpMsg::setFlag(&request, OpMsg::kMoreToCome);
        ASSERT(!conn.call(request, reply, /*assertOK*/ false, nullptr));

        uassertStatusOK(conn.connect(host, "integration_test"));  // Reconnect.

        // Disable eager checking of master to simulate a stepdown occurring after the check. This
        // should respect w:0.
        BSONObj output;
        ASSERT(conn.runCommand("admin",
                               fromjson(R"({
                                   configureFailPoint: 'skipCheckingForNotMasterInCommandDispatch',
                                   mode: 'alwaysOn'
                               })"),
                               output))
            << output;
        ON_BLOCK_EXIT([&] {
            uassertStatusOK(conn.connect(host, "integration_test-cleanup"));
            ASSERT(conn.runCommand("admin",
                                   fromjson(R"({
                                          configureFailPoint:
                                              'skipCheckingForNotMasterInCommandDispatch',
                                          mode: 'off'
                                      })"),
                                   output))
                << output;
        });


        // Round-trip command claims to succeed due to w:0.
        OpMsg::replaceFlags(&request, 0);
        ASSERT(conn.call(request, reply, /*assertOK*/ true, nullptr));
        ASSERT_OK(getStatusFromCommandResult(
            conn.parseCommandReplyMessage(conn.getServerAddress(), reply)->getCommandReply()));

        // Fire-and-forget should still close connection.
        OpMsg::setFlag(&request, OpMsg::kMoreToCome);
        ASSERT(!conn.call(request, reply, /*assertOK*/ false, nullptr));

        break;
    }
    ASSERT(foundSecondary);
}

}  // namespace mongo
