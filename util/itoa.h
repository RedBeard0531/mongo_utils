/**
 *    Copyright (C) 2015 MongoDB Inc.
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

#include <cstdint>
#include <limits>

#include "mongo/base/disallow_copying.h"
#include "mongo/base/string_data.h"

namespace mongo {

/**
 * A utility class for performing itoa style integer formatting. This class is highly optimized
 * and only really should be used in hot code paths.
 */
class ItoA {
    MONGO_DISALLOW_COPYING(ItoA);

public:
    static constexpr size_t kBufSize = std::numeric_limits<uint64_t>::digits10  //
        + 1   // digits10 is 1 less than the maximum number of digits.
        + 1;  // NUL byte.

    explicit ItoA(std::uint64_t i);

    operator StringData() {
        return {_str, _len};
    }

private:
    const char* _str{nullptr};
    std::size_t _len{0};
    char _buf[kBufSize];
};

}  // namespace mongo
