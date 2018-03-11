/**
 * Copyright (C) 2015 MongoDB Inc.
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

#include <atomic>
#include <cstdint>
#include <cstring>
#include <type_traits>

#include "mongo/base/static_assert.h"
#include "mongo/config.h"

namespace mongo {

/**
* Provides a simple version of an atomic version of T
* that uses std::atomic<BaseWordT> as a backing type;
*/
template <typename T, typename BaseWordT>
class AtomicProxy {
    MONGO_STATIC_ASSERT_MSG(sizeof(T) == sizeof(BaseWordT),
                            "T and BaseWordT must have the same size");
    MONGO_STATIC_ASSERT_MSG(std::is_integral<BaseWordT>::value,
                            "BaseWordT must be an integral type");
#if MONGO_HAVE_STD_IS_TRIVIALLY_COPYABLE
    MONGO_STATIC_ASSERT_MSG(std::is_trivially_copyable<T>::value, "T must be trivially copyable");
#endif
public:
    using value_type = T;
    using base_type = BaseWordT;

    explicit AtomicProxy(T value = 0) {
        store(value);
    }

    T operator=(T value) {
        store(value);
        return value;
    }

    operator T() const {
        return load();
    }

    T load(std::memory_order order = std::memory_order_seq_cst) const {
        const BaseWordT tempInteger = _value.load(order);
        T value;
        std::memcpy(&value, &tempInteger, sizeof(T));
        return value;
    }

    void store(const T value, std::memory_order order = std::memory_order_seq_cst) {
        BaseWordT tempInteger;
        std::memcpy(&tempInteger, &value, sizeof(T));
        _value.store(tempInteger, order);
    }

private:
    std::atomic<BaseWordT> _value;  // NOLINT
};

using AtomicDouble = AtomicProxy<double, std::uint64_t>;
}
