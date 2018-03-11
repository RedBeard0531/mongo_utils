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

#include <utility>

#include "mongo/base/data_type.h"

namespace mongo {

/**
 * Allows for specializations of load/store that run validation logic.
 *
 * To add validation for your T:
 * 1) ensure that there are DataType::Handler<T> specializations for your type
 * 2) implement a specialization of Validator<T> for your type. The two methods
 * you must implement are:
 *    - Status validateLoad(const char* ptr, size_t length);
 *    - Status validateStore(const T& toStore);
 *
 * See bson_validate.h for an example.
 *
 * Then you can use Validated<T> in a DataRange (and associated types)
 *
 * Example:
 *
 *     DataRangeCursor drc(buf, buf_end);
 *     Validated<MyObj> vobj;
 *     auto status = drc.readAndAdvance(&vobj);
 *     if (status.isOK()) {
 *         // use vobj.val
 *         // ....
 *     }
 */
template <typename T>
struct Validator {
    // These methods are intentionally unimplemented so that if the default validator
    // is instantiated, the resulting binary will not link.

    /**
     * Checks that the provided buffer contains at least 1 valid object of type T.
     * The length parameter is the size of the buffer, not the size of the object.
     * Specializations of this function should be hardened to malicious input from untrusted
     * sources.
     */
    static Status validateLoad(const char* ptr, size_t length);

    /**
     * Checks that the provided object is valid to store in a buffer.
     */
    static Status validateStore(const T& toStore);
};

template <typename T>
struct Validated {
    Validated() = default;
    Validated(T value) : val(std::move(value)) {}

    operator T&() {
        return val;
    }

    T val = DataType::defaultConstruct<T>();
};

template <typename T>
struct DataType::Handler<Validated<T>> {
    static Status load(Validated<T>* vt,
                       const char* ptr,
                       size_t length,
                       size_t* advanced,
                       std::ptrdiff_t debug_offset) {
        size_t local_advanced = 0;

        auto valid = Validator<T>::validateLoad(ptr, length);

        if (!valid.isOK()) {
            return valid;
        }

        auto loadStatus =
            DataType::load(vt ? &vt->val : nullptr, ptr, length, &local_advanced, debug_offset);

        if (!loadStatus.isOK()) {
            return loadStatus;
        }

        if (advanced) {
            *advanced = local_advanced;
        }

        return Status::OK();
    }

    static Status store(const Validated<T>& vt,
                        char* ptr,
                        size_t length,
                        size_t* advanced,
                        std::ptrdiff_t debug_offset) {
        size_t local_advanced = 0;

        auto valid = Validator<T>::validateStore(vt.val);

        if (!valid.isOK()) {
            return valid;
        }

        auto storeStatus = DataType::store(vt.val, ptr, length, &local_advanced, debug_offset);

        if (!storeStatus.isOK()) {
            return storeStatus;
        }

        if (advanced) {
            *advanced = local_advanced;
        }

        return Status::OK();
    }

    static Validated<T> defaultConstruct() {
        return Validated<T>();
    }
};

}  // namespace mongo
