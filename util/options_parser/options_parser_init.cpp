/**
 *    Copyright (C) 2013 MongoDB Inc.
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

#include "mongo/util/options_parser/startup_options.h"

#include <iostream>

#include "mongo/util/exit_code.h"
#include "mongo/util/options_parser/option_description.h"
#include "mongo/util/options_parser/option_section.h"
#include "mongo/util/options_parser/options_parser.h"
#include "mongo/util/options_parser/startup_option_init.h"
#include "mongo/util/quick_exit.h"

namespace mongo {
namespace optionenvironment {

MONGO_STARTUP_OPTIONS_PARSE(StartupOptions)(InitializerContext* context) {
    OptionsParser parser;
    Status ret = parser.run(startupOptions, context->args(), context->env(), &startupOptionsParsed);
    if (!ret.isOK()) {
        std::cerr << ret.reason() << std::endl;
        // TODO: Figure out if there's a use case for this help message ever being different
        std::cerr << "try '" << context->args()[0] << " --help' for more information" << std::endl;
        quickExit(EXIT_BADOPTIONS);
    }
    return Status::OK();
}

}  // namespace optionenvironment
}  // namespace mongo
