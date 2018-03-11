/**
*    Copyright (C) 2014 MongoDB Inc.
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

#include "mongo/platform/compiler.h"

namespace mongo {

/** This function will call ::_exit and not return. Use this instead of calling ::_exit
 *  directly:
 *   - It offers a debugger hook to catch the process before leaving code under our control.
 *   - For some builds (leak sanitizer) it gives us an opportunity to dump leaks.
 *
 *  The function is named differently than quick_exit so that we can distinguish it from
 *  the C++11 function of the same name.
 */
MONGO_COMPILER_NORETURN void quickExit(int);

}  // namespace mongo
