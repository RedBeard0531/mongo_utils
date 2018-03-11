/**   Copyright 2009 10gen Inc.
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

#include "mongo/logger/log_severity.h"
#include "mongo/logger/logger.h"
#include "mongo/logger/logstream_builder.h"
#include "mongo/util/assert_util.h"

#define DESTRUCTOR_GUARD MONGO_DESTRUCTOR_GUARD
#define MONGO_DESTRUCTOR_GUARD(expression)                                                     \
    try {                                                                                      \
        expression;                                                                            \
    } catch (const std::exception& e) {                                                        \
        ::mongo::logger::LogstreamBuilder(::mongo::logger::globalLogDomain(),                  \
                                          ::mongo::getThreadName(),                            \
                                          ::mongo::logger::LogSeverity::Log())                 \
            << "caught exception (" << e.what() << ") in destructor (" << __FUNCTION__ << ")"  \
            << std::endl;                                                                      \
    } catch (...) {                                                                            \
        ::mongo::logger::LogstreamBuilder(::mongo::logger::globalLogDomain(),                  \
                                          ::mongo::getThreadName(),                            \
                                          ::mongo::logger::LogSeverity::Log())                 \
            << "caught unknown exception in destructor (" << __FUNCTION__ << ")" << std::endl; \
    }
