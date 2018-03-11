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

#include <cstddef>
#include <cstring>
#include <limits>

#include "mongo/base/data_range.h"
#include "mongo/base/data_type.h"
#include "mongo/platform/endian.h"
#include "mongo/util/mongoutils/str.h"

namespace mongo {

class ConstDataRangeCursor : public ConstDataRange {
public:
    ConstDataRangeCursor(const char* begin, const char* end, std::ptrdiff_t debug_offset = 0)
        : ConstDataRange(begin, end, debug_offset) {}

    ConstDataRangeCursor(ConstDataRange cdr) : ConstDataRange(cdr) {}

    Status advance(size_t advance) {
        if (advance > length()) {
            return makeAdvanceStatus(advance);
        }

        _begin += advance;
        _debug_offset += advance;

        return Status::OK();
    }

    template <typename T>
    Status skip() {
        size_t advanced = 0;

        Status x = DataType::load<T>(nullptr, _begin, _end - _begin, &advanced, _debug_offset);

        if (x.isOK()) {
            _begin += advanced;
            _debug_offset += advanced;
        }

        return x;
    }

    template <typename T>
    Status readAndAdvance(T* t) {
        size_t advanced = 0;

        Status x = DataType::load(t, _begin, _end - _begin, &advanced, _debug_offset);

        if (x.isOK()) {
            _begin += advanced;
            _debug_offset += advanced;
        }

        return x;
    }

    template <typename T>
    StatusWith<T> readAndAdvance() {
        T out(DataType::defaultConstruct<T>());
        Status x = readAndAdvance(&out);

        if (x.isOK()) {
            return StatusWith<T>(std::move(out));
        } else {
            return StatusWith<T>(std::move(x));
        }
    }

private:
    Status makeAdvanceStatus(size_t advance) const;
};

class DataRangeCursor : public DataRange {
public:
    DataRangeCursor(char* begin, char* end, std::ptrdiff_t debug_offset = 0)
        : DataRange(begin, end, debug_offset) {}

    DataRangeCursor(DataRange range) : DataRange(range) {}

    operator ConstDataRangeCursor() const {
        return ConstDataRangeCursor(ConstDataRange(_begin, _end, _debug_offset));
    }

    Status advance(size_t advance) {
        if (advance > length()) {
            return makeAdvanceStatus(advance);
        }

        _begin += advance;
        _debug_offset += advance;

        return Status::OK();
    }

    template <typename T>
    Status skip() {
        size_t advanced = 0;

        Status x = DataType::load<T>(nullptr, _begin, _end - _begin, &advanced, _debug_offset);

        if (x.isOK()) {
            _begin += advanced;
            _debug_offset += advanced;
        }

        return x;
    }

    template <typename T>
    Status readAndAdvance(T* t) {
        size_t advanced = 0;

        Status x = DataType::load(t, _begin, _end - _begin, &advanced, _debug_offset);

        if (x.isOK()) {
            _begin += advanced;
            _debug_offset += advanced;
        }

        return x;
    }

    template <typename T>
    StatusWith<T> readAndAdvance() {
        T out(DataType::defaultConstruct<T>());
        Status x = readAndAdvance(&out);

        if (x.isOK()) {
            return StatusWith<T>(std::move(out));
        } else {
            return StatusWith<T>(std::move(x));
        }
    }

    template <typename T>
    Status writeAndAdvance(const T& value) {
        size_t advanced = 0;

        Status x = DataType::store(
            value, const_cast<char*>(_begin), _end - _begin, &advanced, _debug_offset);

        if (x.isOK()) {
            _begin += advanced;
            _debug_offset += advanced;
        }

        return x;
    }

private:
    Status makeAdvanceStatus(size_t advance) const;
};

}  // namespace mongo
