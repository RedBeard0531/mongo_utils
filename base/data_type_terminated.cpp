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

#include "mongo/base/data_type_terminated.h"

#include "mongo/util/mongoutils/str.h"
#include "mongo/util/stringutils.h"

namespace mongo {

Status TerminatedHelper::makeLoadNoTerminalStatus(char c,
                                                  size_t length,
                                                  std::ptrdiff_t debug_offset) {
    str::stream ss;
    ss << "couldn't locate terminal char (" << escape(StringData(&c, 1)) << ") in buffer[" << length
       << "] at offset: " << debug_offset;
    return Status(ErrorCodes::Overflow, ss);
}

Status TerminatedHelper::makeLoadShortReadStatus(char c,
                                                 size_t read,
                                                 size_t length,
                                                 std::ptrdiff_t debug_offset) {
    str::stream ss;
    ss << "only read (" << read << ") bytes. (" << length << ") bytes to terminal char ("
       << escape(StringData(&c, 1)) << ") at offset: " << debug_offset;

    return Status(ErrorCodes::Overflow, ss);
}

Status TerminatedHelper::makeStoreStatus(char c, size_t length, std::ptrdiff_t debug_offset) {
    str::stream ss;
    ss << "couldn't write terminal char (" << escape(StringData(&c, 1)) << ") in buffer[" << length
       << "] at offset: " << debug_offset;
    return Status(ErrorCodes::Overflow, ss);
}

}  // namespace mongo
