/*
 *    Copyright (C) 2018 MongoDB Inc.
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

#include "mongo/util/icu.h"

#include <memory>
#include <unicode/localpointer.h>
#include <unicode/putil.h>
#include <unicode/uiter.h>
#include <unicode/unistr.h>
#include <unicode/usprep.h>
#include <unicode/ustring.h>
#include <unicode/utypes.h>
#include <vector>

#include "mongo/util/assert_util.h"
#include "mongo/util/mongoutils/str.h"

namespace mongo {
namespace {

/**
 * Convenience wrapper for a UChar[] string.
 * Instantiate with UString::fromUTF8() and reseriealize with ustr.toUTF8()
 */
class UString {
public:
    UString() = delete;
    explicit UString(size_t size) {
        _str.resize(size);
    }

    const UChar* uc_str() const {
        return _str.data();
    }
    UChar* data() {
        return _str.data();
    }
    size_t capacity() const {
        return _str.capacity();
    }
    size_t size() const {
        return _str.size();
    }
    void resize(size_t len) {
        _str.resize(len);
    }

    static UString fromUTF8(StringData str) {
        UErrorCode error = U_ZERO_ERROR;
        int32_t len = 0;
        u_strFromUTF8(nullptr, 0, &len, str.rawData(), str.size(), &error);
        uassert(ErrorCodes::BadValue, "Non UTF-8 data encountered", error != U_INVALID_CHAR_FOUND);
        uassert(50687,
                str::stream() << "Error preflighting UTF-8 conversion: " << u_errorName(error),
                error == U_BUFFER_OVERFLOW_ERROR);

        error = U_ZERO_ERROR;
        UString ret(len);
        u_strFromUTF8(ret.data(), ret.capacity(), &len, str.rawData(), str.size(), &error);
        uassert(50688,
                str::stream() << "Error converting UTF-8 string: " << u_errorName(error),
                U_SUCCESS(error));
        ret.resize(len);
        return ret;
    }

    std::string toUTF8() const {
        UErrorCode error = U_ZERO_ERROR;
        int32_t len = 0;
        u_strToUTF8(nullptr, 0, &len, _str.data(), _str.size(), &error);
        uassert(50689,
                str::stream() << "Error preflighting UTF-8 conversion: " << u_errorName(error),
                error == U_BUFFER_OVERFLOW_ERROR);

        error = U_ZERO_ERROR;
        std::string ret;
        ret.resize(len);
        u_strToUTF8(&ret[0], ret.capacity(), &len, _str.data(), _str.size(), &error);
        uassert(50690,
                str::stream() << "Error converting string to UTF-8: " << u_errorName(error),
                U_SUCCESS(error));
        ret.resize(len);
        return ret;
    }

private:
    std::vector<UChar> _str;
};

/**
 * Convenience wrapper for ICU unicode string prep API.
 */
class USPrep {
public:
    USPrep() = delete;
    USPrep(UStringPrepProfileType type) {
        UErrorCode error = U_ZERO_ERROR;
        _profile.reset(usprep_openByType(type, &error));
        uassert(50691,
                str::stream() << "Unable to open unicode string prep profile: "
                              << u_errorName(error),
                U_SUCCESS(error));
    }

    UString prepare(const UString& src, int32_t options = USPREP_DEFAULT) {
        UErrorCode error = U_ZERO_ERROR;
        auto len = usprep_prepare(
            _profile.get(), src.uc_str(), src.size(), nullptr, 0, options, nullptr, &error);
        uassert(ErrorCodes::BadValue,
                "Unable to normalize input string",
                error != U_INVALID_CHAR_FOUND);
        uassert(50692,
                str::stream() << "Error preflighting normalization: " << u_errorName(error),
                error == U_BUFFER_OVERFLOW_ERROR);

        error = U_ZERO_ERROR;
        UString ret(len);
        len = usprep_prepare(_profile.get(),
                             src.uc_str(),
                             src.size(),
                             ret.data(),
                             ret.capacity(),
                             options,
                             nullptr,
                             &error);
        uassert(50693,
                str::stream() << "Failed normalizing string: " << u_errorName(error),
                U_SUCCESS(error));
        ret.resize(len);
        return ret;
    }

private:
    class USPrepDeleter {
    public:
        void operator()(UStringPrepProfile* profile) {
            usprep_close(profile);
        }
    };

    std::unique_ptr<UStringPrepProfile, USPrepDeleter> _profile;
};

}  // namespace
}  // namespace mongo

mongo::StatusWith<std::string> mongo::saslPrep(StringData str, UStringPrepOptions options) try {
    const auto opts = (options == kUStringPrepDefault) ? USPREP_DEFAULT : USPREP_ALLOW_UNASSIGNED;
    return USPrep(USPREP_RFC4013_SASLPREP).prepare(UString::fromUTF8(str), opts).toUTF8();
} catch (const DBException& e) {
    return e.toStatus();
}
