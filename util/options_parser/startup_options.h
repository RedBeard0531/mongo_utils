/*
 *    Copyright (C) 2013 10gen Inc.
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

#include "mongo/util/options_parser/environment.h"
#include "mongo/util/options_parser/option_section.h"

namespace mongo {
namespace optionenvironment {

/*
 * This structure stores information about all the command line options.  The parser will use
 * this description when it parses the command line, the INI config file, and the JSON config
 * file.  See the OptionSection and OptionDescription classes for more details.
 *
 * Example:
 * MONGO_MODULE_STARTUP_OPTIONS_REGISTER(MongodOptions)(InitializerContext* context) {
 *          return addMongodOptions(&moe::startupOptions);
 *     startupOptions.addOptionChaining("option", "option", moe::String, "description");
 *     return Status::OK();
 * }
 */
extern OptionSection startupOptions;

/*
 * This structure stores the parsed command line options.  After the "defult" group of the
 * MONGO_INITIALIZERS, this structure should be fully validated from an option perspective.  See
 * the Environment, Constraint, and Value classes for more details.
 *
 * Example:
 * if (startupOptionsParsed.count("option")) {
 *     std::string value;
 *     ret = startupOptionsParsed.get("option", &value);
 *     if (!ret.isOK()) {
 *         return ret;
 *     }
 * }
 */
extern Environment startupOptionsParsed;

}  // namespace optionenvironment
}  // namespace mongo
