/*    Copyright 2014 MongoDB Inc.
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

#include "mongo/base/data_type.h"
#include "mongo/base/data_view.h"
#include "mongo/platform/endian.h"

namespace mongo {

class ConstDataCursor : public ConstDataView {
public:
    typedef ConstDataView view_type;

    ConstDataCursor(ConstDataView::bytes_type bytes) : ConstDataView(bytes) {}

    ConstDataCursor operator+(std::size_t s) const {
        return view() + s;
    }

    ConstDataCursor& operator+=(std::size_t s) {
        *this = view() + s;
        return *this;
    }

    ConstDataCursor operator-(std::size_t s) const {
        return view() - s;
    }

    ConstDataCursor& operator-=(std::size_t s) {
        *this = view() - s;
        return *this;
    }

    ConstDataCursor& operator++() {
        return operator+=(1);
    }

    ConstDataCursor operator++(int) {
        ConstDataCursor tmp = *this;
        operator++();
        return tmp;
    }

    ConstDataCursor& operator--() {
        return operator-=(1);
    }

    ConstDataCursor operator--(int) {
        ConstDataCursor tmp = *this;
        operator--();
        return tmp;
    }

    template <typename T>
    ConstDataCursor& skip() {
        size_t advance = 0;

        DataType::unsafeLoad<T>(nullptr, view(), &advance);
        *this += advance;

        return *this;
    }

    template <typename T>
    ConstDataCursor& readAndAdvance(T* t) {
        size_t advance = 0;

        DataType::unsafeLoad(t, view(), &advance);
        *this += advance;

        return *this;
    }

    template <typename T>
    T readAndAdvance() {
        T out(DataType::defaultConstruct<T>());
        readAndAdvance(&out);
        return out;
    }
};

class DataCursor : public DataView {
public:
    typedef DataView view_type;

    DataCursor(DataView::bytes_type bytes) : DataView(bytes) {}

    operator ConstDataCursor() const {
        return view();
    }

    DataCursor operator+(std::size_t s) const {
        return view() + s;
    }

    DataCursor& operator+=(std::size_t s) {
        *this = view() + s;
        return *this;
    }

    DataCursor operator-(std::size_t s) const {
        return view() - s;
    }

    DataCursor& operator-=(std::size_t s) {
        *this = view() - s;
        return *this;
    }

    DataCursor& operator++() {
        return operator+=(1);
    }

    DataCursor operator++(int) {
        DataCursor tmp = *this;
        operator++();
        return tmp;
    }

    DataCursor& operator--() {
        return operator-=(1);
    }

    DataCursor operator--(int) {
        DataCursor tmp = *this;
        operator--();
        return tmp;
    }

    template <typename T>
    DataCursor& skip() {
        size_t advance = 0;

        DataType::unsafeLoad<T>(nullptr, view(), &advance);
        *this += advance;

        return *this;
    }

    template <typename T>
    DataCursor& readAndAdvance(T* t) {
        size_t advance = 0;

        DataType::unsafeLoad(t, view(), &advance);
        *this += advance;

        return *this;
    }

    template <typename T>
    T readAndAdvance() {
        T out(DataType::defaultConstruct<T>());
        readAndAdvance(&out);
        return out;
    }

    template <typename T>
    DataCursor& writeAndAdvance(const T& value) {
        size_t advance = 0;

        DataType::unsafeStore(value, view(), &advance);
        *this += advance;

        return *this;
    }
};

}  // namespace mongo
