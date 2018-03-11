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
#include "mongo/util/dns_query.h"

#include "mongo/unittest/unittest.h"

using namespace std::literals::string_literals;

namespace {
std::string getFirstARecord(const std::string& service) {
    auto res = mongo::dns::lookupARecords(service);
    if (res.empty())
        return "";
    return res.front();
}

TEST(MongoDnsQuery, basic) {
    // We only require 50% of the records to pass, because it is possible that some large scale
    // outages could cause some of these records to fail.
    const double kPassingPercentage = 0.50;
    std::size_t resolution_count = 0;
    const struct {
        std::string dns;
        std::string ip;
    } tests[] =
        // The reason for a vast number of tests over basic DNS query calls is to provide a
        // redundancy in testing.  We'd like to make sure that this test always passes.  Lazy
        // maintanance will cause some references to be commented out.  Our belief is that all 13
        // root servers and both of Google's public servers will all be unresolvable (when
        // connections are available) only when a major problem occurs.  This test only fails if
        // more than half of the resolved names fail.
        {
            // These can be kept up to date by checking the root-servers.org listings.  Note that
            // root name servers are located in the `root-servers.net.` domain, NOT in the
            // `root-servers.org.` domain.  The `.org` domain is for webpages with statistics on
            // these servers.  The `.net` domain is the domain with the canonical addresses for
            // these servers.
            {"a.root-servers.net.", "198.41.0.4"},
            {"b.root-servers.net.", "199.9.14.201"},
            {"c.root-servers.net.", "192.33.4.12"},
            {"d.root-servers.net.", "199.7.91.13"},
            {"e.root-servers.net.", "192.203.230.10"},
            {"f.root-servers.net.", "192.5.5.241"},
            {"g.root-servers.net.", "192.112.36.4"},
            {"h.root-servers.net.", "198.97.190.53"},
            {"i.root-servers.net.", "192.36.148.17"},
            {"j.root-servers.net.", "192.58.128.30"},
            {"k.root-servers.net.", "193.0.14.129"},
            {"l.root-servers.net.", "199.7.83.42"},
            {"m.root-servers.net.", "202.12.27.33"},

            // These can be kept up to date by checking with Google's public dns service.
            {"google-public-dns-a.google.com.", "8.8.8.8"},
            {"google-public-dns-b.google.com.", "8.8.4.4"},
        };
    for (const auto& test : tests) {
        try {
            const auto witness = getFirstARecord(test.dns);
            std::cout << "Resolved " << test.dns << " to: " << witness << std::endl;

            const bool resolution = (witness == test.ip);
            if (!resolution)
                std::cerr << "Warning: Did not correctly resolve " << test.dns << std::endl;
            resolution_count += resolution;
        }
        // Failure to resolve is okay, but not great -- print a warning
        catch (const mongo::DBException& ex) {
            std::cerr << "Warning: Did not resolve " << test.dns << " at all: " << ex.what()
                      << std::endl;
        }
    }

    // As long as enough tests pass, we're okay -- this means that a single DNS name server drift
    // won't cause a BF -- when enough fail, then we can rebuild the list in one pass.
    const std::size_t kPassingRate = sizeof(tests) / sizeof(tests[0]) * kPassingPercentage;
    ASSERT_GTE(resolution_count, kPassingRate);
}

TEST(MongoDnsQuery, srvRecords) {
    const auto kMongodbSRVPrefix = "_mongodb._tcp."s;
    const struct {
        std::string query;
        std::vector<mongo::dns::SRVHostEntry> result;
    } tests[] = {
        {"test1.test.build.10gen.cc.",
         {
             {"localhost.test.build.10gen.cc.", 27017}, {"localhost.test.build.10gen.cc.", 27018},
         }},
        {"test2.test.build.10gen.cc.",
         {
             {"localhost.test.build.10gen.cc.", 27018}, {"localhost.test.build.10gen.cc.", 27019},
         }},
        {"test3.test.build.10gen.cc.",
         {
             {"localhost.test.build.10gen.cc.", 27017},
         }},

        // Test case 4 does not exist in the expected DNS records.
        {"test4.test.build.10gen.cc.", {}},

        {"test5.test.build.10gen.cc.",
         {
             {"localhost.test.build.10gen.cc.", 27017},
         }},
        {"test6.test.build.10gen.cc.",
         {
             {"localhost.test.build.10gen.cc.", 27017},
         }},
    };
    for (const auto& test : tests) {
        const auto& expected = test.result;
        if (expected.empty()) {
            ASSERT_THROWS_CODE(mongo::dns::lookupSRVRecords(kMongodbSRVPrefix + test.query),
                               mongo::DBException,
                               mongo::ErrorCodes::DNSHostNotFound);
            continue;
        }

        auto witness = mongo::dns::lookupSRVRecords(kMongodbSRVPrefix + test.query);
        std::sort(begin(witness), end(witness));

        for (const auto& entry : witness) {
            std::cout << "Entry: " << entry << std::endl;
        }

        for (std::size_t i = 0; i < witness.size() && i < expected.size(); ++i) {
            std::cout << "Expected: " << expected.at(i) << std::endl;
            std::cout << "Witness:  " << witness.at(i) << std::endl;
            ASSERT_EQ(witness.at(i), expected.at(i));
        }

        ASSERT_TRUE(std::equal(begin(witness), end(witness), begin(expected), end(expected)));
        ASSERT_TRUE(witness.size() == expected.size());
    }
}

TEST(MongoDnsQuery, txtRecords) {
    const struct {
        std::string query;
        std::vector<std::string> result;
    } tests[] = {
        // Test case 4 does not exist in the expected DNS records.
        {"test4.test.build.10gen.cc.", {}},

        {"test5.test.build.10gen.cc",
         {
             "replicaSet=repl0&authSource=thisDB",
         }},
        {"test6.test.build.10gen.cc",
         {
             "authSource=otherDB", "replicaSet=repl0",
         }},
    };

    for (const auto& test : tests) {
        try {
            auto witness = mongo::dns::getTXTRecords(test.query);
            std::sort(begin(witness), end(witness));

            const auto& expected = test.result;

            ASSERT_TRUE(std::equal(begin(witness), end(witness), begin(expected), end(expected)));
            ASSERT_EQ(witness.size(), expected.size());
        } catch (const mongo::DBException& ex) {
            if (ex.code() != mongo::ErrorCodes::DNSHostNotFound)
                throw;
            ASSERT_TRUE(test.result.empty());
        }
    }
}
}  // namespace
