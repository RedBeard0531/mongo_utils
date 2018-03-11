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

#include "mongo/base/status_with.h"
#include "mongo/base/string_data.h"
#include "mongo/bson/bsonelement.h"
#include "mongo/bson/bsonmisc.h"

#include <stdexcept>
#include <string>

#ifndef _WIN32
#include <sys/socket.h>
#endif

namespace mongo {

/**
 * CIDR (Classless Inter-Domain Routing)
 */
class CIDR {
public:
    explicit CIDR(StringData);

    /**
     * If the given BSONElement represents a valid CIDR range,
     * constructs and returns the CIDR.
     * Otherwise returns an error.
     */
    static StatusWith<CIDR> parse(BSONElement from) noexcept;

    /**
     * If the given string represents a valid CIDR range,
     * constructs and returns the CIDR.
     * Otherwise returns an error.
     */
    static StatusWith<CIDR> parse(StringData from) noexcept;

    /**
     * Returns true if the provided address range is contained
     * entirely within this one, false otherwise.
     */
    bool contains(const CIDR& cidr) const {
        if ((_family != cidr._family) || (_len > cidr._len)) {
            return false;
        }

        auto bytes = _len / 8;
        auto const range = _ip.begin();
        auto const ip = cidr._ip.begin();
        if (!std::equal(range, range + bytes, ip, ip + bytes)) {
            return false;
        }

        if ((_len % 8) == 0) {
            return true;
        }

        auto mask = (0xFF << (8 - (_len % 8))) & 0xFF;
        return (_ip[bytes] & mask) == (cidr._ip[bytes] & mask);
    }

    friend bool operator==(const CIDR& lhs, const CIDR& rhs);
    friend bool operator!=(const CIDR& lhs, const CIDR& rhs) {
        return !(lhs == rhs);
    }
    friend std::ostream& operator<<(std::ostream& s, const CIDR& rhs);
    friend StringBuilder& operator<<(StringBuilder& s, const CIDR& rhs);

    /**
     * Return a string representation of this CIDR (i.e. "169.254.0.0/16")
     */
    std::string toString() const {
        StringBuilder s;
        s << *this;
        return s.str();
    }

private:
#ifdef _WIN32
    using sa_family_t = int;
#endif

    auto equalityLens() const {
        return std::tie(_ip, _family, _len);
    }

    std::array<std::uint8_t, 16> _ip;
    sa_family_t _family;
    std::uint8_t _len;
};

inline bool operator==(const CIDR& lhs, const CIDR& rhs) {
    return lhs.equalityLens() == rhs.equalityLens();
}

std::ostream& operator<<(std::ostream& s, const CIDR& cidr);
StringBuilder& operator<<(StringBuilder& s, const CIDR& cidr);

/**
 * Supports use of CIDR with the BSON macro:
 *     BSON("cidr" << cidr) -> { cidr: "..." }
 */
template <>
BSONObjBuilder& BSONObjBuilderValueStream::operator<<<CIDR>(CIDR value);

}  // namespace mongo
