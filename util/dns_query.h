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

#pragma once

#include <array>
#include <cstdint>
#include <exception>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include <boost/noncopyable.hpp>

#include "mongo/util/assert_util.h"

namespace mongo {
namespace dns {
/**
 * An `SRVHostEntry` object represents the information received from a DNS lookup of an SRV record.
 * NOTE: An SRV DNS record has several fields, such as priority and weight.  This structure lacks
 * those fields at this time.  They should be safe to add in the future.  The code using this
 * structure does not need access to those fields at this time.
 */
struct SRVHostEntry {
    std::string host;
    std::uint16_t port;

    SRVHostEntry(std::string initialHost, const std::uint16_t initialPort)
        : host(std::move(initialHost)), port(initialPort) {}

    inline auto makeRelopsLens() const {
        return std::tie(host, port);
    }

    inline friend std::ostream& operator<<(std::ostream& os, const SRVHostEntry& entry) {
        return os << entry.host << ':' << entry.port;
    }

    inline friend bool operator==(const SRVHostEntry& lhs, const SRVHostEntry& rhs) {
        return lhs.makeRelopsLens() == rhs.makeRelopsLens();
    }

    inline friend bool operator!=(const SRVHostEntry& lhs, const SRVHostEntry& rhs) {
        return !(lhs == rhs);
    }

    inline friend bool operator<(const SRVHostEntry& lhs, const SRVHostEntry& rhs) {
        return lhs.makeRelopsLens() < rhs.makeRelopsLens();
    }
};

/**
 * Returns a vector containing SRVHost entries for the specified `service`.
 * THROWS: `DBException` with `ErrorCodes::DNSHostNotFound` as the status value if the entry is not
 * found and `ErrorCodes::DNSProtocolError` as the status value if the DNS lookup fails, for any
 * other reason
 */
std::vector<SRVHostEntry> lookupSRVRecords(const std::string& service);

/**
 * Returns a group of strings containing text from DNS TXT entries for a specified service.
 * THROWS: `DBException` with `ErrorCodes::DNSHostNotFound` as the status value if the entry is not
 * found and `ErrorCodes::DNSProtocolError` as the status value if the DNS lookup fails, for any
 * other reason
 */
std::vector<std::string> lookupTXTRecords(const std::string& service);

/**
 * Returns a group of strings containing text from DNS TXT entries for a specified service.
 * If the lookup fails because the record doesn't exist, an empty vector is returned.
 * THROWS: `DBException` with `ErrorCodes::DNSProtocolError` as th status value if the DNS lookup
 * fails for any other reason.
 */
std::vector<std::string> getTXTRecords(const std::string& service);

/**
 * Returns a group of strings containing Address entries for a specified service.
 * THROWS: `DBException` with `ErrorCodes::DNSHostNotFound` as the status value if the entry is not
 * found and `ErrorCodes::DNSProtocolError` as the status value if the DNS lookup fails, for any
 * other reason
 * NOTE: This function mostly exists to provide an easy test of the OS DNS APIs in our test driver.
 */
std::vector<std::string> lookupARecords(const std::string& service);
}  // namespace dns
}  // namespace mongo
