/**
 *    Copyright (C) 2015 MongoDB Inc.
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

#include <system_error>

#include "mongo/base/system_error.h"
#include "mongo/unittest/unittest.h"

namespace mongo {
namespace {

TEST(SystemError, Category) {
    ASSERT(make_error_code(ErrorCodes::AuthenticationFailed).category() == mongoErrorCategory());
    ASSERT(std::error_code(ErrorCodes::AlreadyInitialized, mongoErrorCategory()).category() ==
           mongoErrorCategory());
    ASSERT(make_error_condition(ErrorCodes::AuthenticationFailed).category() ==
           mongoErrorCategory());
    ASSERT(std::error_condition(ErrorCodes::AuthenticationFailed).category() ==
           mongoErrorCategory());
}

TEST(SystemError, Conversions) {
    ASSERT(make_error_code(ErrorCodes::AlreadyInitialized) == ErrorCodes::AlreadyInitialized);
    ASSERT(std::error_code(ErrorCodes::AlreadyInitialized, mongoErrorCategory()) ==
           ErrorCodes::AlreadyInitialized);
    ASSERT(make_error_condition(ErrorCodes::AlreadyInitialized) == ErrorCodes::AlreadyInitialized);
    ASSERT(std::error_condition(ErrorCodes::AlreadyInitialized) == ErrorCodes::AlreadyInitialized);
}

TEST(SystemError, Equivalence) {
    ASSERT(ErrorCodes::OK == std::error_code());
    ASSERT(std::error_code() == ErrorCodes::OK);
}

}  // namespace
}  // namespace mongo
