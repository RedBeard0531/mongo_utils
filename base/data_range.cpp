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

#include "mongo/base/data_range.h"

#include "mongo/util/mongoutils/str.h"

namespace mongo {

Status ConstDataRange::makeOffsetStatus(size_t offset) const {
    str::stream ss;
    ss << "Invalid offset(" << offset << ") past end of buffer[" << length()
       << "] at offset: " << _debug_offset;

    return Status(ErrorCodes::Overflow, ss);
}

Status DataRangeTypeHelper::makeStoreStatus(size_t t_length,
                                            size_t length,
                                            std::ptrdiff_t debug_offset) {
    str::stream ss;
    ss << "buffer size too small to write (" << t_length << ") bytes into buffer[" << length
       << "] at offset: " << debug_offset;
    return Status(ErrorCodes::Overflow, ss);
}

}  // namespace mongo
