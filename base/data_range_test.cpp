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

#include "mongo/base/data_range.h"

#include <cstring>

#include "mongo/base/data_type_endian.h"
#include "mongo/platform/endian.h"
#include "mongo/unittest/unittest.h"

namespace mongo {

TEST(DataRange, ConstDataRange) {
    char buf[sizeof(uint32_t) * 3];
    uint32_t native = 1234;
    uint32_t le = endian::nativeToLittle(native);
    uint32_t be = endian::nativeToBig(native);

    std::memcpy(buf, &native, sizeof(uint32_t));
    std::memcpy(buf + sizeof(uint32_t), &le, sizeof(uint32_t));
    std::memcpy(buf + sizeof(uint32_t) * 2, &be, sizeof(uint32_t));

    ConstDataRange cdv(buf, buf + sizeof(buf));

    ASSERT_EQUALS(native, cdv.read<uint32_t>().getValue());
    ASSERT_EQUALS(native, cdv.read<LittleEndian<uint32_t>>(sizeof(uint32_t)).getValue());
    ASSERT_EQUALS(native, cdv.read<BigEndian<uint32_t>>(sizeof(uint32_t) * 2).getValue());

    auto result = cdv.read<uint32_t>(sizeof(uint32_t) * 3);
    ASSERT_EQUALS(false, result.isOK());
    ASSERT_EQUALS(ErrorCodes::Overflow, result.getStatus().code());
}

TEST(DataRange, ConstDataRangeType) {
    char buf[] = "foo";

    ConstDataRange cdr(buf, buf + sizeof(buf));

    ConstDataRange out(nullptr, nullptr);

    auto inner = cdr.read(&out);

    ASSERT_OK(inner);
    ASSERT_EQUALS(buf, out.data());
}

TEST(DataRange, DataRange) {
    char buf[sizeof(uint32_t) * 3];
    uint32_t native = 1234;

    DataRange dv(buf, buf + sizeof(buf));

    ASSERT_EQUALS(true, dv.write(native).isOK());
    ASSERT_EQUALS(true, dv.write(LittleEndian<uint32_t>(native), sizeof(uint32_t)).isOK());
    ASSERT_EQUALS(true, dv.write(BigEndian<uint32_t>(native), sizeof(uint32_t) * 2).isOK());

    auto result = dv.write(native, sizeof(uint32_t) * 3);
    ASSERT_EQUALS(false, result.isOK());
    ASSERT_EQUALS(ErrorCodes::Overflow, result.code());

    ASSERT_EQUALS(native, dv.read<uint32_t>().getValue());
    ASSERT_EQUALS(native, dv.read<LittleEndian<uint32_t>>(sizeof(uint32_t)).getValue());
    ASSERT_EQUALS(native, dv.read<BigEndian<uint32_t>>(sizeof(uint32_t) * 2).getValue());

    ASSERT_EQUALS(false, dv.read<uint32_t>(sizeof(uint32_t) * 3).isOK());
}

TEST(DataRange, DataRangeType) {
    char buf[] = "foo";
    char buf2[] = "barZ";

    DataRange dr(buf, buf + sizeof(buf) + -1);

    DataRange out(nullptr, nullptr);

    Status status = dr.read(&out);

    ASSERT_OK(status);
    ASSERT_EQUALS(buf, out.data());

    dr = DataRange(buf2, buf2 + sizeof(buf2) + -1);
    status = dr.write(out);

    ASSERT_OK(status);
    ASSERT_EQUALS(std::string("fooZ"), buf2);
}

}  // namespace mongo
