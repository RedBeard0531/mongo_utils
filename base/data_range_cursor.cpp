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

#include "mongo/base/data_range_cursor.h"

#include "mongo/util/mongoutils/str.h"

namespace mongo {

Status ConstDataRangeCursor::makeAdvanceStatus(size_t advance) const {
    mongoutils::str::stream ss;
    ss << "Invalid advance (" << advance << ") past end of buffer[" << length()
       << "] at offset: " << _debug_offset;

    return Status(ErrorCodes::Overflow, ss);
}

Status DataRangeCursor::makeAdvanceStatus(size_t advance) const {
    mongoutils::str::stream ss;
    ss << "Invalid advance (" << advance << ") past end of buffer[" << length()
       << "] at offset: " << _debug_offset;

    return Status(ErrorCodes::Overflow, ss);
}

}  // namespace mongo
