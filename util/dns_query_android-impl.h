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

#ifndef MONGO_UTIL_DNS_QUERY_PLATFORM_INCLUDE_WHITELIST
#error Do not include the DNS Query platform implementation headers.  Please use "mongo/util/dns_query.h" instead.
#endif

#include <stdexcept>
#include <string>
#include <vector>

#include "mongo/util/assert_util.h"

namespace mongo {
namespace dns {
namespace {

enum class DNSQueryClass { kInternet };

enum class DNSQueryType { kSRV, kTXT, kAddress };

[[noreturn]] void throwNotSupported() {
    uasserted(ErrorCodes::InternalErrorNotSupported, "srv_nsearch not supported on android");
}

class ResourceRecord {
public:
    explicit ResourceRecord() = default;

    std::vector<std::string> txtEntry() const {
        throwNotSupported();
    }

    std::string addressEntry() const {
        throwNotSupported();
    }

    SRVHostEntry srvHostEntry() const {
        throwNotSupported();
    }
};

using DNSResponse = std::vector<ResourceRecord>;

class DNSQueryState {
public:
    DNSResponse lookup(const std::string&, const DNSQueryClass, const DNSQueryType) {
        throwNotSupported();
    }

    DNSQueryState() {
        throwNotSupported();
    }
};
}  // namespace
}  // namespace dns
}  // namespace mongo
