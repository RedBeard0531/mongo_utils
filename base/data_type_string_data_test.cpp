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

#include "mongo/base/data_type.h"

#include "mongo/base/data_range.h"
#include "mongo/base/data_range_cursor.h"
#include "mongo/base/data_type_terminated.h"
#include "mongo/unittest/unittest.h"

namespace mongo {

TEST(DataTypeStringData, Basic) {
    char buf[100];
    StringData a("a");
    StringData b("bb");
    StringData c("ccc");

    {
        DataRangeCursor drc(buf, buf + sizeof(buf));

        ASSERT_OK(drc.writeAndAdvance(Terminated<'\0', StringData>(a)));
        ASSERT_OK(drc.writeAndAdvance(Terminated<'\0', StringData>(b)));
        ASSERT_OK(drc.writeAndAdvance(Terminated<'\0', StringData>(c)));

        ASSERT_EQUALS(1 + 2 + 3 + 3, drc.data() - buf);
    }

    {
        ConstDataRangeCursor cdrc(buf, buf + sizeof(buf));

        Terminated<'\0', StringData> tsd;

        ASSERT_OK(cdrc.readAndAdvance(&tsd));
        ASSERT_EQUALS(a, tsd.value);

        ASSERT_OK(cdrc.readAndAdvance(&tsd));
        ASSERT_EQUALS(b, tsd.value);

        ASSERT_OK(cdrc.readAndAdvance(&tsd));
        ASSERT_EQUALS(c, tsd.value);
    }
}

}  // namespace mongo
