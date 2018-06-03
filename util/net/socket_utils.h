/*    Copyright 2018 MongoDB Inc.
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


namespace mongo {

void setSocketKeepAliveParams(int sock,
                              unsigned int maxKeepIdleSecs = 300,
                              unsigned int maxKeepIntvlSecs = 300);

std::string makeUnixSockPath(int port);

// If an ip address is passed in, just return that.  If a hostname is passed
// in, look up its ip and return that.  Returns "" on failure.
std::string hostbyname(const char* hostname);

void enableIPv6(bool state = true);
bool IPv6Enabled();

/** this is not cache and does a syscall */
std::string getHostName();

/** this is cached, so if changes during the process lifetime
 * will be stale */
std::string getHostNameCached();

/** Returns getHostNameCached():<port>. */
std::string getHostNameCachedAndPort();

/** Returns getHostNameCached(), or getHostNameCached():<port> if running on a non-default port. */
std::string prettyHostName();

}  // namespace mongo
