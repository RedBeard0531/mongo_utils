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

#pragma once

#include <boost/intrusive_ptr.hpp>
#include <stdlib.h>

#include "mongo/base/disallow_copying.h"
#include "mongo/base/string_data.h"
#include "mongo/platform/atomic_word.h"
#include "mongo/util/allocator.h"

namespace mongo {

/*
  IntrusiveCounter is a sharable implementation of a reference counter that
  objects can use to be compatible with boost::intrusive_ptr<>.

  Some objects that use IntrusiveCounter are immutable, and only have
  const methods.  This may require their pointers to be declared as
  intrusive_ptr<const ClassName> .  In order to be able to share pointers to
  these immutables, the methods associated with IntrusiveCounter are declared
  as const, and the counter itself is marked as mutable.

  IntrusiveCounter itself is abstract, allowing for multiple implementations.
  For example, IntrusiveCounterUnsigned uses ordinary unsigned integers for
  the reference count, and is good for situations where thread safety is not
  required.  For others, other implementations using atomic integers should
  be used.  For static objects, the implementations of addRef() and release()
  can be overridden to do nothing.
 */
class IntrusiveCounter {
    MONGO_DISALLOW_COPYING(IntrusiveCounter);

public:
    IntrusiveCounter() = default;
    virtual ~IntrusiveCounter(){};

    // these are here for the boost intrusive_ptr<> class
    friend inline void intrusive_ptr_add_ref(const IntrusiveCounter* pIC) {
        pIC->addRef();
    };
    friend inline void intrusive_ptr_release(const IntrusiveCounter* pIC) {
        pIC->release();
    };

    virtual void addRef() const = 0;
    virtual void release() const = 0;
};

class IntrusiveCounterUnsigned : public IntrusiveCounter {
public:
    // virtuals from IntrusiveCounter
    virtual void addRef() const;
    virtual void release() const;

    IntrusiveCounterUnsigned();

private:
    mutable unsigned counter;
};

/// This is an alternative base class to the above ones (will replace them eventually)
class RefCountable {
    MONGO_DISALLOW_COPYING(RefCountable);

public:
    /// If false you have exclusive access to this object. This is useful for implementing COW.
    bool isShared() const {
        // TODO: switch to unfenced read method after SERVER-6973
        return reinterpret_cast<unsigned&>(_count) > 1;
    }

    friend void intrusive_ptr_add_ref(const RefCountable* ptr) {
        ptr->_count.addAndFetch(1);
    };

    friend void intrusive_ptr_release(const RefCountable* ptr) {
        if (ptr->_count.subtractAndFetch(1) == 0) {
            delete ptr;  // uses subclass destructor and operator delete
        }
    };

protected:
    RefCountable() {}
    virtual ~RefCountable() {}

private:
    mutable AtomicUInt32 _count;  // default initialized to 0
};

/// This is an immutable reference-counted string
class RCString : public RefCountable {
public:
    const char* c_str() const {
        return reinterpret_cast<const char*>(this) + sizeof(RCString);
    }
    int size() const {
        return _size;
    }
    StringData stringData() const {
        return StringData(c_str(), _size);
    }

    static boost::intrusive_ptr<const RCString> create(StringData s);

// MSVC: C4291: 'declaration' : no matching operator delete found; memory will not be freed if
// initialization throws an exception
// We simply rely on the default global placement delete since a local placement delete would be
// ambiguous for some compilers
#pragma warning(push)
#pragma warning(disable : 4291)
    void operator delete(void* ptr) {
        free(ptr);
    }
#pragma warning(pop)

private:
    // these can only be created by calling create()
    RCString(){};
    void* operator new(size_t objSize, size_t realSize) {
        return mongoMalloc(realSize);
    }

    int _size;  // does NOT include trailing NUL byte.
    // char[_size+1] array allocated past end of class
};
};

/* ======================= INLINED IMPLEMENTATIONS ========================== */

namespace mongo {

inline IntrusiveCounterUnsigned::IntrusiveCounterUnsigned() : counter(0) {}
};
