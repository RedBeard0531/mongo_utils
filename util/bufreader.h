// @file bufreader.h parse a memory region into usable pieces

/**
*    Copyright (C) 2009 10gen Inc.
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

#include <utility>

#include "mongo/base/data_range.h"
#include "mongo/base/data_range_cursor.h"
#include "mongo/base/data_type_terminated.h"
#include "mongo/base/disallow_copying.h"
#include "mongo/bson/util/builder.h"
#include "mongo/platform/strnlen.h"
#include "mongo/util/assert_util.h"

namespace mongo {

/** helper to read and parse a block of memory
    methods throw the eof exception if the operation would pass the end of the
    buffer with which we are working.
*/
class BufReader {
    MONGO_DISALLOW_COPYING(BufReader);

public:
    BufReader(const void* p, unsigned len)
        : _start(reinterpret_cast<const char*>(p)), _pos(_start), _end(_start + len) {}

    bool atEof() const {
        return _pos == _end;
    }

    /** read in the object specified, and advance buffer pointer */
    template <typename T>
    void read(T& t) {
        ConstDataRangeCursor cdrc(_pos, _end);
        uassertStatusOK(cdrc.readAndAdvance(&t));
        _pos = cdrc.data();
    }

    /** read in and return an object of the specified type, and advance buffer pointer */
    template <typename T>
    T read() {
        T out{};
        read(out);
        return out;
    }

    /** read in the object specified, but do not advance buffer pointer */
    template <typename T>
    void peek(T& t) const {
        uassertStatusOK(ConstDataRange(_pos, _end).read(&t));
    }

    /** read in and return an object of the specified type, but do not advance buffer pointer */
    template <typename T>
    T peek() const {
        T out{};
        peek(out);
        return out;
    }

    /** return current offset into buffer */
    unsigned offset() const {
        return _pos - _start;
    }

    /** return remaining bytes */
    unsigned remaining() const {
        return _end - _pos;
    }

    /** back up by nbytes */
    void rewind(unsigned nbytes) {
        _pos = _pos - nbytes;
        invariant(_pos >= _start);
    }

    /** return current position pointer, and advance by len */
    const void* skip(unsigned len) {
        ConstDataRangeCursor cdrc(_pos, _end);
        uassertStatusOK(cdrc.advance(len));
        return std::exchange(_pos, cdrc.data());
    }

    /// reads a NUL terminated string
    StringData readCStr() {
        auto range = read<Terminated<'\0', ConstDataRange>>().value;

        return StringData(range.data(), range.length());
    }

    void readStr(std::string& s) {
        s = readCStr().toString();
    }

    const void* pos() {
        return _pos;
    }
    const void* start() {
        return _start;
    }

private:
    const char* _start;
    const char* _pos;
    const char* _end;
};
}
