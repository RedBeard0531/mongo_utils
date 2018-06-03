/*    Copyright 2012 10gen Inc.
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

#include "mongo/base/disallow_copying.h"
#include "mongo/base/initializer_function.h"
#include "mongo/base/status.h"

/**
* Convenience parameter representing the default set of dependents for initializer functions. Should
* just be used internally, for MONGO_INITIALIZER macros, use MONGO_DEFAULT_PREREQUISITES from init.h
* instead.
*/
#define MONGO_DEFAULT_PREREQUISITES_STR "default"

namespace mongo {

/**
 * Type representing the act of registering a process-global intialization function.
 *
 * Create a module-global instance of this type to register a new initializer, to be run by a
 * call to a variant of mongo::runGlobalInitializers().  See mongo/base/initializer.h,
 * mongo/base/init.h and mongo/base/initializer_dependency_graph.h for details.
 */
class GlobalInitializerRegisterer {
    MONGO_DISALLOW_COPYING(GlobalInitializerRegisterer);

public:
    /**
    * Defines an initializer function that depends on default prerequisites and has no explicit
    * dependents.
    *
    * Does not support deinitialization and will never be re-initialized.
    *
    * Usage:
    *    GlobalInitializerRegisterer myRegsisterer(
    *            "myInitializer",
    *            [](InitializerContext* context) {
    *                // initialization code
    *                return Status::OK();
    *            }
    *    );
    */
    GlobalInitializerRegisterer(std::string name, InitializerFunction initFn);

    /**
    * Defines an initializer function that depends on PREREQUISITES and has no explicit dependents.
    *
    * At run time, the full set of prerequisites for NAME will be computed as the union of the
    * explicit PREREQUISITES.
    *
    * Does not support deinitialization and will never be re-initialized.
    *
    * Usage:
    *    GlobalInitializerRegisterer myRegsisterer(
    *            "myInitializer",
    *            {"myPrereq1", "myPrereq2", ...},
    *            [](InitializerContext* context) {
    *                // initialization code
    *                return Status::OK();
    *            }
    *    );
    */
    GlobalInitializerRegisterer(std::string name,
                                std::vector<std::string> prerequisites,
                                InitializerFunction initFn);

    /**
    * Defines an initializer function that depends on PREREQUISITES and has DEPENDENTS as explicit
    * dependents.
    *
    * At run time, the full set of prerequisites for NAME will be computed as the union of the
    * explicit PREREQUISITES and the set of all other mongo initializers that name NAME in their
    * list of dependents.
    *
    * Does not support deinitialization and will never be re-initialized.
    *
    * Usage:
    *    GlobalInitializerRegisterer myRegsisterer(
    *            "myInitializer",
    *            {"myPrereq1", "myPrereq2", ...},
    *            {"myDependent1", "myDependent2", ...},
    *            [](InitializerContext* context) {
    *                // initialization code
    *                return Status::OK();
    *            }
    *    );
    */
    GlobalInitializerRegisterer(std::string name,
                                std::vector<std::string> prerequisites,
                                std::vector<std::string> dependents,
                                InitializerFunction initFn);

    /**
    * Defines an initializer function that depends on default prerequisites and has no explicit
    * dependents.
    * It also provides a deinitialization that will execute in reverse order from initialization and
    * support re-initialization.
    *
    * Usage:
    *    GlobalInitializerRegisterer myRegsisterer(
    *            "myInitializer",
    *            [](InitializerContext* context) {
    *                // initialization code
    *                return Status::OK();
    *            },
    *            [](DeinitializerContext* context) {
    *                // deinitialization code
    *                return Status::OK();
    *            }
    *    );
    */
    GlobalInitializerRegisterer(std::string name,
                                InitializerFunction initFn,
                                DeinitializerFunction deinitFn);

    /**
    * Defines an initializer function that depends on PREREQUISITES and has no explicit dependents.
    *
    * At run time, the full set of prerequisites for NAME will be computed as the union of the
    * explicit PREREQUISITES.
    *
    * It also provides a deinitialization that will execute in reverse order from initialization and
    * support re-initialization.
    *
    * Usage:
    *    GlobalInitializerRegisterer myRegsisterer(
    *            "myInitializer",
    *            {"myPrereq1", "myPrereq2", ...},
    *            [](InitializerContext* context) {
    *                // initialization code
    *                return Status::OK();
    *            },
    *            [](DeinitializerContext* context) {
    *                // deinitialization code
    *                return Status::OK();
    *            }
    *    );
    */
    GlobalInitializerRegisterer(std::string name,
                                std::vector<std::string> prerequisites,
                                InitializerFunction initFn,
                                DeinitializerFunction deinitFn);

    /**
    * Defines an initializer function that depends on PREREQUISITES and has DEPENDENTS as explicit
    * dependents.
    *
    * At run time, the full set of prerequisites for NAME will be computed as the union of the
    * explicit PREREQUISITES and the set of all other mongo initializers that name NAME in their
    * list of dependents.
    *
    * It also provides a deinitialization that will execute in reverse order from initialization and
    * support re-initialization.
    *
    * Usage:
    *    GlobalInitializerRegisterer myRegsisterer(
    *            "myInitializer",
    *            {"myPrereq1", "myPrereq2", ...},
    *            {"myDependent1", "myDependent2", ...},
    *            [](InitializerContext* context) {
    *                // initialization code
    *                return Status::OK();
    *            },
    *            [](DeinitializerContext* context) {
    *                // deinitialization code
    *                return Status::OK();
    *            }
    *    );
    */
    GlobalInitializerRegisterer(std::string name,
                                std::vector<std::string> prerequisites,
                                std::vector<std::string> dependents,
                                InitializerFunction initFn,
                                DeinitializerFunction deinitFn);
};

inline GlobalInitializerRegisterer::GlobalInitializerRegisterer(std::string name,
                                                                InitializerFunction initFn)
    : GlobalInitializerRegisterer(std::move(name),
                                  {MONGO_DEFAULT_PREREQUISITES_STR},
                                  {},
                                  std::move(initFn),
                                  DeinitializerFunction()) {}

inline GlobalInitializerRegisterer::GlobalInitializerRegisterer(
    std::string name, std::vector<std::string> prerequisites, InitializerFunction initFn)
    : GlobalInitializerRegisterer(std::move(name),
                                  std::move(prerequisites),
                                  {},
                                  std::move(initFn),
                                  DeinitializerFunction()) {}

inline GlobalInitializerRegisterer::GlobalInitializerRegisterer(
    std::string name,
    std::vector<std::string> prerequisites,
    std::vector<std::string> dependents,
    InitializerFunction initFn)
    : GlobalInitializerRegisterer(std::move(name),
                                  std::move(prerequisites),
                                  std::move(dependents),
                                  std::move(initFn),
                                  DeinitializerFunction()) {}

inline GlobalInitializerRegisterer::GlobalInitializerRegisterer(std::string name,
                                                                InitializerFunction initFn,
                                                                DeinitializerFunction deinitFn)
    : GlobalInitializerRegisterer(std::move(name),
                                  {MONGO_DEFAULT_PREREQUISITES_STR},
                                  {},
                                  std::move(initFn),
                                  std::move(deinitFn)) {}

inline GlobalInitializerRegisterer::GlobalInitializerRegisterer(
    std::string name,
    std::vector<std::string> prerequisites,
    InitializerFunction initFn,
    DeinitializerFunction deinitFn)
    : GlobalInitializerRegisterer(
          std::move(name), std::move(prerequisites), {}, std::move(initFn), std::move(deinitFn)) {}

}  // namespace mongo
