/*    Copyright 2016 MongoDB, Inc.
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

#include "mongo/util/duration.h"

#include <iostream>

#include "mongo/bson/util/builder.h"

namespace mongo {
namespace {
template <typename Stream>
Stream& streamPut(Stream& os, Nanoseconds ns) {
    return os << ns.count() << "ns";
}

template <typename Stream>
Stream& streamPut(Stream& os, Microseconds us) {
    return os << us.count() << "\xce\xbcs";
}

template <typename Stream>
Stream& streamPut(Stream& os, Milliseconds ms) {
    return os << ms.count() << "ms";
}

template <typename Stream>
Stream& streamPut(Stream& os, Seconds s) {
    return os << s.count() << 's';
}

template <typename Stream>
Stream& streamPut(Stream& os, Minutes min) {
    return os << min.count() << "min";
}

template <typename Stream>
Stream& streamPut(Stream& os, Hours hrs) {
    return os << hrs.count() << "hr";
}

}  // namespace

std::ostream& operator<<(std::ostream& os, Nanoseconds ns) {
    return streamPut(os, ns);
}

std::ostream& operator<<(std::ostream& os, Microseconds us) {
    return streamPut(os, us);
}

std::ostream& operator<<(std::ostream& os, Milliseconds ms) {
    return streamPut(os, ms);
}
std::ostream& operator<<(std::ostream& os, Seconds s) {
    return streamPut(os, s);
}

std::ostream& operator<<(std::ostream& os, Minutes m) {
    return streamPut(os, m);
}

std::ostream& operator<<(std::ostream& os, Hours h) {
    return streamPut(os, h);
}

template <typename Allocator>
StringBuilderImpl<Allocator>& operator<<(StringBuilderImpl<Allocator>& os, Nanoseconds ns) {
    return streamPut(os, ns);
}

template <typename Allocator>
StringBuilderImpl<Allocator>& operator<<(StringBuilderImpl<Allocator>& os, Microseconds us) {
    return streamPut(os, us);
}

template <typename Allocator>
StringBuilderImpl<Allocator>& operator<<(StringBuilderImpl<Allocator>& os, Milliseconds ms) {
    return streamPut(os, ms);
}

template <typename Allocator>
StringBuilderImpl<Allocator>& operator<<(StringBuilderImpl<Allocator>& os, Seconds s) {
    return streamPut(os, s);
}

template <typename Allocator>
StringBuilderImpl<Allocator>& operator<<(StringBuilderImpl<Allocator>& os, Minutes m) {
    return streamPut(os, m);
}

template <typename Allocator>
StringBuilderImpl<Allocator>& operator<<(StringBuilderImpl<Allocator>& os, Hours h) {
    return streamPut(os, h);
}

template StringBuilderImpl<StackAllocator>& operator<<(StringBuilderImpl<StackAllocator>&,
                                                       Nanoseconds);
template StringBuilderImpl<StackAllocator>& operator<<(StringBuilderImpl<StackAllocator>&,
                                                       Microseconds);
template StringBuilderImpl<StackAllocator>& operator<<(StringBuilderImpl<StackAllocator>&,
                                                       Milliseconds);
template StringBuilderImpl<StackAllocator>& operator<<(StringBuilderImpl<StackAllocator>&, Seconds);
template StringBuilderImpl<StackAllocator>& operator<<(StringBuilderImpl<StackAllocator>&, Minutes);
template StringBuilderImpl<StackAllocator>& operator<<(StringBuilderImpl<StackAllocator>&, Hours);
template StringBuilderImpl<SharedBufferAllocator>& operator<<(
    StringBuilderImpl<SharedBufferAllocator>&, Nanoseconds);
template StringBuilderImpl<SharedBufferAllocator>& operator<<(
    StringBuilderImpl<SharedBufferAllocator>&, Microseconds);
template StringBuilderImpl<SharedBufferAllocator>& operator<<(
    StringBuilderImpl<SharedBufferAllocator>&, Milliseconds);
template StringBuilderImpl<SharedBufferAllocator>& operator<<(
    StringBuilderImpl<SharedBufferAllocator>&, Seconds);
template StringBuilderImpl<SharedBufferAllocator>& operator<<(
    StringBuilderImpl<SharedBufferAllocator>&, Minutes);
template StringBuilderImpl<SharedBufferAllocator>& operator<<(
    StringBuilderImpl<SharedBufferAllocator>&, Hours);
}  // namespace mongo
