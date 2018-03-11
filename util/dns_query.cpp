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

#include "mongo/util/dns_query.h"

#include <array>
#include <cassert>
#include <cstdint>
#include <exception>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <boost/noncopyable.hpp>

#include "mongo/bson/util/builder.h"

// It is safe to include the implementation "headers" in an anonymous namespace, as the code is
// meant to live in a single TU -- this one.  Include one of these headers last.
#define MONGO_UTIL_DNS_QUERY_PLATFORM_INCLUDE_WHITELIST
#ifdef WIN32
#include "mongo/util/dns_query_windows-impl.h"
#elif __ANDROID__
#include "mongo/util/dns_query_android-impl.h"
#else
#include "mongo/util/dns_query_posix-impl.h"
#endif
#undef MONGO_UTIL_DNS_QUERY_PLATFORM_INCLUDE_WHITELIST

using std::begin;
using std::end;
using namespace std::literals::string_literals;

namespace mongo {

/**
 * Returns a string with the IP address or domain name listed...
 */
std::vector<std::string> dns::lookupARecords(const std::string& service) {
    DNSQueryState dnsQuery;
    auto response = dnsQuery.lookup(service, DNSQueryClass::kInternet, DNSQueryType::kAddress);

    std::vector<std::string> rv;

    for (const auto& entry : response) {
        try {
            rv.push_back(entry.addressEntry());
        } catch (const ExceptionFor<ErrorCodes::DNSRecordTypeMismatch>&) {
        }
    }

    if (rv.empty()) {
        StringBuilder oss;
        oss << "Looking up " << service << " A record yielded ";
        if (response.size() == 0) {
            oss << "no results.";
        } else {
            oss << "no A records but " << response.size() << " other records";
        }
        uasserted(ErrorCodes::DNSProtocolError, oss.str());
    }

    return rv;
}

std::vector<dns::SRVHostEntry> dns::lookupSRVRecords(const std::string& service) {
    DNSQueryState dnsQuery;

    auto response = dnsQuery.lookup(service, DNSQueryClass::kInternet, DNSQueryType::kSRV);

    std::vector<SRVHostEntry> rv;

    for (const auto& entry : response) {
        try {
            rv.push_back(entry.srvHostEntry());
        } catch (const ExceptionFor<ErrorCodes::DNSRecordTypeMismatch>&) {
        }
    }

    if (rv.empty()) {
        StringBuilder oss;
        oss << "Looking up " << service << " SRV record yielded ";
        if (response.size() == 0) {
            oss << "no results.";
        } else {
            oss << "no SRV records but " << response.size() << " other records";
        }
        uasserted(ErrorCodes::DNSProtocolError, oss.str());
    }

    return rv;
}

std::vector<std::string> dns::lookupTXTRecords(const std::string& service) {
    DNSQueryState dnsQuery;

    auto response = dnsQuery.lookup(service, DNSQueryClass::kInternet, DNSQueryType::kTXT);

    std::vector<std::string> rv;

    for (auto& entry : response) {
        try {
            auto txtEntry = entry.txtEntry();
            rv.insert(end(rv),
                      std::make_move_iterator(begin(txtEntry)),
                      std::make_move_iterator(end(txtEntry)));
        } catch (const ExceptionFor<ErrorCodes::DNSRecordTypeMismatch>&) {
        }
    }

    return rv;
}

std::vector<std::string> dns::getTXTRecords(const std::string& service) try {
    return lookupTXTRecords(service);
} catch (const ExceptionFor<ErrorCodes::DNSHostNotFound>&) {
    return {};
}
}  // namespace mongo
