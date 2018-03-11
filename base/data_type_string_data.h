/*    Copyright 2015 MongoDB Inc.
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

#include <cstring>
#include <stddef.h>

#include "mongo/base/data_type.h"
#include "mongo/base/string_data.h"

#ifndef MONGO_BASE_DATA_TYPE_H_INCLUDE_HANDSHAKE_
#error "do not include directly. Use mongo/base/data_type.h"
#endif  // MONGO_BASE_DATA_TYPE_H_INCLUDE_HANDSHAKE_

// Provides a DataType::Handler specialization for StringData.

namespace mongo {

template <>
struct DataType::Handler<StringData> {
    // Consumes all available data, producing
    // a `StringData(ptr,length)`.
    static Status load(StringData* sdata,
                       const char* ptr,
                       size_t length,
                       size_t* advanced,
                       std::ptrdiff_t debug_offset);

    // Copies `sdata` fully into the [ptr,ptr+length) range.
    // Does nothing and returns an Overflow status if
    // `sdata` doesn't fit.
    static Status store(const StringData& sdata,
                        char* ptr,
                        size_t length,
                        size_t* advanced,
                        std::ptrdiff_t debug_offset);

    static StringData defaultConstruct() {
        return StringData();
    }
};

}  // namespace mongo
