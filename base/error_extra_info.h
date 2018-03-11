/**
 *    Copyright 2017 MongoDB, Inc.
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

#include <memory>

// This file is included by many low-level headers including status.h, so it isn't able to include
// much without creating a cycle.
#include "mongo/base/error_codes.h"
#include "mongo/base/static_assert.h"

namespace mongo {

class BSONObj;
class BSONObjBuilder;

/**
 * Base class for the extra info that can be attached to commands.
 *
 * Actual implementations must have a 'constexpr ErrorCode::Error code' to indicate which
 * error code they bind to, and a static method with the following signature:
 *      static std::shared_ptr<const ErrorExtraInfo> parse(const BSONObj&);
 * You must call the MONGO_INIT_REGISTER_ERROR_EXTRA_INFO(type) macro in the cpp file that contains
 * the implementation for your subtype.
 */
class ErrorExtraInfo {
public:
    using Parser = std::shared_ptr<const ErrorExtraInfo>(const BSONObj&);

    ErrorExtraInfo() = default;
    virtual ~ErrorExtraInfo() = default;

    /**
     * Puts the extra info (and just the extra info) into builder.
     */
    virtual void serialize(BSONObjBuilder* builder) const = 0;

    /**
     * Returns the registered parser for a given code or nullptr if none is registered.
     */
    static Parser* parserFor(ErrorCodes::Error);

    /**
     * Use the MONGO_INIT_REGISTER_ERROR_EXTRA_INFO(type) macro below rather than calling this
     * directly.
     */
    template <typename T>
    static void registerType() {
        MONGO_STATIC_ASSERT(std::is_base_of<ErrorExtraInfo, T>());
        MONGO_STATIC_ASSERT(std::is_same<error_details::ErrorExtraInfoFor<T::code>, T>());
        MONGO_STATIC_ASSERT(std::is_final<T>());
        MONGO_STATIC_ASSERT(std::is_move_constructible<T>());
        registerParser(T::code, T::parse);
    }

    /**
     * Fails fatally if any error codes that should have parsers registered don't. Call this during
     * startup of any shipping executable to prevent failures at runtime.
     */
    static void invariantHaveAllParsers();

private:
    static void registerParser(ErrorCodes::Error code, Parser* parser);
};

/**
 * Registers the parser for an ErrorExtraInfo subclass. This must be called at namespace scope in
 * the same cpp file as the virtual methods for that type.
 *
 * You must separately #include "mongo/base/init.h" since including it here would create an include
 * cycle.
 */
#define MONGO_INIT_REGISTER_ERROR_EXTRA_INFO(type)                            \
    MONGO_INITIALIZER_GENERAL(                                                \
        RegisterErrorExtraInfoFor##type, MONGO_NO_PREREQUISITES, ("default")) \
    (InitializerContext * context) {                                          \
        ErrorExtraInfo::registerType<type>();                                 \
        return Status::OK();                                                  \
    }

/**
 * This is an example ErrorExtraInfo subclass. It is used for testing the ErrorExtraInfoHandling.
 *
 * The default parser just throws a numeric code since this class should never be encountered in
 * production. A normal parser is activated while an EnableParserForTesting object is in scope.
 */
class ErrorExtraInfoExample final : public ErrorExtraInfo {
public:
    static constexpr auto code = ErrorCodes::ForTestingErrorExtraInfo;

    void serialize(BSONObjBuilder*) const override;
    static std::shared_ptr<const ErrorExtraInfo> parse(const BSONObj&);

    // Everything else in this class is just for testing and shouldn't by copied by users.

    struct EnableParserForTest {
        EnableParserForTest() {
            isParserEnabledForTest = true;
        }
        ~EnableParserForTest() {
            isParserEnabledForTest = false;
        }
    };

    ErrorExtraInfoExample(int data) : data(data) {}
    int data;  // This uses the fieldname "data".
private:
    static bool isParserEnabledForTest;
};
}  // namespace mongo
