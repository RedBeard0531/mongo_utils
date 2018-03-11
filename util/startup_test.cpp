// mongo/util/startup_test.cpp

/**
*    Copyright (C) 2012 10gen Inc.
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

#include "mongo/util/startup_test.h"

namespace mongo {
std::vector<StartupTest*>* StartupTest::tests = 0;
bool StartupTest::running = false;

StartupTest::StartupTest() {
    registerTest(this);
}

StartupTest::~StartupTest() {}

void StartupTest::registerTest(StartupTest* t) {
    if (tests == 0)
        tests = new std::vector<StartupTest*>();
    tests->push_back(t);
}

void StartupTest::runTests() {
    running = true;
    for (std::vector<StartupTest*>::const_iterator i = tests->begin(); i != tests->end(); i++) {
        (*i)->run();
    }
    running = false;
}

}  // namespace mongo
