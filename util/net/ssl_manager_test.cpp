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

#define MONGO_LOG_DEFAULT_COMPONENT ::mongo::logger::LogComponent::kNetwork

#include "mongo/platform/basic.h"

#include "mongo/util/net/ssl_manager.h"

#include "mongo/config.h"
#include "mongo/unittest/unittest.h"
#include "mongo/util/log.h"


namespace mongo {
namespace {
TEST(SSLManager, matchHostname) {
#ifdef MONGO_CONFIG_SSL
    enum Expected : bool { match = true, mismatch = false };
    const struct {
        Expected expected;
        std::string hostname;
        std::string certName;
    } tests[] = {
        // clang-format off
        // Matches?  |    Hostname and possibly FQDN   |  Certificate name
        {match,                    "foo.bar.bas" ,           "*.bar.bas."},
        {mismatch,       "foo.subdomain.bar.bas" ,           "*.bar.bas."},
        {match,                    "foo.bar.bas.",           "*.bar.bas."},
        {mismatch,       "foo.subdomain.bar.bas.",           "*.bar.bas."},

        {match,                    "foo.bar.bas" ,           "*.bar.bas"},
        {mismatch,       "foo.subdomain.bar.bas" ,           "*.bar.bas"},
        {match,                    "foo.bar.bas.",           "*.bar.bas"},
        {mismatch,       "foo.subdomain.bar.bas.",           "*.bar.bas"},

        {mismatch,                "foo.evil.bas" ,           "*.bar.bas."},
        {mismatch,      "foo.subdomain.evil.bas" ,           "*.bar.bas."},
        {mismatch,                "foo.evil.bas.",           "*.bar.bas."},
        {mismatch,      "foo.subdomain.evil.bas.",           "*.bar.bas."},

        {mismatch,                "foo.evil.bas" ,           "*.bar.bas"},
        {mismatch,      "foo.subdomain.evil.bas" ,           "*.bar.bas"},
        {mismatch,                "foo.evil.bas.",           "*.bar.bas"},
        {mismatch,      "foo.subdomain.evil.bas.",           "*.bar.bas"},
        // clang-format on
    };
    bool failure = false;
    for (const auto& test : tests) {
        if (bool(test.expected) != hostNameMatchForX509Certificates(test.hostname, test.certName)) {
            failure = true;
            LOG(1) << "Failure for Hostname: " << test.hostname
                   << " Certificate: " << test.certName;
        } else {
            LOG(1) << "Passed for Hostname: " << test.hostname << " Certificate: " << test.certName;
        }
    }
    ASSERT_FALSE(failure);
#endif
}
}

#ifdef MONGO_CONFIG_SSL

std::vector<RoleName> getSortedRoles(const stdx::unordered_set<RoleName>& roles) {
    std::vector<RoleName> vec;
    vec.reserve(roles.size());
    std::copy(roles.begin(), roles.end(), std::back_inserter<std::vector<RoleName>>(vec));
    std::sort(vec.begin(), vec.end());
    return vec;
}

TEST(SSLManager, MongoDBRolesParser) {
    /*
    openssl asn1parse -genconf mongodbroles.cnf -out foo.der

    -------- mongodbroles.cnf --------
    asn1 = SET:MongoDBAuthorizationGrant

    [MongoDBAuthorizationGrant]
    grant1 = SEQUENCE:MongoDBRole

    [MongoDBRole]
    role  = UTF8:role_name
    database = UTF8:Third field
    */
    // Positive: Simple parsing test
    {
        unsigned char derData[] = {0x31, 0x1a, 0x30, 0x18, 0x0c, 0x09, 0x72, 0x6f, 0x6c, 0x65,
                                   0x5f, 0x6e, 0x61, 0x6d, 0x65, 0x0c, 0x0b, 0x54, 0x68, 0x69,
                                   0x72, 0x64, 0x20, 0x66, 0x69, 0x65, 0x6c, 0x64};
        auto swPeer = parsePeerRoles(ConstDataRange(reinterpret_cast<char*>(derData),
                                                    std::extent<decltype(derData)>::value));
        ASSERT_OK(swPeer.getStatus());
        auto item = *(swPeer.getValue().begin());
        ASSERT_EQ(item.getRole(), "role_name");
        ASSERT_EQ(item.getDB(), "Third field");
    }

    // Positive: Very long role_name, and long form lengths
    {
        unsigned char derData[] = {
            0x31, 0x82, 0x01, 0x3e, 0x30, 0x82, 0x01, 0x3a, 0x0c, 0x82, 0x01, 0x29, 0x72, 0x6f,
            0x6c, 0x65, 0x5f, 0x6e, 0x61, 0x6d, 0x65, 0x72, 0x6f, 0x6c, 0x65, 0x5f, 0x6e, 0x61,
            0x6d, 0x65, 0x72, 0x6f, 0x6c, 0x65, 0x5f, 0x6e, 0x61, 0x6d, 0x65, 0x72, 0x6f, 0x6c,
            0x65, 0x5f, 0x6e, 0x61, 0x6d, 0x65, 0x72, 0x6f, 0x6c, 0x65, 0x5f, 0x6e, 0x61, 0x6d,
            0x65, 0x72, 0x6f, 0x6c, 0x65, 0x5f, 0x6e, 0x61, 0x6d, 0x65, 0x72, 0x6f, 0x6c, 0x65,
            0x5f, 0x6e, 0x61, 0x6d, 0x65, 0x72, 0x6f, 0x6c, 0x65, 0x5f, 0x6e, 0x61, 0x6d, 0x65,
            0x72, 0x6f, 0x6c, 0x65, 0x5f, 0x6e, 0x61, 0x6d, 0x65, 0x72, 0x6f, 0x6c, 0x65, 0x5f,
            0x6e, 0x61, 0x6d, 0x65, 0x72, 0x6f, 0x6c, 0x65, 0x5f, 0x6e, 0x61, 0x6d, 0x65, 0x72,
            0x6f, 0x6c, 0x65, 0x5f, 0x6e, 0x61, 0x6d, 0x65, 0x72, 0x6f, 0x6c, 0x65, 0x5f, 0x6e,
            0x61, 0x6d, 0x65, 0x72, 0x6f, 0x6c, 0x65, 0x5f, 0x6e, 0x61, 0x6d, 0x65, 0x72, 0x6f,
            0x6c, 0x65, 0x5f, 0x6e, 0x61, 0x6d, 0x65, 0x72, 0x6f, 0x6c, 0x65, 0x5f, 0x6e, 0x61,
            0x6d, 0x65, 0x72, 0x6f, 0x6c, 0x65, 0x5f, 0x6e, 0x61, 0x6d, 0x65, 0x72, 0x6f, 0x6c,
            0x65, 0x5f, 0x6e, 0x61, 0x6d, 0x65, 0x72, 0x6f, 0x6c, 0x65, 0x5f, 0x6e, 0x61, 0x6d,
            0x65, 0x72, 0x6f, 0x6c, 0x65, 0x5f, 0x6e, 0x61, 0x6d, 0x65, 0x72, 0x6f, 0x6c, 0x65,
            0x5f, 0x6e, 0x61, 0x6d, 0x65, 0x72, 0x6f, 0x6c, 0x65, 0x5f, 0x6e, 0x61, 0x6d, 0x65,
            0x72, 0x6f, 0x6c, 0x65, 0x5f, 0x6e, 0x61, 0x6d, 0x65, 0x72, 0x6f, 0x6c, 0x65, 0x5f,
            0x6e, 0x61, 0x6d, 0x65, 0x72, 0x6f, 0x6c, 0x65, 0x5f, 0x6e, 0x61, 0x6d, 0x65, 0x72,
            0x6f, 0x6c, 0x65, 0x5f, 0x6e, 0x61, 0x6d, 0x65, 0x72, 0x6f, 0x6c, 0x65, 0x5f, 0x6e,
            0x61, 0x6d, 0x65, 0x72, 0x6f, 0x6c, 0x65, 0x5f, 0x6e, 0x61, 0x6d, 0x65, 0x72, 0x6f,
            0x6c, 0x65, 0x5f, 0x6e, 0x61, 0x6d, 0x65, 0x72, 0x6f, 0x6c, 0x65, 0x5f, 0x6e, 0x61,
            0x6d, 0x65, 0x72, 0x6f, 0x6c, 0x65, 0x5f, 0x6e, 0x61, 0x6d, 0x65, 0x72, 0x6f, 0x6c,
            0x65, 0x5f, 0x6e, 0x61, 0x6d, 0x65, 0x72, 0x6f, 0x6c, 0x65, 0x5f, 0x6e, 0x61, 0x6d,
            0x65, 0x0c, 0x0b, 0x54, 0x68, 0x69, 0x72, 0x64, 0x20, 0x66, 0x69, 0x65, 0x6c, 0x64};
        auto swPeer = parsePeerRoles(ConstDataRange(reinterpret_cast<char*>(derData),
                                                    std::extent<decltype(derData)>::value));
        ASSERT_OK(swPeer.getStatus());

        auto item = *(swPeer.getValue().begin());
        ASSERT_EQ(item.getRole(),
                  "role_namerole_namerole_namerole_namerole_namerole_namerole_namerole_namerole_"
                  "namerole_namerole_namerole_namerole_namerole_namerole_namerole_namerole_"
                  "namerole_namerole_namerole_namerole_namerole_namerole_namerole_namerole_"
                  "namerole_namerole_namerole_namerole_namerole_namerole_namerole_namerole_name");
        ASSERT_EQ(item.getDB(), "Third field");
    }

    // Negative: Encode MAX_INT64 into a length
    {
        unsigned char derData[] = {0x31, 0x88, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                                   0xff, 0x3e, 0x18, 0x0c, 0x09, 0x72, 0x6f, 0x6c, 0x65, 0x5f,
                                   0x6e, 0x61, 0x6d, 0x65, 0x0c, 0x0b, 0x54, 0x68, 0x69, 0x72,
                                   0x64, 0x20, 0x66, 0x69, 0x65, 0x6c, 0x64};

        auto swPeer = parsePeerRoles(ConstDataRange(reinterpret_cast<char*>(derData),
                                                    std::extent<decltype(derData)>::value));
        ASSERT_NOT_OK(swPeer.getStatus());
    }

    // Negative: Runt, only a tag
    {
        unsigned char derData[] = {0x31};
        auto swPeer = parsePeerRoles(ConstDataRange(reinterpret_cast<char*>(derData),
                                                    std::extent<decltype(derData)>::value));
        ASSERT_NOT_OK(swPeer.getStatus());
    }

    // Negative: Runt, only a tag and short length
    {
        unsigned char derData[] = {0x31, 0x0b};
        auto swPeer = parsePeerRoles(ConstDataRange(reinterpret_cast<char*>(derData),
                                                    std::extent<decltype(derData)>::value));
        ASSERT_NOT_OK(swPeer.getStatus());
    }

    // Negative: Runt, only a tag and long length with wrong missing length
    {
        unsigned char derData[] = {
            0x31, 0x88, 0xff, 0xff,
        };
        auto swPeer = parsePeerRoles(ConstDataRange(reinterpret_cast<char*>(derData),
                                                    std::extent<decltype(derData)>::value));
        ASSERT_NOT_OK(swPeer.getStatus());
    }

    // Negative: Runt, only a tag and long length
    {
        unsigned char derData[] = {
            0x31, 0x82, 0xff, 0xff,
        };
        auto swPeer = parsePeerRoles(ConstDataRange(reinterpret_cast<char*>(derData),
                                                    std::extent<decltype(derData)>::value));
        ASSERT_NOT_OK(swPeer.getStatus());
    }

    // Negative: Single UTF8 String
    {
        unsigned char derData[] = {
            0x0c, 0x0b, 0x48, 0x65, 0x6c, 0x6c, 0x6f, 0x20, 0x57, 0x6f, 0x72, 0x6c, 0x64};
        auto swPeer = parsePeerRoles(ConstDataRange(reinterpret_cast<char*>(derData),
                                                    std::extent<decltype(derData)>::value));
        ASSERT_NOT_OK(swPeer.getStatus());
    }

    // Negative: Unknown type - IAString
    {
        unsigned char derData[] = {
            0x16, 0x0b, 0x48, 0x65, 0x6c, 0x6c, 0x6f, 0x20, 0x57, 0x6f, 0x72, 0x6c, 0x64};
        auto swPeer = parsePeerRoles(ConstDataRange(reinterpret_cast<char*>(derData),
                                                    std::extent<decltype(derData)>::value));
        ASSERT_NOT_OK(swPeer.getStatus());
    }

    // Positive: two roles
    {
        unsigned char derData[] = {0x31, 0x2b, 0x30, 0x0f, 0x0c, 0x06, 0x62, 0x61, 0x63,
                                   0x6b, 0x75, 0x70, 0x0c, 0x05, 0x61, 0x64, 0x6d, 0x69,
                                   0x6e, 0x30, 0x18, 0x0c, 0x0f, 0x72, 0x65, 0x61, 0x64,
                                   0x41, 0x6e, 0x79, 0x44, 0x61, 0x74, 0x61, 0x62, 0x61,
                                   0x73, 0x65, 0x0c, 0x05, 0x61, 0x64, 0x6d, 0x69, 0x6e};
        auto swPeer = parsePeerRoles(ConstDataRange(reinterpret_cast<char*>(derData),
                                                    std::extent<decltype(derData)>::value));
        ASSERT_OK(swPeer.getStatus());

        auto roles = getSortedRoles(swPeer.getValue());
        ASSERT_EQ(roles[0].getRole(), "backup");
        ASSERT_EQ(roles[0].getDB(), "admin");
        ASSERT_EQ(roles[1].getRole(), "readAnyDatabase");
        ASSERT_EQ(roles[1].getDB(), "admin");
    }
}

TEST(SSLManager, EscapeRFC2253) {
    ASSERT_EQ(escapeRfc2253("abc"), "abc");
    ASSERT_EQ(escapeRfc2253(" abc"), "\\ abc");
    ASSERT_EQ(escapeRfc2253("#abc"), "\\#abc");
    ASSERT_EQ(escapeRfc2253("a,c"), "a\\,c");
    ASSERT_EQ(escapeRfc2253("a+c"), "a\\+c");
    ASSERT_EQ(escapeRfc2253("a\"c"), "a\\\"c");
    ASSERT_EQ(escapeRfc2253("a\\c"), "a\\\\c");
    ASSERT_EQ(escapeRfc2253("a<c"), "a\\<c");
    ASSERT_EQ(escapeRfc2253("a>c"), "a\\>c");
    ASSERT_EQ(escapeRfc2253("a;c"), "a\\;c");
    ASSERT_EQ(escapeRfc2253("abc "), "abc\\ ");
}

#endif

//  // namespace
}  // namespace mongo
