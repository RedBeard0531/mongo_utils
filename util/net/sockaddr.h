/**
 *    Copyright (C) 2016 MongoDB Inc.
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
#include <vector>

#ifndef _WIN32

#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>

#ifdef __OpenBSD__
#include <sys/uio.h>
#endif  // __OpenBSD__

#endif  // not _WIN32

#include "mongo/base/string_data.h"

namespace mongo {

#if defined(_WIN32)

typedef short sa_family_t;
typedef int socklen_t;

// This won't actually be used on windows
struct sockaddr_un {
    short sun_family;
    char sun_path[108];  // length from unix header
};

#endif  // _WIN32

// Generate a string representation for getaddrinfo return codes
std::string getAddrInfoStrError(int code);

/**
 * Wrapper around os representation of network address.
 */
struct SockAddr {
    SockAddr();

    explicit SockAddr(int sourcePort); /* listener side */

    /**
     * Initialize a SockAddr for a given IP or Hostname.
     *
     * If target fails to resolve/parse, SockAddr.isValid() may return false,
     * or the resulting SockAddr may be equivalent to SockAddr(port).
     *
     * If target is a unix domain socket, a uassert() exception will be thrown
     * on windows or if addr exceeds maximum path length.
     *
     * If target resolves to more than one address, only the first address
     * will be used. Others will be discarded.
     * SockAddr::createAll() is recommended for capturing all addresses.
     */
    explicit SockAddr(StringData target, int port, sa_family_t familyHint);

    explicit SockAddr(struct sockaddr_storage& other, socklen_t size);

    /**
     * Resolve an ip or hostname to a vector of SockAddr objects.
     *
     * Works similar to SockAddr(StringData, int, sa_family_t) above,
     * however all addresses returned from ::getaddrinfo() are used,
     * it never falls-open to SockAddr(port),
     * and isInvalid() SockAddrs are excluded.
     *
     * May return an empty vector.
     */
    static std::vector<SockAddr> createAll(StringData target, int port, sa_family_t familyHint);

    template <typename T>
    T& as() {
        return *(T*)(&sa);
    }
    template <typename T>
    const T& as() const {
        return *(const T*)(&sa);
    }

    std::string hostOrIp() const {
        return _hostOrIp;
    }

    std::string toString(bool includePort = true) const;

    bool isValid() const {
        return _isValid;
    }

    bool isIP() const;

    /**
     * @return one of AF_INET, AF_INET6, or AF_UNIX
     */
    sa_family_t getType() const;

    unsigned getPort() const;

    std::string getAddr() const;

    bool isLocalHost() const;
    bool isDefaultRoute() const;
    bool isAnonymousUNIXSocket() const;

    bool operator==(const SockAddr& r) const;
    bool operator!=(const SockAddr& r) const;
    bool operator<(const SockAddr& r) const;

    const sockaddr* raw() const {
        return (sockaddr*)&sa;
    }
    sockaddr* raw() {
        return (sockaddr*)&sa;
    }

    socklen_t addressSize;

private:
    void initUnixDomainSocket(const std::string& path, int port);

    std::string _hostOrIp;
    struct sockaddr_storage sa;
    bool _isValid;
};

}  // namespace mongo
