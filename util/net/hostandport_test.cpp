/*    Copyright 2014 MongoDB, Inc.
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

#include "mongo/db/server_options.h"
#include "mongo/unittest/unittest.h"
#include "mongo/util/assert_util.h"
#include "mongo/util/net/hostandport.h"

namespace mongo {
namespace {

TEST(HostAndPort, BasicLessThanComparison) {
    // Not less than self.
    ASSERT_FALSE(HostAndPort("a", 1) < HostAndPort("a", 1));

    // Lex order by name.
    ASSERT_LESS_THAN(HostAndPort("a", 1), HostAndPort("b", 1));
    ASSERT_FALSE(HostAndPort("b", 1) < HostAndPort("a", 1));

    // Then, order by port number.
    ASSERT_LESS_THAN(HostAndPort("a", 1), HostAndPort("a", 2));
    ASSERT_FALSE(HostAndPort("a", 2) < HostAndPort("a", 1));
}

TEST(HostAndPort, BasicEquality) {
    // Comparison on host field
    ASSERT_EQUALS(HostAndPort("a", 1), HostAndPort("a", 1));
    ASSERT_FALSE(HostAndPort("b", 1) == HostAndPort("a", 1));
    ASSERT_FALSE(HostAndPort("a", 1) != HostAndPort("a", 1));
    ASSERT_NOT_EQUALS(HostAndPort("b", 1), HostAndPort("a", 1));

    // Comparison on port field
    ASSERT_FALSE(HostAndPort("a", 1) == HostAndPort("a", 2));
    ASSERT_NOT_EQUALS(HostAndPort("a", 1), HostAndPort("a", 2));
}

TEST(HostAndPort, ImplicitPortSelection) {
    ASSERT_EQUALS(HostAndPort("a", -1), HostAndPort("a", int(ServerGlobalParams::DefaultDBPort)));
    ASSERT_EQUALS(int(ServerGlobalParams::DefaultDBPort), HostAndPort("a", -1).port());
    ASSERT_FALSE(HostAndPort("a", -1).empty());
}

TEST(HostAndPort, ConstructorParsing) {
    ASSERT_THROWS(HostAndPort(""), AssertionException);
    ASSERT_THROWS(HostAndPort("a:"), AssertionException);
    ASSERT_THROWS(HostAndPort("a:0xa"), AssertionException);
    ASSERT_THROWS(HostAndPort(":123"), AssertionException);
    ASSERT_THROWS(HostAndPort("[124d:"), AssertionException);
    ASSERT_THROWS(HostAndPort("[124d:]asdf:34"), AssertionException);
    ASSERT_THROWS(HostAndPort("frim[124d:]:34"), AssertionException);
    ASSERT_THROWS(HostAndPort("[124d:]12:34"), AssertionException);
    ASSERT_THROWS(HostAndPort("124d:12:34"), AssertionException);

    ASSERT_EQUALS(HostAndPort("abc"), HostAndPort("abc", -1));
    ASSERT_EQUALS(HostAndPort("abc.def:3421"), HostAndPort("abc.def", 3421));
    ASSERT_EQUALS(HostAndPort("[124d:]:34"), HostAndPort("124d:", 34));
    ASSERT_EQUALS(HostAndPort("[124d:efg]:34"), HostAndPort("124d:efg", 34));
    ASSERT_EQUALS(HostAndPort("[124d:]"), HostAndPort("124d:", -1));
}

TEST(HostAndPort, StaticParseFunction) {
    ASSERT_EQUALS(ErrorCodes::FailedToParse, HostAndPort::parse("").getStatus());
    ASSERT_EQUALS(ErrorCodes::FailedToParse, HostAndPort::parse("a:").getStatus());
    ASSERT_EQUALS(ErrorCodes::FailedToParse, HostAndPort::parse("a:0").getStatus());
    ASSERT_EQUALS(ErrorCodes::FailedToParse, HostAndPort::parse("a:0xa").getStatus());
    ASSERT_EQUALS(ErrorCodes::FailedToParse, HostAndPort::parse(":123").getStatus());
    ASSERT_EQUALS(ErrorCodes::FailedToParse, HostAndPort::parse("[124d:").getStatus());
    ASSERT_EQUALS(ErrorCodes::FailedToParse, HostAndPort::parse("[124d:]asdf:34").getStatus());
    ASSERT_EQUALS(ErrorCodes::FailedToParse, HostAndPort::parse("124d:asdf:34").getStatus());
    ASSERT_EQUALS(ErrorCodes::FailedToParse, HostAndPort::parse("1234:").getStatus());
    ASSERT_EQUALS(ErrorCodes::FailedToParse, HostAndPort::parse("[[124d]]").getStatus());
    ASSERT_EQUALS(ErrorCodes::FailedToParse, HostAndPort::parse("[[124d]:34]").getStatus());

    ASSERT_EQUALS(unittest::assertGet(HostAndPort::parse("abc")), HostAndPort("abc", -1));
    ASSERT_EQUALS(unittest::assertGet(HostAndPort::parse("abc.def:3421")),
                  HostAndPort("abc.def", 3421));
    ASSERT_EQUALS(unittest::assertGet(HostAndPort::parse("[243:1bc]:21")),
                  HostAndPort("243:1bc", 21));
}

TEST(HostAndPort, RoundTripAbility) {
    ASSERT_EQUALS(HostAndPort("abc"), HostAndPort(HostAndPort("abc").toString()));
    ASSERT_EQUALS(HostAndPort("abc.def:3421"), HostAndPort(HostAndPort("abc.def:3421").toString()));
    ASSERT_EQUALS(HostAndPort("[124d:]:34"), HostAndPort(HostAndPort("[124d:]:34").toString()));
    ASSERT_EQUALS(HostAndPort("[124d:]"), HostAndPort(HostAndPort("[124d:]").toString()));
}

TEST(HostAndPort, CanIdentifyDefaultRoutes) {
    ASSERT_TRUE(HostAndPort("0.0.0.0").isDefaultRoute());
    ASSERT_FALSE(HostAndPort("127.0.0.1").isDefaultRoute());
    ASSERT_TRUE(HostAndPort("[::]").isDefaultRoute());
    ASSERT_FALSE(HostAndPort("[::1]").isDefaultRoute());
    ASSERT_TRUE(HostAndPort("[0:0:0:0:0:0:0:0]").isDefaultRoute());
    ASSERT_TRUE(HostAndPort("[0:0:0::0:0:0]").isDefaultRoute());
    ASSERT_TRUE(HostAndPort("[0:0:0::00:0:0]").isDefaultRoute());
}

}  // namespace
}  // namespace mongo
