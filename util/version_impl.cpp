/*    Copyright 2016 10gen Inc.
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

#define MONGO_LOG_DEFAULT_COMPONENT ::mongo::logger::LogComponent::kControl

#include "mongo/platform/basic.h"

#include "mongo/util/version.h"

#include "mongo/base/init.h"
#include "mongo/util/assert_util.h"
#include "mongo/util/log.h"

#define MONGO_UTIL_VERSION_CONSTANTS_H_WHITELISTED
#include "mongo/util/version_constants.h"

namespace mongo {
namespace {

const class : public VersionInfoInterface {
public:
    int majorVersion() const noexcept final {
        return version::kMajorVersion;
    }

    int minorVersion() const noexcept final {
        return version::kMinorVersion;
    }

    int patchVersion() const noexcept final {
        return version::kPatchVersion;
    }

    int extraVersion() const noexcept final {
        return version::kExtraVersion;
    }

    StringData version() const noexcept final {
        return version::kVersion;
    }

    StringData gitVersion() const noexcept final {
        return version::kGitVersion;
    }

    std::vector<StringData> modules() const final {
        return version::kModulesList;
    }

    StringData allocator() const noexcept final {
        return version::kAllocator;
    }

    StringData jsEngine() const noexcept final {
        return version::kJsEngine;
    }

    StringData targetMinOS() const noexcept final {
#if defined(_WIN32)
#if (NTDDI_VERSION >= NTDDI_WIN7)
        return "Windows 7/Windows Server 2008 R2";
#else
#error This targeted Windows version is not supported
#endif  // NTDDI_VERSION
#else
        severe() << "VersionInfoInterface::targetMinOS is only available for Windows";
        fassertFailed(40277);
#endif
    }

    std::vector<BuildInfoTuple> buildInfo() const final {
        return version::kBuildEnvironment;
    }

} kInterpolatedVersionInfo{};

MONGO_INITIALIZER_GENERAL(EnableVersionInfo,
                          MONGO_NO_PREREQUISITES,
                          ("BeginStartupOptionRegistration", "GlobalLogManager"))
(InitializerContext*) {
    VersionInfoInterface::enable(&kInterpolatedVersionInfo);
    return Status::OK();
}

}  // namespace
}  // namespace mongo
