/**
 *    Copyright (C) 2012 10gen Inc.
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

#include <boost/range/size.hpp>
#include <string>
#include <vector>

#include "mongo/db/jsobj.h"
#include "mongo/unittest/unittest.h"
#include "mongo/util/cmdline_utils/censor_cmdline.h"
#include "mongo/util/options_parser/environment.h"

namespace mongo {

namespace {

namespace moe = mongo::optionenvironment;

void testCensoringArgv(const char* const* expected, const char* const* toCensor, int elementCount) {
    std::vector<std::string> toCensorStringVec(toCensor, toCensor + elementCount);
    std::vector<char*> arrayStandin;
    for (size_t i = 0; i < toCensorStringVec.size(); ++i)
        arrayStandin.push_back(&*toCensorStringVec[i].begin());

    char** argv = &*arrayStandin.begin();

    cmdline_utils::censorArgvArray(elementCount, argv);

    for (int i = 0; i < elementCount; ++i) {
        ASSERT_EQUALS(std::string(expected[i]), std::string(argv[i]));
    }
}

void testCensoringVector(const char* const* expected,
                         const char* const* toCensor,
                         int elementCount) {
    std::vector<std::string> actual(toCensor, toCensor + elementCount);

    cmdline_utils::censorArgsVector(&actual);

    for (int i = 0; i < elementCount; ++i) {
        ASSERT_EQUALS(std::string(expected[i]), actual[i]);
    }
}

TEST(ArgvCensorTests, NothingCensored) {
    const char* const argv[] = {"first",
                                "second",
                                "sslPEMKeyPassword=KEEP",
                                "---sslPEMKeyPassword=KEEP",
                                "sslPEMKeyPassword",
                                "KEEP",
                                "servicePassword=KEEP",
                                "--servicePassword-",
                                "KEEP",
                                "--servicePasswordFake=KEEP"};
    const int argc = boost::size(argv);
    testCensoringArgv(argv, argv, argc);
}

TEST(ArgvCensorTests, SomeStuffCensoredDoubleHyphen) {
    const char* const argv[] = {"first",
                                "second",
                                "--sslPEMKeyPassword=LOSEME",
                                "--sslPEMKeyPassword",
                                "Really, loose me!",
                                "--servicePassword=bad news",
                                "--servicePassword-",
                                "KEEP",
                                "--servicePassword",
                                "get out of dodge"};
    const int argc = boost::size(argv);

    const char* const expected[] = {"first",
                                    "second",
                                    "--sslPEMKeyPassword=xxxxxx",
                                    "--sslPEMKeyPassword",
                                    "xxxxxxxxxxxxxxxxx",
                                    "--servicePassword=xxxxxxxx",
                                    "--servicePassword-",
                                    "KEEP",
                                    "--servicePassword",
                                    "xxxxxxxxxxxxxxxx"};
    ASSERT_EQUALS(static_cast<int>(boost::size(expected)), argc);

    testCensoringArgv(expected, argv, argc);
}

TEST(ArgvCensorTests, SomeStuffCensoredSingleHyphen) {
    const char* const argv[] = {"first",
                                "second",
                                "-sslPEMKeyPassword=LOSEME",
                                "-sslPEMKeyPassword",
                                "Really, loose me!",
                                "-servicePassword=bad news",
                                "-servicePassword-",
                                "KEEP",
                                "-servicePassword",
                                "get out of dodge"};
    const int argc = boost::size(argv);

    const char* const expected[] = {"first",
                                    "second",
                                    "-sslPEMKeyPassword=xxxxxx",
                                    "-sslPEMKeyPassword",
                                    "xxxxxxxxxxxxxxxxx",
                                    "-servicePassword=xxxxxxxx",
                                    "-servicePassword-",
                                    "KEEP",
                                    "-servicePassword",
                                    "xxxxxxxxxxxxxxxx"};
    ASSERT_EQUALS(static_cast<int>(boost::size(expected)), argc);

    testCensoringArgv(expected, argv, argc);
}

TEST(VectorCensorTests, NothingCensored) {
    const char* const argv[] = {"first",
                                "second",
                                "sslPEMKeyPassword=KEEP",
                                "---sslPEMKeyPassword=KEEP",
                                "sslPEMKeyPassword",
                                "KEEP",
                                "servicePassword=KEEP",
                                "--servicePassword-",
                                "KEEP",
                                "--servicePasswordFake=KEEP"};
    const int argc = boost::size(argv);
    testCensoringVector(argv, argv, argc);
}

TEST(VectorCensorTests, SomeStuffCensoredDoubleHyphen) {
    const char* const argv[] = {"first",
                                "second",
                                "--sslPEMKeyPassword=LOSEME",
                                "--sslPEMKeyPassword",
                                "Really, loose me!",
                                "--servicePassword=bad news",
                                "--servicePassword-",
                                "KEEP",
                                "--servicePassword",
                                "get out of dodge"};
    const int argc = boost::size(argv);

    const char* const expected[] = {"first",
                                    "second",
                                    "--sslPEMKeyPassword=<password>",
                                    "--sslPEMKeyPassword",
                                    "<password>",
                                    "--servicePassword=<password>",
                                    "--servicePassword-",
                                    "KEEP",
                                    "--servicePassword",
                                    "<password>"};
    ASSERT_EQUALS(static_cast<int>(boost::size(expected)), argc);

    testCensoringVector(expected, argv, argc);
}

TEST(VectorCensorTests, SomeStuffCensoredSingleHyphen) {
    const char* const argv[] = {"first",
                                "second",
                                "-sslPEMKeyPassword=LOSEME",
                                "-sslPEMKeyPassword",
                                "Really, loose me!",
                                "-servicePassword=bad news",
                                "-servicePassword-",
                                "KEEP",
                                "-servicePassword",
                                "get out of dodge"};
    const int argc = boost::size(argv);

    const char* const expected[] = {"first",
                                    "second",
                                    "-sslPEMKeyPassword=<password>",
                                    "-sslPEMKeyPassword",
                                    "<password>",
                                    "-servicePassword=<password>",
                                    "-servicePassword-",
                                    "KEEP",
                                    "-servicePassword",
                                    "<password>"};
    ASSERT_EQUALS(static_cast<int>(boost::size(expected)), argc);

    testCensoringVector(expected, argv, argc);
}

TEST(BSONObjCensorTests, Strings) {
    BSONObj obj = BSON("firstarg"
                       << "not a password"
                       << "net.ssl.PEMKeyPassword"
                       << "this password should be censored"
                       << "net.ssl.clusterPassword"
                       << "this password should be censored"
                       << "middlearg"
                       << "also not a password"
                       << "processManagement.windowsService.servicePassword"
                       << "this password should also be censored"
                       << "lastarg"
                       << false);

    BSONObj res = BSON("firstarg"
                       << "not a password"
                       << "net.ssl.PEMKeyPassword"
                       << "<password>"
                       << "net.ssl.clusterPassword"
                       << "<password>"
                       << "middlearg"
                       << "also not a password"
                       << "processManagement.windowsService.servicePassword"
                       << "<password>"
                       << "lastarg"
                       << false);

    cmdline_utils::censorBSONObj(&obj);
    ASSERT_BSONOBJ_EQ(res, obj);
}

TEST(BSONObjCensorTests, Arrays) {
    BSONObj obj = BSON("firstarg"
                       << "not a password"
                       << "net.ssl.PEMKeyPassword"
                       << BSON_ARRAY("first censored password"
                                     << "next censored password")
                       << "net.ssl.clusterPassword"
                       << BSON_ARRAY("first censored password"
                                     << "next censored password")
                       << "middlearg"
                       << "also not a password"
                       << "processManagement.windowsService.servicePassword"
                       << BSON_ARRAY("first censored password"
                                     << "next censored password")
                       << "lastarg"
                       << false);

    BSONObj res = BSON("firstarg"
                       << "not a password"
                       << "net.ssl.PEMKeyPassword"
                       << BSON_ARRAY("<password>"
                                     << "<password>")
                       << "net.ssl.clusterPassword"
                       << BSON_ARRAY("<password>"
                                     << "<password>")
                       << "middlearg"
                       << "also not a password"
                       << "processManagement.windowsService.servicePassword"
                       << BSON_ARRAY("<password>"
                                     << "<password>")
                       << "lastarg"
                       << false);

    cmdline_utils::censorBSONObj(&obj);
    ASSERT_BSONOBJ_EQ(res, obj);
}

TEST(BSONObjCensorTests, SubObjects) {
    BSONObj obj =
        BSON("firstarg"
             << "not a password"
             << "net"
             << BSON("ssl" << BSON("PEMKeyPassword" << BSON_ARRAY("first censored password"
                                                                  << "next censored password")
                                                    << "PEMKeyPassword"
                                                    << "should be censored too"
                                                    << "clusterPassword"
                                                    << BSON_ARRAY("first censored password"
                                                                  << "next censored password")
                                                    << "clusterPassword"
                                                    << "should be censored too"))
             << "lastarg"
             << false);

    BSONObj res = BSON("firstarg"
                       << "not a password"
                       << "net"
                       << BSON("ssl" << BSON("PEMKeyPassword" << BSON_ARRAY("<password>"
                                                                            << "<password>")
                                                              << "PEMKeyPassword"
                                                              << "<password>"
                                                              << "clusterPassword"
                                                              << BSON_ARRAY("<password>"
                                                                            << "<password>")
                                                              << "clusterPassword"
                                                              << "<password>"))
                       << "lastarg"
                       << false);

    cmdline_utils::censorBSONObj(&obj);
    ASSERT_BSONOBJ_EQ(res, obj);
}

}  // namespace
}  // namespace mongo
