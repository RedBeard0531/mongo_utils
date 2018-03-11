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

#include "mongo/base/data_type.h"

namespace mongo {

template <char C, typename T>
struct Terminated {
    Terminated() : value(DataType::defaultConstruct<T>()) {}
    Terminated(T value) : value(std::move(value)) {}
    T value;

    operator T() const {
        return value;
    }
};

struct TerminatedHelper {
    static Status makeLoadNoTerminalStatus(char c, size_t length, std::ptrdiff_t debug_offset);
    static Status makeLoadShortReadStatus(char c,
                                          size_t read,
                                          size_t length,
                                          std::ptrdiff_t debug_offset);
    static Status makeStoreStatus(char c, size_t length, std::ptrdiff_t debug_offset);
};

template <char C, typename T>
struct DataType::Handler<Terminated<C, T>> {
    using TerminatedType = Terminated<C, T>;

    static Status load(TerminatedType* tt,
                       const char* ptr,
                       size_t length,
                       size_t* advanced,
                       std::ptrdiff_t debug_offset) {
        size_t local_advanced = 0;

        const char* end = static_cast<const char*>(std::memchr(ptr, C, length));

        if (!end) {
            return TerminatedHelper::makeLoadNoTerminalStatus(C, length, debug_offset);
        }

        auto status = DataType::load(
            tt ? &tt->value : nullptr, ptr, end - ptr, &local_advanced, debug_offset);

        if (!status.isOK()) {
            return status;
        }

        if (local_advanced != static_cast<size_t>(end - ptr)) {
            return TerminatedHelper::makeLoadShortReadStatus(
                C, local_advanced, end - ptr, debug_offset);
        }

        if (advanced) {
            *advanced = local_advanced + 1;
        }

        return Status::OK();
    }

    static Status store(const TerminatedType& tt,
                        char* ptr,
                        size_t length,
                        size_t* advanced,
                        std::ptrdiff_t debug_offset) {
        size_t local_advanced = 0;

        auto status = DataType::store(tt.value, ptr, length, &local_advanced, debug_offset);

        if (!status.isOK()) {
            return status;
        }

        if (length - local_advanced < 1) {
            return TerminatedHelper::makeStoreStatus(C, length, debug_offset + local_advanced);
        }

        ptr[local_advanced] = C;

        if (advanced) {
            *advanced = local_advanced + 1;
        }

        return Status::OK();
    }

    static TerminatedType defaultConstruct() {
        return TerminatedType();
    }
};

}  // namespace mongo
