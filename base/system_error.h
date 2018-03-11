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

#pragma once

#include <system_error>
#include <type_traits>

#include "mongo/base/error_codes.h"

namespace mongo {

const std::error_category& mongoErrorCategory();

// The next two functions are explicitly named (contrary to our naming style) so that they can be
// picked up by ADL.
std::error_code make_error_code(ErrorCodes::Error code);

std::error_condition make_error_condition(ErrorCodes::Error code);

}  // namespace mongo

namespace std {

/**
 * Allows a std::error_condition to be implicitly constructed from a mongo::ErrorCodes::Error.
 * We specialize this instead of is_error_code_enum as our ErrorCodes are platform independent.
 */
template <>
struct is_error_condition_enum<mongo::ErrorCodes::Error> : public true_type {};

}  // namespace std
