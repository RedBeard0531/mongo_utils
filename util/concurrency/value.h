/* @file value.h
   concurrency helpers DiagStr
*/

/**
*    Copyright (C) 2008 10gen Inc.
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

#include "spin_lock.h"

namespace mongo {

// todo: rename this to ThreadSafeString or something
/** there is now one mutex per DiagStr.  If you have hundreds or millions of
    DiagStrs you'll need to do something different.
*/
class DiagStr {
    mutable SpinLock m;
    std::string _s;

public:
    DiagStr(const DiagStr& r) : _s(r.get()) {}
    DiagStr(const std::string& r) : _s(r) {}
    DiagStr() {}
    bool empty() const {
        scoped_spinlock lk(m);
        return _s.empty();
    }
    std::string get() const {
        scoped_spinlock lk(m);
        return _s;
    }
    void set(const char* s) {
        scoped_spinlock lk(m);
        _s = s;
    }
    void set(const std::string& s) {
        scoped_spinlock lk(m);
        _s = s;
    }
    operator std::string() const {
        return get();
    }
    void operator=(const std::string& s) {
        set(s);
    }
    void operator=(const DiagStr& rhs) {
        set(rhs.get());
    }

    // == is not defined.  use get() == ... instead.  done this way so one thinks about if composing
    // multiple operations
    bool operator==(const std::string& s) const;
};
}
