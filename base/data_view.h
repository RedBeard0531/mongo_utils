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

#include <cstring>
#include <type_traits>

#include "mongo/config.h"

#include "mongo/base/data_type.h"

namespace mongo {

class ConstDataView {
public:
    typedef const char* bytes_type;

    ConstDataView(bytes_type bytes) : _bytes(bytes) {}

    bytes_type view(std::size_t offset = 0) const {
        return _bytes + offset;
    }

    template <typename T>
    const ConstDataView& read(T* t, size_t offset = 0) const {
        DataType::unsafeLoad(t, view(offset), nullptr);

        return *this;
    }

    template <typename T>
    T read(std::size_t offset = 0) const {
        T t(DataType::defaultConstruct<T>());

        read(&t, offset);

        return t;
    }

private:
    bytes_type _bytes;
};

class DataView : public ConstDataView {
public:
    typedef char* bytes_type;

    DataView(bytes_type bytes) : ConstDataView(bytes) {}

    bytes_type view(std::size_t offset = 0) const {
        // It is safe to cast away const here since the pointer stored in our base class was
        // originally non-const by way of our constructor.
        return const_cast<bytes_type>(ConstDataView::view(offset));
    }

    template <typename T>
    DataView& write(const T& value, std::size_t offset = 0) {
        DataType::unsafeStore(value, view(offset), nullptr);

        return *this;
    }
};

}  // namespace mongo
