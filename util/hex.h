// util/hex.h

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

#pragma once

#include <algorithm>
#include <cctype>
#include <string>

#include "mongo/base/string_data.h"
#include "mongo/bson/util/builder.h"

namespace mongo {
// can't use hex namespace because it conflicts with hex iostream function
inline int fromHex(char c) {
    if ('0' <= c && c <= '9')
        return c - '0';
    if ('a' <= c && c <= 'f')
        return c - 'a' + 10;
    if ('A' <= c && c <= 'F')
        return c - 'A' + 10;
    verify(false);
    return 0xff;
}
inline char fromHex(const char* c) {
    return (char)((fromHex(c[0]) << 4) | fromHex(c[1]));
}
inline char fromHex(StringData c) {
    return (char)((fromHex(c[0]) << 4) | fromHex(c[1]));
}

/**
 * Decodes 'hexString' into raw bytes, appended to the out parameter 'buf'. Callers must first
 * ensure that 'hexString' is a valid hex encoding.
 */
inline void fromHexString(StringData hexString, BufBuilder* buf) {
    invariant(hexString.size() % 2 == 0);
    // Combine every pair of two characters into one byte.
    for (std::size_t i = 0; i < hexString.size(); i += 2) {
        buf->appendChar(fromHex(StringData(&hexString.rawData()[i], 2)));
    }
}

/**
 * Returns true if 'hexString' is a valid hexadecimal encoding.
 */
inline bool isValidHex(StringData hexString) {
    // There must be an even number of characters, since each pair encodes a single byte.
    return hexString.size() % 2 == 0 &&
        std::all_of(hexString.begin(), hexString.end(), [](char c) { return std::isxdigit(c); });
}

inline std::string toHex(const void* inRaw, int len) {
    static const char hexchars[] = "0123456789ABCDEF";

    StringBuilder out;
    const char* in = reinterpret_cast<const char*>(inRaw);
    for (int i = 0; i < len; ++i) {
        char c = in[i];
        char hi = hexchars[(c & 0xF0) >> 4];
        char lo = hexchars[(c & 0x0F)];

        out << hi << lo;
    }

    return out.str();
}

template <typename T>
std::string integerToHex(T val);

inline std::string toHexLower(const void* inRaw, int len) {
    static const char hexchars[] = "0123456789abcdef";

    StringBuilder out;
    const char* in = reinterpret_cast<const char*>(inRaw);
    for (int i = 0; i < len; ++i) {
        char c = in[i];
        char hi = hexchars[(c & 0xF0) >> 4];
        char lo = hexchars[(c & 0x0F)];

        out << hi << lo;
    }

    return out.str();
}

/* @return a dump of the buffer as hex byte ascii output */
std::string hexdump(const char* data, unsigned len);
}
