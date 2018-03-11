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

#include "mongo/platform/basic.h"

#include <string>

#include "mongo/base/system_error.h"

namespace mongo {

namespace {

/**
 * A std::error_category for the codes in the named ErrorCodes space.
 */
class MongoErrorCategoryImpl final : public std::error_category {
public:
    MongoErrorCategoryImpl() = default;

    const char* name() const noexcept override {
        return "mongo";
    }

    std::string message(int ev) const override {
        return ErrorCodes::errorString(ErrorCodes::Error(ev));
    }

    // We don't really want to override this function, but to override the second we need to,
    // otherwise there will be issues with overload resolution.
    bool equivalent(const int code, const std::error_condition& condition) const noexcept override {
        return std::error_category::equivalent(code, condition);
    }

    bool equivalent(const std::error_code& code, int condition) const noexcept override {
        switch (ErrorCodes::Error(condition)) {
            case ErrorCodes::OK:
                // Make ErrorCodes::OK to be equivalent to the default constructed error code.
                return code == std::error_code();
            default:
                return false;
        }
    }
};

}  // namespace

const std::error_category& mongoErrorCategory() {
    // TODO: Remove this static, and make a constexpr instance when we move to C++14.
    static const MongoErrorCategoryImpl instance{};
    return instance;
}

std::error_code make_error_code(ErrorCodes::Error code) {
    return std::error_code(ErrorCodes::Error(code), mongoErrorCategory());
}

std::error_condition make_error_condition(ErrorCodes::Error code) {
    return std::error_condition(ErrorCodes::Error(code), mongoErrorCategory());
}

}  // namespace mongo
