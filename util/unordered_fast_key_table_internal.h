// unordered_fast_key_table_internal.h


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

#include "mongo/util/unordered_fast_key_table.h"

namespace mongo {

template <typename K_L, typename K_S, typename V, typename Traits>
inline int UnorderedFastKeyTable<K_L, K_S, V, Traits>::Area::find(const HashedKey& key,
                                                                  int* firstEmpty) const {
    dassert(capacity());                        // Caller must special-case empty tables.
    dassert(!firstEmpty || *firstEmpty == -1);  // Caller must initialize *firstEmpty.

    unsigned probe = 0;
    do {
        unsigned pos = (key.hash() + probe) & _hashMask;

        if (!_entries[pos].isUsed()) {
            // space is empty
            if (firstEmpty && *firstEmpty == -1)
                *firstEmpty = pos;
            if (!_entries[pos].wasEverUsed())
                return -1;
            continue;
        }

        if (_entries[pos].getCurHash() != key.hash()) {
            // space has something else
            continue;
        }

        if (!Traits::equals(key.key(), Traits::toLookup(_entries[pos].getData().first))) {
            // hashes match
            // strings are not equals
            continue;
        }

        // hashes and strings are equal
        // yay!
        return pos;
    } while (++probe < _maxProbe);
    return -1;
}

template <typename K_L, typename K_S, typename V, typename Traits>
inline bool UnorderedFastKeyTable<K_L, K_S, V, Traits>::Area::transfer(Area* newArea) const {
    for (auto&& entry : *this) {
        if (!entry.isUsed())
            continue;

        int firstEmpty = -1;
        int loc = newArea->find(
            HashedKey(Traits::toLookup(entry.getData().first), entry.getCurHash()), &firstEmpty);

        verify(loc == -1);
        if (firstEmpty < 0) {
            return false;
        }

        newArea->_entries[firstEmpty] = entry;
    }
    return true;
}

template <typename K_L, typename K_S, typename V, typename Traits>
inline UnorderedFastKeyTable<K_L, K_S, V, Traits>::UnorderedFastKeyTable(
    std::initializer_list<std::pair<key_type, mapped_type>> entries)
    : UnorderedFastKeyTable() {
    for (auto&& entry : entries) {
        // Only insert the entry if the key is not equivalent to the key of any other element
        // already in the table.
        auto key = HashedKey(entry.first);
        if (find(key) == end()) {
            get(key) = entry.second;
        }
    }
}

template <typename K_L, typename K_S, typename V, typename Traits>
inline V& UnorderedFastKeyTable<K_L, K_S, V, Traits>::get(const HashedKey& key) {
    return try_emplace(key).first->second;
}

template <typename K_L, typename K_S, typename V, typename Traits>
inline size_t UnorderedFastKeyTable<K_L, K_S, V, Traits>::erase(const HashedKey& key) {
    if (_size == 0)
        return 0;  // Nothing to delete.

    int pos = _area.find(key, nullptr);

    if (pos < 0)
        return 0;

    --_size;
    _area._entries[pos].unUse();
    return 1;
}

template <typename K_L, typename K_S, typename V, typename Traits>
void UnorderedFastKeyTable<K_L, K_S, V, Traits>::erase(const_iterator it) {
    dassert(it._position >= 0);
    dassert(it._area == &_area);

    --_size;
    _area._entries[it._position].unUse();
}

template <typename K_L, typename K_S, typename V, typename Traits>
template <typename... Args>
inline auto UnorderedFastKeyTable<K_L, K_S, V, Traits>::try_emplace(const HashedKey& key,
                                                                    Args&&... args)
    -> std::pair<iterator, bool> {
    if (!_area._entries) {
        // This is the first insert ever. Need to allocate initial space.
        dassert(_area.capacity() == 0);
        _grow();
    }

    for (int numGrowTries = 0; numGrowTries < 5; numGrowTries++) {
        int firstEmpty = -1;
        int pos = _area.find(key, &firstEmpty);
        if (pos >= 0) {
            // This is only possible the first pass through the loop, since you're allocating space
            // for a new element after that.
            dassert(numGrowTries == 0);
            return {iterator(&_area, pos), false};
        }

        // key not in map
        // need to add
        if (firstEmpty >= 0) {
            _size++;
            _area._entries[firstEmpty].emplaceData(key, std::forward<Args>(args)...);
            return {iterator(&_area, firstEmpty), true};
        }

        // no space left in map
        _grow();
    }
    msgasserted(16471, "UnorderedFastKeyTable couldn't add entry after growing many times");
}

template <typename K_L, typename K_S, typename V, typename Traits>
inline void UnorderedFastKeyTable<K_L, K_S, V, Traits>::_grow() {
    unsigned capacity = _area.capacity();
    for (int numGrowTries = 0; numGrowTries < 5; numGrowTries++) {
        if (capacity == 0) {
            const unsigned kDefaultStartingCapacity = 16;
            capacity = kDefaultStartingCapacity;
        } else {
            capacity *= 2;
        }

        const double kMaxProbeRatio = 0.05;
        unsigned maxProbes = (capacity * kMaxProbeRatio) + 1;  // Round up

        Area newArea(capacity, maxProbes);
        bool success = _area.transfer(&newArea);
        if (!success) {
            continue;
        }
        _area.swap(&newArea);
        return;
    }
    msgasserted(16845, "UnorderedFastKeyTable::_grow couldn't add entry after growing many times");
}
}
