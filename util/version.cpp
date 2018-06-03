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

#include "mongo/config.h"

#ifdef MONGO_CONFIG_SSL
#if MONGO_CONFIG_SSL_PROVIDER == SSL_PROVIDER_OPENSSL
#include <openssl/crypto.h>
#endif
#endif

#include <pcrecpp.h>

#include <sstream>

#include "mongo/db/jsobj.h"
#include "mongo/util/assert_util.h"
#include "mongo/util/log.h"

namespace mongo {
namespace {

const class : public VersionInfoInterface {
public:
    int majorVersion() const noexcept final {
        return 0;
    }

    int minorVersion() const noexcept final {
        return 0;
    }

    int patchVersion() const noexcept final {
        return 0;
    }

    int extraVersion() const noexcept final {
        return 0;
    }

    StringData version() const noexcept final {
        return "unknown";
    }

    StringData gitVersion() const noexcept final {
        return "none";
    }

    std::vector<StringData> modules() const final {
        return {"unknown"};
    }

    StringData allocator() const noexcept final {
        return "unknown";
    }

    StringData jsEngine() const noexcept final {
        return "unknown";
    }

    StringData targetMinOS() const noexcept final {
        return "unknown";
    }

    std::vector<BuildInfoTuple> buildInfo() const final {
        return {};
    }

} kFallbackVersionInfo{};

const VersionInfoInterface* globalVersionInfo = nullptr;

}  // namespace

void VersionInfoInterface::enable(const VersionInfoInterface* handler) {
    globalVersionInfo = handler;
}

const VersionInfoInterface& VersionInfoInterface::instance(NotEnabledAction action) noexcept {
    if (globalVersionInfo)
        return *globalVersionInfo;
    if (action == NotEnabledAction::kFallback)
        return kFallbackVersionInfo;
    severe() << "Terminating because valid version info has not been configured";
    fassertFailed(40278);
}

bool VersionInfoInterface::isSameMajorVersion(const char* otherVersion) const noexcept {
    int major = -1, minor = -1;
    pcrecpp::RE ver_regex("^(\\d+)\\.(\\d+)\\.");
    ver_regex.PartialMatch(otherVersion, &major, &minor);

    if (major == -1 || minor == -1)
        return false;

    return (major == majorVersion() && minor == minorVersion());
}

std::string VersionInfoInterface::makeVersionString(StringData binaryName) const {
    std::stringstream ss;
    ss << binaryName << " v" << version();
    return ss.str();
}

void VersionInfoInterface::appendBuildInfo(BSONObjBuilder* result) const {
    *result << "version" << version() << "gitVersion" << gitVersion()
#if defined(_WIN32)
            << "targetMinOS" << targetMinOS()
#endif
            << "modules" << modules() << "allocator" << allocator() << "javascriptEngine"
            << jsEngine() << "sysInfo"
            << "deprecated";

    BSONArrayBuilder versionArray(result->subarrayStart("versionArray"));
    versionArray << majorVersion() << minorVersion() << patchVersion() << extraVersion();
    versionArray.done();

    BSONObjBuilder opensslInfo(result->subobjStart("openssl"));
#ifdef MONGO_CONFIG_SSL
#if MONGO_CONFIG_SSL_PROVIDER == SSL_PROVIDER_OPENSSL
    opensslInfo << "running" << openSSLVersion() << "compiled" << OPENSSL_VERSION_TEXT;
#elif MONGO_CONFIG_SSL_PROVIDER == SSL_PROVIDER_WINDOWS
    opensslInfo << "running"
                << "Windows SChannel";
#elif MONGO_CONFIG_SSL_PROVIDER == SSL_PROVIDER_APPLE
    opensslInfo << "running"
                << "Apple Secure Transport";
#else
#error "Unknown SSL Provider"
#endif  // MONGO_CONFIG_SSL_PROVIDER
#else
    opensslInfo << "running"
                << "disabled"
                << "compiled"
                << "disabled";
#endif
    opensslInfo.done();

    BSONObjBuilder buildvarsInfo(result->subobjStart("buildEnvironment"));
    for (auto&& envDataEntry : buildInfo()) {
        if (std::get<2>(envDataEntry)) {
            buildvarsInfo << std::get<0>(envDataEntry) << std::get<1>(envDataEntry);
        }
    }
    buildvarsInfo.done();

    *result << "bits" << (int)sizeof(void*) * 8;
    result->appendBool("debug", kDebugBuild);
    result->appendNumber("maxBsonObjectSize", BSONObjMaxUserSize);
}

std::string VersionInfoInterface::openSSLVersion(StringData prefix, StringData suffix) const {
#if !defined(MONGO_CONFIG_SSL) || MONGO_CONFIG_SSL_PROVIDER != SSL_PROVIDER_OPENSSL
    return "";
#elif MONGO_CONFIG_SSL_PROVIDER == SSL_PROVIDER_OPENSSL
    return prefix.toString() + SSLeay_version(SSLEAY_VERSION) + suffix;
#endif
}

void VersionInfoInterface::logTargetMinOS() const {
    log() << "targetMinOS: " << targetMinOS();
}

void VersionInfoInterface::logBuildInfo() const {
    log() << "git version: " << gitVersion();

#if defined(MONGO_CONFIG_SSL) && MONGO_CONFIG_SSL_PROVIDER == SSL_PROVIDER_OPENSSL
    log() << openSSLVersion("OpenSSL version: ");
#endif

    log() << "allocator: " << allocator();

    std::stringstream ss;
    ss << "modules: ";
    auto modules_list = modules();
    if (modules_list.size() == 0) {
        ss << "none";
    } else {
        for (const auto& m : modules_list) {
            ss << m << " ";
        }
    }
    log() << ss.str();

    log() << "build environment:";
    for (auto&& envDataEntry : buildInfo()) {
        if (std::get<3>(envDataEntry)) {
            auto val = std::get<1>(envDataEntry);
            if (val.size() == 0)
                continue;
            log() << "    " << std::get<0>(envDataEntry) << ": " << std::get<1>(envDataEntry);
        }
    }
}

std::string mongoShellVersion(const VersionInfoInterface& provider) {
    std::stringstream ss;
    ss << "MongoDB shell version v" << provider.version();
    return ss.str();
}

std::string mongosVersion(const VersionInfoInterface& provider) {
    std::stringstream ss;
    ss << "mongos version v" << provider.version();
    return ss.str();
}

std::string mongodVersion(const VersionInfoInterface& provider) {
    std::stringstream ss;
    ss << "db version v" << provider.version();
    return ss.str();
}

}  // namespace mongo
