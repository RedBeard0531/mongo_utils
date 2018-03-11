/**
 *    Copyright 2016 MongoDB Inc.
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

#define MONGO_LOG_DEFAULT_COMPONENT ::mongo::logger::LogComponent::kDefault

#include "mongo/platform/basic.h"

#include "mongo/platform/decimal128_bson_test.h"

#include <array>
#include <cmath>
#include <utility>

#include "mongo/bson/bsonelement.h"
#include "mongo/bson/bsonobj.h"
#include "mongo/bson/bsonobjbuilder.h"
#include "mongo/config.h"
#include "mongo/db/json.h"
#include "mongo/platform/decimal128.h"
#include "mongo/stdx/memory.h"
#include "mongo/unittest/unittest.h"
#include "mongo/util/hex.h"
#include "mongo/util/log.h"

namespace {
using namespace mongo;

BSONObj convertHexStringToBsonObj(StringData hexString) {
    const char* p = hexString.rawData();
    size_t bufferSize = hexString.size() / 2;
    auto buffer = SharedBuffer::allocate(bufferSize);

    for (unsigned int i = 0; i < bufferSize; i++) {
        buffer.get()[i] = fromHex(p);
        p += 2;
    }

    return BSONObj(std::move(buffer));
}

// Reconcile format differences between test data and output data.
std::string trimWhiteSpace(std::string str) {
    std::string result;
    for (size_t i = 0; i < str.size(); i++) {
        if (str[i] != ' ') {
            result += str[i];
        }
    }
    return result;
}

TEST(Decimal128BSONTest, TestsConstructingDecimalWithBsonDump) {
    BSONObj allData = fromjson(testData);
    BSONObj data = allData.getObjectField("valid");
    BSONObjIterator it(data);

    while (it.moreWithEOO()) {
        BSONElement testCase = it.next();
        if (testCase.eoo()) {
            break;
        }
        if (testCase.type() == Object) {
            BSONObj b = testCase.Obj();
            BSONElement desc = b.getField("description");
            BSONElement bson = b.getField("bson");
            BSONElement extjson = b.getField("extjson");
            BSONElement canonical_extjson = b.getField("canonical_extjson");

            log() << "Test - " << desc.str();

            StringData hexString = bson.valueStringData();
            BSONObj d = convertHexStringToBsonObj(hexString);
            std::string outputJson = d.jsonString();
            std::string expectedJson;

            if (!canonical_extjson.eoo()) {
                expectedJson = canonical_extjson.str();
            } else {
                expectedJson = extjson.str();
            }

            ASSERT_EQ(trimWhiteSpace(outputJson), trimWhiteSpace(expectedJson));
            log() << "PASSED";
        }
    }
}
}  // namespace
