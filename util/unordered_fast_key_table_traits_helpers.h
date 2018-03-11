/*    Copyright 2016 MongoDB Inc.
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

#include "mongo/util/unordered_fast_key_table.h"

namespace mongo {

template <typename Key, typename Hasher>
struct UnorderedFastKeyTableTraitsFactoryForPtrKey {
    struct Traits {
        static uint32_t hash(const Key* a) {
            return Hasher()(*a);
        }

        static bool equals(const Key* a, const Key* b) {
            return *a == *b;
        }

        static Key toStorage(const Key* s) {
            return *s;
        }

        static const Key* toLookup(const Key& s) {
            return &s;
        }

        class HashedKey {
        public:
            HashedKey() = default;

            explicit HashedKey(const Key* key) : _key(key), _hash(Traits::hash(_key)) {}

            HashedKey(const Key* key, uint32_t hash) : _key(key), _hash(hash) {
                // If you claim to know the hash, it better be correct.
                dassert(_hash == Traits::hash(_key));
            }

            const Key* key() const {
                return _key;
            }

            uint32_t hash() const {
                return _hash;
            }

        private:
            const Key* _key = nullptr;
            uint32_t _hash = 0;
        };
    };

    template <typename V>
    using type = UnorderedFastKeyTable<Key*, Key, V, Traits>;
};

/**
 * Provides a Hasher which forwards to an instance's .hash() method.  This should only be used with
 * high quality hashing functions because UnorderedFastKeyMap uses bit masks, rather than % by
 * prime, which can provide poor behavior without good overall distribution.
 */
template <typename T>
struct UnorderedFastKeyTableInstanceMethodHasher {
    auto operator()(const T& t) const -> decltype(t.hash()) {
        return t.hash();
    }
};
}
