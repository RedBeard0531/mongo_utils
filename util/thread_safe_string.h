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

#include <cstring>
#include <iosfwd>
#include <string>

#include "mongo/base/disallow_copying.h"
#include "mongo/base/string_data.h"

namespace mongo {

/**
 * this is a thread safe string
 * you will never get a bad pointer, though data may be mungedd
 */
class ThreadSafeString {
    MONGO_DISALLOW_COPYING(ThreadSafeString);

public:
    ThreadSafeString(size_t size = 256) : _size(size), _buf(new char[size]) {
        memset(_buf, '\0', _size);
    }

    ~ThreadSafeString() {
        delete[] _buf;
    }

    std::string toString() const {
        return _buf;
    }

    ThreadSafeString& operator=(StringData str) {
        size_t s = str.size();
        if (s >= _size - 2)
            s = _size - 2;
        strncpy(_buf, str.rawData(), s);
        _buf[s] = '\0';
        return *this;
    }

    bool empty() const {
        return _buf[0] == '\0';
    }

private:
    const size_t _size;
    char* const _buf;
};

std::ostream& operator<<(std::ostream& s, const ThreadSafeString& o);

}  // namespace mongo
