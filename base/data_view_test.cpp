/**
 *    Copyright (C) 2014 MongoDB Inc.
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

#include "mongo/base/data_view.h"

#include <cstring>

#include "mongo/base/data_type_endian.h"
#include "mongo/platform/endian.h"
#include "mongo/unittest/unittest.h"

namespace mongo {

TEST(DataView, ConstDataView) {
    char buf[sizeof(uint32_t) * 3];
    uint32_t native = 1234;
    uint32_t le = endian::nativeToLittle(native);
    uint32_t be = endian::nativeToBig(native);

    std::memcpy(buf, &native, sizeof(uint32_t));
    std::memcpy(buf + sizeof(uint32_t), &le, sizeof(uint32_t));
    std::memcpy(buf + sizeof(uint32_t) * 2, &be, sizeof(uint32_t));

    ConstDataView cdv(buf);

    ASSERT_EQUALS(buf, cdv.view());
    ASSERT_EQUALS(buf + 5, cdv.view(5));

    ASSERT_EQUALS(native, cdv.read<uint32_t>());
    ASSERT_EQUALS(native, cdv.read<LittleEndian<uint32_t>>(sizeof(uint32_t)));
    ASSERT_EQUALS(native, cdv.read<BigEndian<uint32_t>>(sizeof(uint32_t) * 2));
}

TEST(DataView, DataView) {
    char buf[sizeof(uint32_t) * 3];
    uint32_t native = 1234;

    DataView dv(buf);

    dv.write(native);
    dv.write(LittleEndian<uint32_t>(native), sizeof(uint32_t));
    dv.write(BigEndian<uint32_t>(native), sizeof(uint32_t) * 2);

    ASSERT_EQUALS(buf, dv.view());
    ASSERT_EQUALS(buf + 5, dv.view(5));

    ASSERT_EQUALS(native, dv.read<uint32_t>());
    ASSERT_EQUALS(native, dv.read<LittleEndian<uint32_t>>(sizeof(uint32_t)));
    ASSERT_EQUALS(native, dv.read<BigEndian<uint32_t>>(sizeof(uint32_t) * 2));
}

}  // namespace mongo
