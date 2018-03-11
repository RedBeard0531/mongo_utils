/**
 * Copyright (c) 2011 10gen Inc.
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

#include "mongo/util/intrusive_counter.h"

#include "mongo/util/mongoutils/str.h"

namespace mongo {
using boost::intrusive_ptr;
using namespace mongoutils;

intrusive_ptr<const RCString> RCString::create(StringData s) {
    uassert(16493,
            str::stream() << "Tried to create string longer than "
                          << (BSONObjMaxUserSize / 1024 / 1024)
                          << "MB",
            s.size() < static_cast<size_t>(BSONObjMaxUserSize));

    const size_t sizeWithNUL = s.size() + 1;
    const size_t bytesNeeded = sizeof(RCString) + sizeWithNUL;

#pragma warning(push)
#pragma warning(disable : 4291)
    intrusive_ptr<RCString> ptr = new (bytesNeeded) RCString();  // uses custom operator new
#pragma warning(pop)

    ptr->_size = s.size();
    char* stringStart = reinterpret_cast<char*>(ptr.get()) + sizeof(RCString);
    s.copyTo(stringStart, true);

    return ptr;
}

void IntrusiveCounterUnsigned::addRef() const {
    ++counter;
}

void IntrusiveCounterUnsigned::release() const {
    if (!--counter)
        delete this;
}
}
