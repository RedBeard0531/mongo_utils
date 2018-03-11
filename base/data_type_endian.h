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

#include <cstring>
#include <type_traits>

#include "mongo/base/data_type.h"
#include "mongo/base/error_codes.h"
#include "mongo/platform/endian.h"

namespace mongo {

template <typename T>
struct IsEndian : std::false_type {};

#define MAKE_ENDIAN(name, loadFunc, storeFunc)             \
    template <typename T>                                  \
    struct name {                                          \
        using value_type = T;                              \
        name() : value(T{}) {}                             \
        name(T t) : value(t) {}                            \
        T value;                                           \
                                                           \
        operator T() const {                               \
            return value;                                  \
        }                                                  \
                                                           \
        static auto load(T t) -> decltype(loadFunc(t)) {   \
            return loadFunc(t);                            \
        }                                                  \
                                                           \
        static auto store(T t) -> decltype(storeFunc(t)) { \
            return storeFunc(t);                           \
        }                                                  \
    };                                                     \
                                                           \
    template <typename T>                                  \
    name<T> tag##name(T t) {                               \
        return t;                                          \
    }                                                      \
                                                           \
    template <typename T>                                  \
    struct IsEndian<name<T>> : std::true_type {};

/**
 * BigEndian and LittleEndian offer support for using natively encoded types
 * and reading/writing them as big endian or little endian through char ptrs.
 *
 * The Reverse variants assume the pointed to bytes are natively encoded and
 * return big or little endian encoded types.  I.e. you probably shouldn't use
 * them with floats and should be wary of using them with signed types.
 */
MAKE_ENDIAN(BigEndian, endian::bigToNative, endian::nativeToBig)
MAKE_ENDIAN(LittleEndian, endian::littleToNative, endian::nativeToLittle)
MAKE_ENDIAN(ReverseBigEndian, endian::nativeToBig, endian::bigToNative)
MAKE_ENDIAN(ReverseLittleEndian, endian::nativeToLittle, endian::littleToNative)

template <typename T>
struct DataType::Handler<T, typename std::enable_if<IsEndian<T>::value>::type> {
    static void unsafeLoad(T* t, const char* ptr, size_t* advanced) {
        if (t) {
            DataType::unsafeLoad(&t->value, ptr, advanced);

            t->value = T::load(t->value);
        } else {
            DataType::unsafeLoad(decltype(&t->value){nullptr}, ptr, advanced);
        }
    }

    static Status load(
        T* t, const char* ptr, size_t length, size_t* advanced, std::ptrdiff_t debug_offset) {
        if (t) {
            Status x = DataType::load(&t->value, ptr, length, advanced, debug_offset);

            if (x.isOK()) {
                t->value = T::load(t->value);
            }

            return x;
        } else {
            return DataType::load(
                decltype(&t->value){nullptr}, ptr, length, advanced, debug_offset);
        }
    }

    static void unsafeStore(const T& t, char* ptr, size_t* advanced) {
        DataType::unsafeStore(T::store(t.value), ptr, advanced);
    }

    static Status store(
        const T& t, char* ptr, size_t length, size_t* advanced, std::ptrdiff_t debug_offset) {
        return DataType::store(T::store(t.value), ptr, length, advanced, debug_offset);
    }

    static typename T::value_type defaultConstruct() {
        return DataType::defaultConstruct<typename T::value_type>();
    }
};

}  // namespace mongo
