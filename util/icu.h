/*
 *    Copyright (C) 2018 MongoDB Inc.
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

#include <string>

#include "mongo/base/status_with.h"
#include "mongo/base/string_data.h"

namespace mongo {

/**
 * Unicode string prepare options.
 * By default, unassigned codepoints in the input string will result in an error.
 * Using the AllowUnassigned option will pass them through without change,
 * which may not turn out to be appropriate in later Unicode standards.
 */
enum UStringPrepOptions {
    kUStringPrepDefault = 0,
    kUStringPrepAllowUnassigned = 1,
};

/**
 * Attempt to apply RFC4013 saslPrep to the target string.
 * Normalizes unicode sequences for SCRAM authentication.
 */
StatusWith<std::string> saslPrep(StringData str, UStringPrepOptions = kUStringPrepDefault);

}  // namespace mongo
