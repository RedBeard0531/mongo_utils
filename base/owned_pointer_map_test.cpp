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

/** Unit tests for OwnedPointerMap. */

#include "mongo/base/owned_pointer_map.h"

#include <string>
#include <vector>

#include "mongo/unittest/unittest.h"

namespace mongo {
namespace {

using std::make_pair;
using std::string;

/** Helper class that logs its constructor argument to a static vector on destruction. */
class DestructionLogger {
public:
    DestructionLogger(const string& name) : _name(name) {}
    ~DestructionLogger() {
        _destroyedNames.push_back(_name);
    }

    static std::vector<string>& destroyedNames() {
        return _destroyedNames;
    }

    string getName() {
        return _name;
    }

private:
    string _name;
    static std::vector<string> _destroyedNames;
};

std::vector<string> DestructionLogger::_destroyedNames;

TEST(OwnedPointerMapTest, OwnedPointerDestroyed) {
    DestructionLogger::destroyedNames().clear();
    {
        OwnedPointerMap<int, DestructionLogger> owned;
        owned.mutableMap().insert(make_pair(0, new DestructionLogger("foo")));
        // owned destroyed
    }
    ASSERT_EQUALS(1U, DestructionLogger::destroyedNames().size());
    ASSERT_EQUALS("foo", DestructionLogger::destroyedNames()[0]);
}

TEST(OwnedPointerMapTest, OwnedConstPointerDestroyed) {
    DestructionLogger::destroyedNames().clear();
    {
        OwnedPointerMap<int, const DestructionLogger> owned;
        owned.mutableMap().insert(make_pair(0, new DestructionLogger("foo")));
        // owned destroyed
    }
    ASSERT_EQUALS(1U, DestructionLogger::destroyedNames().size());
    ASSERT_EQUALS("foo", DestructionLogger::destroyedNames()[0]);
}

TEST(OwnedPointerMapTest, OwnedPointersDestroyedInOrder) {
    DestructionLogger::destroyedNames().clear();
    {
        OwnedPointerMap<int, DestructionLogger> owned;
        owned.mutableMap().insert(make_pair(0, new DestructionLogger("first")));
        owned.mutableMap().insert(make_pair(1, new DestructionLogger("second")));
        // owned destroyed
    }
    ASSERT_EQUALS(2U, DestructionLogger::destroyedNames().size());
    ASSERT_EQUALS("first", DestructionLogger::destroyedNames()[0]);
    ASSERT_EQUALS("second", DestructionLogger::destroyedNames()[1]);
}

TEST(OwnedPointerMapTest, OwnedPointersWithCompare) {
    DestructionLogger::destroyedNames().clear();
    {
        OwnedPointerMap<int, DestructionLogger, std::greater<int>> owned;
        owned.mutableMap().insert(make_pair(0, new DestructionLogger("0")));
        owned.mutableMap().insert(make_pair(1, new DestructionLogger("1")));

        // use std::greater<int> rather than the default std::less<int>
        std::map<int, DestructionLogger*, std::greater<int>>::iterator it =
            owned.mutableMap().begin();

        ASSERT(owned.mutableMap().end() != it);
        // "1" should be sorted to be the first item.
        ASSERT_EQUALS("1", it->second->getName());

        it++;
        ASSERT(owned.mutableMap().end() != it);
        ASSERT_EQUALS("0", it->second->getName());

        // owned destroyed
    }
    // destroyed in descending order
    ASSERT_EQUALS(2U, DestructionLogger::destroyedNames().size());
    ASSERT_EQUALS("1", DestructionLogger::destroyedNames()[0]);
    ASSERT_EQUALS("0", DestructionLogger::destroyedNames()[1]);
}


}  // namespace
}  // namespace mongo
