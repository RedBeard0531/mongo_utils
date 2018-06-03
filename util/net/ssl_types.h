/*    Copyright 2016 MongoDB Inc.
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

#include <string>

#include "mongo/bson/util/builder.h"
#include "mongo/db/auth/role_name.h"
#include "mongo/stdx/unordered_set.h"
#include "mongo/transport/session.h"

namespace mongo {

constexpr StringData kOID_CommonName = "2.5.4.3"_sd;

/**
 * Represents a structed X509 certificate subject name.
 * For example: C=US,O=MongoDB,OU=KernelTeam,CN=server
 * would be held as a four element vector of Entries.
 * The first entry of which yould be broken down something like:
 * {{"2.5.4.6", 19, "US"}}.
 * Note that _entries is a vector of vectors to accomodate
 * multi-value RDNs.
 */
class SSLX509Name {
public:
    struct Entry {
        Entry(std::string oid, int type, std::string value)
            : oid(std::move(oid)), type(type), value(std::move(value)) {}
        std::string oid;  // e.g. "2.5.4.8" (ST)
        int type;         // e.g. 19 (PRINTABLESTRING)
        std::string value;
        auto equalityLens() const {
            return std::tie(oid, type, value);
        }
    };

    SSLX509Name() = default;
    explicit SSLX509Name(std::vector<std::vector<Entry>> entries) : _entries(std::move(entries)) {}

    /**
     * Retreive the first instance of the value for a given OID in this name.
     * Returns ErrorCodes::KeyNotFound if the OID does not exist.
     */
    StatusWith<std::string> getOID(StringData oid) const;

    bool empty() const {
        return std::all_of(
            _entries.cbegin(), _entries.cend(), [](const auto& e) { return e.empty(); });
    }

    friend StringBuilder& operator<<(StringBuilder&, const SSLX509Name&);
    std::string toString() const;

    friend bool operator==(const SSLX509Name& lhs, const SSLX509Name& rhs) {
        return lhs._entries == rhs._entries;
    }
    friend bool operator!=(const SSLX509Name& lhs, const SSLX509Name& rhs) {
        return !(lhs._entries == rhs._entries);
    }

private:
    friend struct SSLConfiguration;
    std::vector<std::vector<Entry>> _entries;
};

std::ostream& operator<<(std::ostream&, const SSLX509Name&);
inline bool operator==(const SSLX509Name::Entry& lhs, const SSLX509Name::Entry& rhs) {
    return lhs.equalityLens() == rhs.equalityLens();
}
inline bool operator<(const SSLX509Name::Entry& lhs, const SSLX509Name::Entry& rhs) {
    return lhs.equalityLens() < rhs.equalityLens();
}

/**
 * Contains information extracted from the peer certificate which is consumed by subsystems
 * outside of the networking stack.
 */
struct SSLPeerInfo {
    SSLPeerInfo(SSLX509Name subjectName, stdx::unordered_set<RoleName> roles)
        : subjectName(std::move(subjectName)), roles(std::move(roles)) {}
    SSLPeerInfo() = default;

    SSLX509Name subjectName;
    stdx::unordered_set<RoleName> roles;

    static SSLPeerInfo& forSession(const transport::SessionHandle& session);
};

}  // namespace mongo
