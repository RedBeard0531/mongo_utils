/*    Copyright 2009 10gen Inc.
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

#include "mongo/base/string_data.h"

namespace mongo {

/**
 * Sets the name of the current thread.
 */
void setThreadName(StringData name);

/**
 * Retrieves the name of the current thread, as previously set, or "thread#" if no name was
 * previously set. The returned StringData is always null terminated so it is safe to pass to APIs
 * that expect c-strings.
 */
StringData getThreadName();

}  // namespace mongo
