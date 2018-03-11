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

#include "mongo/base/data_type.h"

#include "mongo/util/mongoutils/str.h"

namespace mongo {

namespace {

Status makeStoreStatus(const StringData& sdata, size_t length, std::ptrdiff_t debug_offset) {
    str::stream ss;
    ss << "buffer size too small to write StringData(" << sdata.size() << ") bytes into buffer["
       << length << "] at offset: " << debug_offset;
    return Status(ErrorCodes::Overflow, ss);
}
}  // namespace

Status DataType::Handler<StringData>::load(StringData* sdata,
                                           const char* ptr,
                                           size_t length,
                                           size_t* advanced,
                                           std::ptrdiff_t debug_offset) {
    if (sdata) {
        *sdata = StringData(ptr, length);
    }

    if (advanced) {
        *advanced = length;
    }

    return Status::OK();
}

Status DataType::Handler<StringData>::store(const StringData& sdata,
                                            char* ptr,
                                            size_t length,
                                            size_t* advanced,
                                            std::ptrdiff_t debug_offset) {
    if (sdata.size() > length) {
        return makeStoreStatus(sdata, length, debug_offset);
    }

    if (ptr) {
        std::memcpy(ptr, sdata.rawData(), sdata.size());
    }

    if (advanced) {
        *advanced = sdata.size();
    }

    return Status::OK();
}

}  // namespace mongo
