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

#include "mongo/platform/basic.h"

#include "mongo/base/error_codes.h"

#include "mongo/base/static_assert.h"
#include "mongo/util/assert_util.h"
#include "mongo/util/mongoutils/str.h"

//#set $codes_with_extra = [ec for ec in $codes if ec.extra]

namespace mongo {

namespace {
// You can thing of this namespace as a compile-time map<ErrorCodes::Error, ErrorExtraInfoParser*>.
namespace parsers {
//#for $ec in $codes_with_extra
ErrorExtraInfo::Parser* $ec.name = nullptr;
//#end for
}  // namespace parsers
}  // namespace


MONGO_STATIC_ASSERT(sizeof(ErrorCodes::Error) == sizeof(int));

std::string ErrorCodes::errorString(Error err) {
    switch (err) {
        //#for $ec in $codes
        case $ec.name:
            return "$ec.name";
        //#end for
        default:
            return mongoutils::str::stream() << "Location" << int(err);
    }
}

ErrorCodes::Error ErrorCodes::fromString(StringData name) {
    //#for $ec in $codes
    if (name == "$ec.name"_sd)
        return $ec.name;
    //#end for
    return UnknownError;
}

std::ostream& operator<<(std::ostream& stream, ErrorCodes::Error code) {
    return stream << ErrorCodes::errorString(code);
}

//#for $cat in $categories
bool ErrorCodes::is${cat.name}(Error err) {
    switch (err) {
        //#for $code in $cat.codes
        case $code:
            return true;
        //#end for
        default:
            return false;
    }
}
//#end for

bool ErrorCodes::shouldHaveExtraInfo(Error code) {
    switch (code) {
        //#for $ec in $codes_with_extra
        case ErrorCodes::$ec.name:
            return true;
        //#end for
        default:
            return false;
    }
}

ErrorExtraInfo::Parser* ErrorExtraInfo::parserFor(ErrorCodes::Error code) {
    switch (code) {
        //#for $ec in $codes_with_extra
        case ErrorCodes::$ec.name:
            invariant(parsers::$ec.name);
            return parsers::$ec.name;
        //#end for
        default:
            return nullptr;
    }
}

void ErrorExtraInfo::registerParser(ErrorCodes::Error code, Parser* parser) {
    switch (code) {
        //#for $ec in $codes_with_extra
        case ErrorCodes::$ec.name:
            invariant(!parsers::$ec.name);
            parsers::$ec.name = parser;
            break;
        //#end for
        default:
            MONGO_UNREACHABLE;
    }
}

void ErrorExtraInfo::invariantHaveAllParsers() {
    //#for $ec in $codes_with_extra
    invariant(parsers::$ec.name);
    //#end for
}

void error_details::throwExceptionForStatus(const Status& status) {
    /**
     * This type is used for all exceptions that don't have a more specific type. It is defined
     * locally in this function to prevent anyone from catching it specifically separately from
     * AssertionException.
     */
    class NonspecificAssertionException final : public AssertionException {
    public:
        using AssertionException::AssertionException;

    private:
        void defineOnlyInFinalSubclassToPreventSlicing() final {}
    };

    switch (status.code()) {
        //#for $ec in $codes
        case ErrorCodes::$ec.name:
            throw ExceptionFor<ErrorCodes::$ec.name>(status);
        //#end for
        default:
            throw NonspecificAssertionException(status);
    }
}

}  // namespace mongo
