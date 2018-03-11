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

#include "mongo/base/data_type_validated.h"

#include <algorithm>
#include <iterator>

#include "mongo/base/data_range.h"
#include "mongo/base/data_range_cursor.h"
#include "mongo/base/data_type_endian.h"
#include "mongo/base/status.h"
#include "mongo/db/jsobj.h"
#include "mongo/unittest/unittest.h"

namespace mongo {
template <>
struct Validator<char> {
    static Status validateLoad(const char* ptr, size_t length) {
        if ((length >= sizeof(char)) && (ptr[0] == 0xFU)) {
            return Status::OK();
        }
        return Status(ErrorCodes::BadValue, "bad");
    }

    static Status validateStore(const char& toStore) {
        if (toStore == 0xFU) {
            return Status::OK();
        }
        return Status(ErrorCodes::BadValue, "bad");
    }
};
}  // namespace mongo

namespace {

using namespace mongo;
using std::end;
using std::begin;

TEST(DataTypeValidated, SuccessfulValidation) {
    char buf[1];

    {
        DataRangeCursor drc(begin(buf), end(buf));
        ASSERT_OK(drc.writeAndAdvance(Validated<char>(0xFU)));
    }

    {
        Validated<char> valid;
        ConstDataRangeCursor cdrc(begin(buf), end(buf));
        ASSERT_OK(cdrc.readAndAdvance(&valid));
        ASSERT_EQUALS(valid.val, char{0xFU});
    }
}

TEST(DataTypeValidated, FailedValidation) {
    char buf[1];

    {
        DataRangeCursor drc(begin(buf), end(buf));
        ASSERT_NOT_OK(drc.writeAndAdvance(Validated<char>(0x01)));
    }

    buf[0] = char{0x01};

    {
        Validated<char> valid;
        ConstDataRangeCursor cdrc(begin(buf), end(buf));
        ASSERT_NOT_OK(cdrc.readAndAdvance(&valid));
    }
}

}  // namespace
