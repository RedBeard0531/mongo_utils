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

#include "mongo/base/data_range_cursor.h"

#include "mongo/base/data_type_endian.h"
#include "mongo/platform/endian.h"
#include "mongo/unittest/unittest.h"

namespace mongo {

TEST(DataRangeCursor, ConstDataRangeCursor) {
    char buf[14];

    DataView(buf).write<uint16_t>(1);
    DataView(buf).write<LittleEndian<uint32_t>>(2, sizeof(uint16_t));
    DataView(buf).write<BigEndian<uint64_t>>(3, sizeof(uint16_t) + sizeof(uint32_t));

    ConstDataRangeCursor cdrc(buf, buf + sizeof(buf));
    ConstDataRangeCursor backup(cdrc);

    ASSERT_EQUALS(static_cast<uint16_t>(1), cdrc.readAndAdvance<uint16_t>().getValue());
    ASSERT_EQUALS(static_cast<uint32_t>(2),
                  cdrc.readAndAdvance<LittleEndian<uint32_t>>().getValue());
    ASSERT_EQUALS(static_cast<uint64_t>(3), cdrc.readAndAdvance<BigEndian<uint64_t>>().getValue());
    ASSERT_EQUALS(false, cdrc.readAndAdvance<char>().isOK());

    // test skip()
    cdrc = backup;
    ASSERT_EQUALS(true, cdrc.skip<uint32_t>().isOK());
    ;
    ASSERT_EQUALS(true, cdrc.advance(10).isOK());
    ASSERT_EQUALS(false, cdrc.readAndAdvance<char>().isOK());
}

TEST(DataRangeCursor, ConstDataRangeCursorType) {
    char buf[] = "foo";

    ConstDataRangeCursor cdrc(buf, buf + sizeof(buf));

    ConstDataRangeCursor out(nullptr, nullptr);

    auto inner = cdrc.read(&out);

    ASSERT_OK(inner);
    ASSERT_EQUALS(buf, out.data());
}

TEST(DataRangeCursor, DataRangeCursor) {
    char buf[100] = {0};

    DataRangeCursor dc(buf, buf + 14);

    ASSERT_EQUALS(true, dc.writeAndAdvance<uint16_t>(1).isOK());
    ASSERT_EQUALS(true, dc.writeAndAdvance<LittleEndian<uint32_t>>(2).isOK());
    ASSERT_EQUALS(true, dc.writeAndAdvance<BigEndian<uint64_t>>(3).isOK());
    ASSERT_EQUALS(false, dc.writeAndAdvance<char>(1).isOK());

    ConstDataRangeCursor cdrc(buf, buf + sizeof(buf));

    ASSERT_EQUALS(static_cast<uint16_t>(1), cdrc.readAndAdvance<uint16_t>().getValue());
    ASSERT_EQUALS(static_cast<uint32_t>(2),
                  cdrc.readAndAdvance<LittleEndian<uint32_t>>().getValue());
    ASSERT_EQUALS(static_cast<uint64_t>(3), cdrc.readAndAdvance<BigEndian<uint64_t>>().getValue());
    ASSERT_EQUALS(static_cast<char>(0), cdrc.readAndAdvance<char>().getValue());
}

TEST(DataRangeCursor, DataRangeCursorType) {
    char buf[] = "foo";
    char buf2[] = "barZ";

    DataRangeCursor drc(buf, buf + sizeof(buf) + -1);

    DataRangeCursor out(nullptr, nullptr);

    Status status = drc.read(&out);

    ASSERT_OK(status);
    ASSERT_EQUALS(buf, out.data());

    drc = DataRangeCursor(buf2, buf2 + sizeof(buf2) + -1);
    status = drc.write(out);

    ASSERT_OK(status);
    ASSERT_EQUALS(std::string("fooZ"), buf2);
}
}  // namespace mongo
