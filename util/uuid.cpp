/**
 *    Copyright (C) 2017 MongoDB Inc.
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

#include "mongo/platform/basic.h"

#include <regex>

#include "mongo/util/uuid.h"

#include "mongo/bson/bsonobjbuilder.h"
#include "mongo/platform/random.h"
#include "mongo/stdx/mutex.h"
#include "mongo/util/hex.h"

namespace mongo {

namespace {

stdx::mutex uuidGenMutex;
auto uuidGen = SecureRandom::create();

// Regex to match valid version 4 UUIDs with variant bits set
std::regex uuidRegex("[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}",
                     std::regex::optimize);

}  // namespace

StatusWith<UUID> UUID::parse(BSONElement from) {
    try {
        return UUID{from.uuid()};
    } catch (const AssertionException& e) {
        return e.toStatus();
    }
}

StatusWith<UUID> UUID::parse(const std::string& s) {
    if (!isUUIDString(s)) {
        return {ErrorCodes::InvalidUUID, "Invalid UUID string: " + s};
    }

    UUIDStorage uuid;

    // 4 Octets - 2 Octets - 2 Octets - 2 Octets - 6 Octets
    int j = 0;
    for (int i = 0; i < UUID::kNumBytes; i++) {
        // Skip hyphens
        if (s[j] == '-')
            j++;

        char high = s[j++];
        char low = s[j++];

        uuid[i] = ((fromHex(high) << 4) | fromHex(low));
    }

    return UUID{std::move(uuid)};
}

UUID UUID::parse(const BSONObj& obj) {
    auto res = parse(obj.getField("uuid"));
    uassert(40566, res.getStatus().reason(), res.isOK());
    return res.getValue();
}

bool UUID::isUUIDString(const std::string& s) {
    return std::regex_match(s, uuidRegex);
}

bool UUID::isRFC4122v4() const {
    return (_uuid[6] & ~0x0f) == 0x40 && (_uuid[8] & ~0x3f) == 0x80;  // See RFC 4122, section 4.4.
}

UUID UUID::gen() {
    int64_t randomWords[2];

    {
        stdx::lock_guard<stdx::mutex> lk(uuidGenMutex);

        // Generate 128 random bits
        randomWords[0] = uuidGen->nextInt64();
        randomWords[1] = uuidGen->nextInt64();
    }

    UUIDStorage randomBytes;
    memcpy(&randomBytes, randomWords, sizeof(randomBytes));

    // Set version in high 4 bits of byte 6 and variant in high 2 bits of byte 8, see RFC 4122,
    // section 4.1.1, 4.1.2 and 4.1.3.
    randomBytes[6] &= 0x0f;
    randomBytes[6] |= 0x40;  // v4
    randomBytes[8] &= 0x3f;
    randomBytes[8] |= 0x80;  // Randomly assigned

    return UUID{randomBytes};
}

void UUID::appendToBuilder(BSONObjBuilder* builder, StringData name) const {
    builder->appendBinData(name, sizeof(UUIDStorage), BinDataType::newUUID, &_uuid);
}

BSONObj UUID::toBSON() const {
    BSONObjBuilder builder;
    appendToBuilder(&builder, "uuid");
    return builder.obj();
}

std::string UUID::toString() const {
    StringBuilder ss;

    // 4 Octets - 2 Octets - 2 Octets - 2 Octets - 6 Octets
    ss << toHexLower(&_uuid[0], 4);
    ss << "-";
    ss << toHexLower(&_uuid[4], 2);
    ss << "-";
    ss << toHexLower(&_uuid[6], 2);
    ss << "-";
    ss << toHexLower(&_uuid[8], 2);
    ss << "-";
    ss << toHexLower(&_uuid[10], 6);

    return ss.str();
}

template <>
BSONObjBuilder& BSONObjBuilderValueStream::operator<<<UUID>(UUID value) {
    value.appendToBuilder(_builder, _fieldName);
    _fieldName = StringData();
    return *_builder;
}

}  // namespace mongo
