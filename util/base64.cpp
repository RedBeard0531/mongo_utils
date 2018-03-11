// util/base64.cpp


/*    Copyright 2009 10gen Inc.
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

#include "mongo/util/base64.h"

#include "mongo/util/assert_util.h"

#include <array>

namespace mongo {

using std::begin;
using std::end;
using std::string;
using std::stringstream;

namespace {
constexpr unsigned char kInvalid = -1;

const class Alphabet {
public:
    Alphabet() {
        decode.fill(kInvalid);
        for (size_t i = 0; i < encode.size(); ++i) {
            decode[encode[i]] = i;
        }
    }

    unsigned char e(std::uint8_t x) const {
        return encode[x & 0x3f];
    }

    std::uint8_t d(unsigned char x) const {
        auto const c = decode[x];
        uassert(40537, "Invalid base64 character", c != kInvalid);
        return c;
    }

    bool valid(unsigned char x) const {
        return decode[x] != kInvalid;
    }

private:
    StringData encode{
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/"};
    std::array<unsigned char, 256> decode;
} alphabet;
}  // namespace

void base64::encode(stringstream& ss, const char* data, int size) {
    for (int i = 0; i < size; i += 3) {
        int left = size - i;
        const unsigned char* start = (const unsigned char*)data + i;

        // byte 0
        ss << alphabet.e(start[0] >> 2);

        // byte 1
        unsigned char temp = (start[0] << 4);
        if (left == 1) {
            ss << alphabet.e(temp);
            break;
        }
        temp |= ((start[1] >> 4) & 0xF);
        ss << alphabet.e(temp);

        // byte 2
        temp = (start[1] & 0xF) << 2;
        if (left == 2) {
            ss << alphabet.e(temp);
            break;
        }
        temp |= ((start[2] >> 6) & 0x3);
        ss << alphabet.e(temp);

        // byte 3
        ss << alphabet.e(start[2] & 0x3f);
    }

    int mod = size % 3;
    if (mod == 1) {
        ss << "==";
    } else if (mod == 2) {
        ss << "=";
    }
}


string base64::encode(const char* data, int size) {
    stringstream ss;
    encode(ss, data, size);
    return ss.str();
}

string base64::encode(const string& s) {
    return encode(s.c_str(), s.size());
}


void base64::decode(stringstream& ss, const string& s) {
    uassert(10270, "invalid base64", s.size() % 4 == 0);
    auto const data = reinterpret_cast<const unsigned char*>(s.c_str());
    auto const size = s.size();
    bool done = false;

    for (size_t i = 0; i < size; i += 4) {
        uassert(
            40538, "Invalid Base64 stream. Additional data following terminating sequence.", !done);
        auto const start = data + i;
        done = (start[2] == '=') || (start[3] == '=');

        ss << (char)(((alphabet.d(start[0]) << 2) & 0xFC) | ((alphabet.d(start[1]) >> 4) & 0x3));
        if (start[2] != '=') {
            ss << (char)(((alphabet.d(start[1]) << 4) & 0xF0) |
                         ((alphabet.d(start[2]) >> 2) & 0xF));
            if (!done) {
                ss << (char)(((alphabet.d(start[2]) << 6) & 0xC0) |
                             ((alphabet.d(start[3]) & 0x3F)));
            }
        }
    }
}

string base64::decode(const string& s) {
    stringstream ss;
    decode(ss, s);
    return ss.str();
}

bool base64::validate(const StringData s) {
    if (s.size() % 4) {
        return false;
    }
    if (s.empty()) {
        return true;
    }

    auto const unwindTerminator = [](auto it) { return (*(it - 1) == '=') ? (it - 1) : it; };
    auto const e = unwindTerminator(unwindTerminator(end(s)));

    return e == std::find_if(begin(s), e, [](const char ch) { return !alphabet.valid(ch); });
}

}  // namespace mongo
