// string_map.h

/*    Copyright 2012 10gen Inc.
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

#include <third_party/murmurhash3/MurmurHash3.h>

#include "mongo/base/string_data.h"
#include "mongo/util/assert_util.h"
#include "mongo/util/unordered_fast_key_table.h"

namespace mongo {

struct StringMapTraits {
    static uint32_t hash(StringData a) {
        uint32_t hash;
        MurmurHash3_x86_32(a.rawData(), a.size(), 0, &hash);
        return hash;
    }

    static bool equals(StringData a, StringData b) {
        return a == b;
    }

    static std::string toStorage(StringData s) {
        return s.toString();
    }

    static StringData toLookup(const std::string& s) {
        return StringData(s);
    }

    class HashedKey {
    public:
        explicit HashedKey(StringData key = "") : _key(key), _hash(StringMapTraits::hash(_key)) {}

        HashedKey(StringData key, uint32_t hash) : _key(key), _hash(hash) {
            // If you claim to know the hash, it better be correct.
            dassert(_hash == StringMapTraits::hash(_key));
        }

        const StringData& key() const {
            return _key;
        }

        uint32_t hash() const {
            return _hash;
        }

    private:
        StringData _key;
        uint32_t _hash;
    };
};

template <typename V>
using StringMap = UnorderedFastKeyTable<StringData,   // K_L
                                        std::string,  // K_S
                                        V,
                                        StringMapTraits>;

}  // namespace mongo
