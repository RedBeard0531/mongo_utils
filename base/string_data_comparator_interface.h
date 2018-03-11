/**
 *    Copyright (C) 2016 MongoDB Inc.
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

#include "mongo/base/disallow_copying.h"
#include "mongo/stdx/unordered_map.h"
#include "mongo/stdx/unordered_set.h"
#include "string_data.h"

namespace mongo {

/**
 * A StringData::ComparatorInterface is an abstract class for comparing StringData objects.
 */
class StringData::ComparatorInterface {
    MONGO_DISALLOW_COPYING(ComparatorInterface);

public:
    /**
     * Functor for checking string equality under this comparator. Compatible for use with unordered
     * STL containers.
     */
    class EqualTo {
    public:
        explicit EqualTo(const ComparatorInterface* stringComparator)
            : _stringComparator(stringComparator) {}

        bool operator()(StringData lhs, StringData rhs) const {
            return _stringComparator->compare(lhs, rhs) == 0;
        }

    private:
        const ComparatorInterface* _stringComparator;
    };

    /**
     * Functor for hashing strings under this comparator. Compatible for use with unordered STL
     * containers.
     */
    class Hasher {
    public:
        explicit Hasher(const ComparatorInterface* stringComparator)
            : _stringComparator(stringComparator) {}

        size_t operator()(StringData stringToHash) const {
            return _stringComparator->hash(stringToHash);
        }

    private:
        const ComparatorInterface* _stringComparator;
    };

    using StringDataUnorderedSet = stdx::unordered_set<StringData, Hasher, EqualTo>;

    template <typename T>
    using StringDataUnorderedMap = stdx::unordered_map<StringData, T, Hasher, EqualTo>;

    ComparatorInterface() = default;

    virtual ~ComparatorInterface() = default;

    /**
     * Compares two StringData objects.
     */
    virtual int compare(StringData left, StringData right) const = 0;

    /**
     * Hash a StringData in a way that respects this comparator.
     */
    size_t hash(StringData stringToHash) const {
        size_t seed = 0;
        hash_combine(seed, stringToHash);
        return seed;
    }

    /**
     * Hash a StringData in a way that respects this comparator, and return the result in the 'seed'
     * in-out parameter.
     */
    virtual void hash_combine(size_t& seed, StringData stringToHash) const = 0;

    /**
     * Returns a function object which can evaluate string equality according to this comparator.
     * This comparator must outlive the returned function object.
     */
    EqualTo makeEqualTo() const {
        return EqualTo(this);
    }

    /**
     * Returns a function object which can hash strings according to this comparator. This
     * comparator must outlive the returned function object.
     */
    Hasher makeHasher() const {
        return Hasher(this);
    }

    /**
     * Construct an empty unordered set of StringData whose equivalence classes are given by this
     * comparator. This comparator must outlive the returned set.
     */
    StringDataUnorderedSet makeStringDataUnorderedSet() const {
        return StringDataUnorderedSet(0, makeHasher(), makeEqualTo());
    }

    /**
     * Construct an empty unordered map from StringData to type T whose equivalence classes are
     * given by this comparator. This comparator must outlive the returned set.
     */
    template <typename T>
    StringDataUnorderedMap<T> makeStringDataUnorderedMap() const {
        return StringDataUnorderedMap<T>(0, makeHasher(), makeEqualTo());
    }
};

using StringDataUnorderedSet = StringData::ComparatorInterface::StringDataUnorderedSet;

template <typename T>
using StringDataUnorderedMap = StringData::ComparatorInterface::StringDataUnorderedMap<T>;

}  // namespace mongo
