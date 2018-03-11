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

#include "mongo/platform/basic.h"

#include "mongo/base/simple_string_data_comparator.h"

#include <third_party/murmurhash3/MurmurHash3.h>

#include "mongo/base/data_type_endian.h"
#include "mongo/base/data_view.h"

namespace mongo {

namespace {

template <int SizeofSizeT>
size_t murmur3(StringData str, size_t seed);

template <>
size_t murmur3<4>(StringData str, size_t seed) {
    char hash[4];
    MurmurHash3_x86_32(str.rawData(), str.size(), seed, &hash);
    return ConstDataView(hash).read<LittleEndian<std::uint32_t>>();
}

template <>
size_t murmur3<8>(StringData str, size_t seed) {
    char hash[16];
    MurmurHash3_x64_128(str.rawData(), str.size(), seed, hash);
    return static_cast<size_t>(ConstDataView(hash).read<LittleEndian<std::uint64_t>>());
}

}  // namespace

const SimpleStringDataComparator SimpleStringDataComparator::kInstance{};

int SimpleStringDataComparator::compare(StringData left, StringData right) const {
    return left.compare(right);
}

void SimpleStringDataComparator::hash_combine(size_t& seed, StringData stringToHash) const {
    seed = murmur3<sizeof(size_t)>(stringToHash, seed);
}

}  // namespace mongo
