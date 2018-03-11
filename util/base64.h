// util/base64.h

/*    Copyright 2009 10gen Inc.
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

#include <sstream>
#include <string>

#include "mongo/base/string_data.h"

namespace mongo {
namespace base64 {

void encode(std::stringstream& ss, const char* data, int size);
std::string encode(const char* data, int size);
std::string encode(const std::string& s);

void decode(std::stringstream& ss, const std::string& s);
std::string decode(const std::string& s);

bool validate(StringData);

/**
 * Calculate how large a given input would expand to.
 * Effectively: ceil(inLen * 4 / 3)
 */
constexpr size_t encodedLength(size_t inLen) {
    return static_cast<size_t>((inLen + 2.5) / 3) * 4;
}

}  // namespace base64
}  // namespace mongo
