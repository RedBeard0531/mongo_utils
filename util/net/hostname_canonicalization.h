/**
 * Copyright (C) 2015 MongoDB Inc.
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

namespace mongo {

/**
 * DNS canonicalization converts a hostname into another potentially more globally useful hostname.
 *
 * This involves issuing some combination of IP and name lookups.
 * This enum controls which canonicalization process a client will perform on a
 * hostname it is canonicalizing.
 */
enum class HostnameCanonicalizationMode {
    kNone,              // Perform no canonicalization at all
    kForward,           // Perform a DNS lookup on the hostname, follow CNAMEs to find the A record
    kForwardAndReverse  // Forward resolve to get an IP, then perform reverse lookup on it
};

/**
 *  Returns zero or more fully qualified hostnames associated with the provided hostname.
 *
 *  May return an empty vector if no FQDNs can be determined, or if the underlying
 *  implementation returns an error. The returned information is advisory only.
 */
std::vector<std::string> getHostFQDNs(std::string hostName, HostnameCanonicalizationMode mode);

}  // namespace mongo
