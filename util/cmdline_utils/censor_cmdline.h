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

#include <string>
#include <vector>

#include "mongo/db/jsobj.h"

namespace mongo {

namespace cmdline_utils {

/**
 * Blot out sensitive fields in the argv array.
 */
void censorArgvArray(int argc, char** argv);
void censorArgsVector(std::vector<std::string>* args);
void censorBSONObj(BSONObj* params);

}  // namespace cmdline_utils
}
