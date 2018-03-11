/* Copyright 2013 10gen Inc.
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

#include "mongo/util/net/ssl_options.h"

#include <boost/filesystem/operations.hpp>

#include "mongo/base/status.h"
#include "mongo/config.h"
#include "mongo/db/server_options.h"
#include "mongo/util/hex.h"
#include "mongo/util/log.h"
#include "mongo/util/options_parser/startup_options.h"
#include "mongo/util/text.h"

namespace mongo {

namespace moe = mongo::optionenvironment;
using std::string;

namespace {
StatusWith<std::vector<uint8_t>> hexToVector(StringData hex) {
    if (std::any_of(hex.begin(), hex.end(), [](char c) { return !isxdigit(c); })) {
        return {ErrorCodes::BadValue, "Not a valid hex string"};
    }
    if (hex.size() % 2) {
        return {ErrorCodes::BadValue, "Not an even number of hexits"};
    }

    std::vector<uint8_t> ret;
    ret.resize(hex.size() >> 1);
    int idx = -2;
    std::generate(ret.begin(), ret.end(), [&hex, &idx] {
        idx += 2;
        return (fromHex(hex[idx]) << 4) | fromHex(hex[idx + 1]);
    });
    return ret;
}
}  // nameapace

Status parseCertificateSelector(SSLParams::CertificateSelector* selector,
                                StringData name,
                                StringData value) {
    selector->subject.clear();
    selector->thumbprint.clear();
    selector->serial.clear();

    const auto delim = value.find('=');
    if (delim == std::string::npos) {
        return {ErrorCodes::BadValue,
                str::stream() << "Certificate selector for '" << name
                              << "' must be a key=value pair"};
    }

    auto key = value.substr(0, delim);
    if (key == "subject") {
        selector->subject = value.substr(delim + 1).toString();
        return Status::OK();
    }

    if ((key != "thumbprint") && (key != "serial")) {
        return {ErrorCodes::BadValue,
                str::stream() << "Unknown certificate selector property for '" << name << "': '"
                              << key
                              << "'"};
    }

    auto swHex = hexToVector(value.substr(delim + 1));
    if (!swHex.isOK()) {
        return {ErrorCodes::BadValue,
                str::stream() << "Invalid certificate selector value for '" << name << "': "
                              << swHex.getStatus().reason()};
    }

    if (key == "thumbprint") {
        selector->thumbprint = std::move(swHex.getValue());
    } else {
        invariant(key == "serial");
        selector->serial = std::move(swHex.getValue());
    }

    return Status::OK();
}

Status addSSLServerOptions(moe::OptionSection* options) {
    options
        ->addOptionChaining("net.ssl.sslOnNormalPorts",
                            "sslOnNormalPorts",
                            moe::Switch,
                            "use ssl on configured ports")
        .setSources(moe::SourceAllLegacy)
        .incompatibleWith("net.ssl.mode");

    options->addOptionChaining(
        "net.ssl.mode",
        "sslMode",
        moe::String,
        "set the SSL operation mode (disabled|allowSSL|preferSSL|requireSSL)");

    options->addOptionChaining(
        "net.ssl.PEMKeyFile", "sslPEMKeyFile", moe::String, "PEM file for ssl");

    options
        ->addOptionChaining(
            "net.ssl.PEMKeyPassword", "sslPEMKeyPassword", moe::String, "PEM file password")
        .setImplicit(moe::Value(std::string("")));

    options->addOptionChaining("net.ssl.clusterFile",
                               "sslClusterFile",
                               moe::String,
                               "Key file for internal SSL authentication");

    options
        ->addOptionChaining("net.ssl.clusterPassword",
                            "sslClusterPassword",
                            moe::String,
                            "Internal authentication key file password")
        .setImplicit(moe::Value(std::string("")));

    options->addOptionChaining(
        "net.ssl.CAFile", "sslCAFile", moe::String, "Certificate Authority file for SSL");

    options->addOptionChaining(
        "net.ssl.CRLFile", "sslCRLFile", moe::String, "Certificate Revocation List file for SSL");

    options
        ->addOptionChaining("net.ssl.sslCipherConfig",
                            "sslCipherConfig",
                            moe::String,
                            "OpenSSL cipher configuration string")
        .hidden();

    options->addOptionChaining(
        "net.ssl.disabledProtocols",
        "sslDisabledProtocols",
        moe::String,
        "Comma separated list of TLS protocols to disable [TLS1_0,TLS1_1,TLS1_2]");

    options->addOptionChaining("net.ssl.weakCertificateValidation",
                               "sslWeakCertificateValidation",
                               moe::Switch,
                               "allow client to connect without "
                               "presenting a certificate");

    // Alias for --sslWeakCertificateValidation.
    options->addOptionChaining("net.ssl.allowConnectionsWithoutCertificates",
                               "sslAllowConnectionsWithoutCertificates",
                               moe::Switch,
                               "allow client to connect without presenting a certificate");

    options->addOptionChaining("net.ssl.allowInvalidHostnames",
                               "sslAllowInvalidHostnames",
                               moe::Switch,
                               "Allow server certificates to provide non-matching hostnames");

    options->addOptionChaining("net.ssl.allowInvalidCertificates",
                               "sslAllowInvalidCertificates",
                               moe::Switch,
                               "allow connections to servers with invalid certificates");

    options->addOptionChaining(
        "net.ssl.FIPSMode", "sslFIPSMode", moe::Switch, "activate FIPS 140-2 mode at startup");

#ifdef MONGO_CONFIG_SSL_CERTIFICATE_SELECTORS
    options
        ->addOptionChaining("net.ssl.certificateSelector",
                            "sslCertificateSelector",
                            moe::String,
                            "SSL Certificate in system store")
        .incompatibleWith("net.ssl.PEMKeyFile")
        .incompatibleWith("net.ssl.PEMKeyPassword");
    options
        ->addOptionChaining("net.ssl.clusterCertificateSelector",
                            "sslClusterCertificateSelector",
                            moe::String,
                            "SSL Certificate in system store for internal SSL authentication")
        .incompatibleWith("net.ssl.clusterFile")
        .incompatibleWith("net.ssl.clusterFilePassword");
#endif

    return Status::OK();
}

Status addSSLClientOptions(moe::OptionSection* options) {
    options->addOptionChaining("ssl", "ssl", moe::Switch, "use SSL for all connections");

    options
        ->addOptionChaining(
            "ssl.CAFile", "sslCAFile", moe::String, "Certificate Authority file for SSL")
        .requires("ssl");

    options
        ->addOptionChaining(
            "ssl.PEMKeyFile", "sslPEMKeyFile", moe::String, "PEM certificate/key file for SSL")
        .requires("ssl");

    options
        ->addOptionChaining("ssl.PEMKeyPassword",
                            "sslPEMKeyPassword",
                            moe::String,
                            "password for key in PEM file for SSL")
        .requires("ssl");

    options
        ->addOptionChaining(
            "ssl.CRLFile", "sslCRLFile", moe::String, "Certificate Revocation List file for SSL")
        .requires("ssl")
        .requires("ssl.CAFile");

    options
        ->addOptionChaining("net.ssl.allowInvalidHostnames",
                            "sslAllowInvalidHostnames",
                            moe::Switch,
                            "allow connections to servers with non-matching hostnames")
        .requires("ssl");

    options
        ->addOptionChaining("ssl.allowInvalidCertificates",
                            "sslAllowInvalidCertificates",
                            moe::Switch,
                            "allow connections to servers with invalid certificates")
        .requires("ssl");

    options->addOptionChaining(
        "ssl.FIPSMode", "sslFIPSMode", moe::Switch, "activate FIPS 140-2 mode at startup");

#ifdef MONGO_CONFIG_SSL_CERTIFICATE_SELECTORS
    options
        ->addOptionChaining("ssl.certificateSelector",
                            "sslCertificateSelector",
                            moe::String,
                            "SSL Certificate in system store")
        .incompatibleWith("ssl.PEMKeyFile")
        .incompatibleWith("ssl.PEMKeyPassword");
#endif

    return Status::OK();
}

Status validateSSLServerOptions(const moe::Environment& params) {
#ifdef _WIN32
    if (params.count("install") || params.count("reinstall")) {
        if (params.count("net.ssl.PEMKeyFile") &&
            !boost::filesystem::path(params["net.ssl.PEMKeyFile"].as<string>()).is_absolute()) {
            return Status(ErrorCodes::BadValue,
                          "PEMKeyFile requires an absolute file path with Windows services");
        }

        if (params.count("net.ssl.clusterFile") &&
            !boost::filesystem::path(params["net.ssl.clusterFile"].as<string>()).is_absolute()) {
            return Status(ErrorCodes::BadValue,
                          "clusterFile requires an absolute file path with Windows services");
        }

        if (params.count("net.ssl.CAFile") &&
            !boost::filesystem::path(params["net.ssl.CAFile"].as<string>()).is_absolute()) {
            return Status(ErrorCodes::BadValue,
                          "CAFile requires an absolute file path with Windows services");
        }

        if (params.count("net.ssl.CRLFile") &&
            !boost::filesystem::path(params["net.ssl.CRLFile"].as<string>()).is_absolute()) {
            return Status(ErrorCodes::BadValue,
                          "CRLFile requires an absolute file path with Windows services");
        }
    }
#endif

    return Status::OK();
}

Status canonicalizeSSLServerOptions(moe::Environment* params) {
    if (params->count("net.ssl.sslOnNormalPorts") &&
        (*params)["net.ssl.sslOnNormalPorts"].as<bool>() == true) {
        Status ret = params->set("net.ssl.mode", moe::Value(std::string("requireSSL")));
        if (!ret.isOK()) {
            return ret;
        }
        ret = params->remove("net.ssl.sslOnNormalPorts");
        if (!ret.isOK()) {
            return ret;
        }
    }

    return Status::OK();
}

Status storeSSLServerOptions(const moe::Environment& params) {
    if (params.count("net.ssl.mode")) {
        std::string sslModeParam = params["net.ssl.mode"].as<string>();
        if (sslModeParam == "disabled") {
            sslGlobalParams.sslMode.store(SSLParams::SSLMode_disabled);
        } else if (sslModeParam == "allowSSL") {
            sslGlobalParams.sslMode.store(SSLParams::SSLMode_allowSSL);
        } else if (sslModeParam == "preferSSL") {
            sslGlobalParams.sslMode.store(SSLParams::SSLMode_preferSSL);
        } else if (sslModeParam == "requireSSL") {
            sslGlobalParams.sslMode.store(SSLParams::SSLMode_requireSSL);
        } else {
            return Status(ErrorCodes::BadValue, "unsupported value for sslMode " + sslModeParam);
        }
    }

    if (params.count("net.ssl.PEMKeyFile")) {
        sslGlobalParams.sslPEMKeyFile =
            boost::filesystem::absolute(params["net.ssl.PEMKeyFile"].as<string>()).generic_string();
    }

    if (params.count("net.ssl.PEMKeyPassword")) {
        sslGlobalParams.sslPEMKeyPassword = params["net.ssl.PEMKeyPassword"].as<string>();
    }

    if (params.count("net.ssl.clusterFile")) {
        sslGlobalParams.sslClusterFile =
            boost::filesystem::absolute(params["net.ssl.clusterFile"].as<string>())
                .generic_string();
    }

    if (params.count("net.ssl.clusterPassword")) {
        sslGlobalParams.sslClusterPassword = params["net.ssl.clusterPassword"].as<string>();
    }

    if (params.count("net.ssl.CAFile")) {
        sslGlobalParams.sslCAFile =
            boost::filesystem::absolute(params["net.ssl.CAFile"].as<std::string>())
                .generic_string();
    }

    if (params.count("net.ssl.CRLFile")) {
        sslGlobalParams.sslCRLFile =
            boost::filesystem::absolute(params["net.ssl.CRLFile"].as<std::string>())
                .generic_string();
    }

    if (params.count("net.ssl.sslCipherConfig")) {
        warning()
            << "net.ssl.sslCipherConfig is deprecated. It will be removed in a future release.";
        if (!sslGlobalParams.sslCipherConfig.empty()) {
            return Status(ErrorCodes::BadValue,
                          "net.ssl.sslCipherConfig is incompatible with the openSSLCipherConfig "
                          "setParameter");
        }
        sslGlobalParams.sslCipherConfig = params["net.ssl.sslCipherConfig"].as<string>();
    }

    if (params.count("net.ssl.disabledProtocols")) {
        // The disabledProtocols field is composed of a comma separated list of protocols to
        // disable. First, tokenize the field.
        std::vector<std::string> tokens =
            StringSplitter::split(params["net.ssl.disabledProtocols"].as<string>(), ",");

        // All accepted tokens, and their corresponding enum representation. The noTLS* tokens
        // exist for backwards compatibility.
        const std::map<std::string, SSLParams::Protocols> validConfigs{
            {"TLS1_0", SSLParams::Protocols::TLS1_0},
            {"noTLS1_0", SSLParams::Protocols::TLS1_0},
            {"TLS1_1", SSLParams::Protocols::TLS1_1},
            {"noTLS1_1", SSLParams::Protocols::TLS1_1},
            {"TLS1_2", SSLParams::Protocols::TLS1_2},
            {"noTLS1_2", SSLParams::Protocols::TLS1_2}};

        // Map the tokens to their enum values, and push them onto the list of disabled protocols.
        for (const std::string& token : tokens) {
            auto mappedToken = validConfigs.find(token);
            if (mappedToken != validConfigs.end()) {
                sslGlobalParams.sslDisabledProtocols.push_back(mappedToken->second);
            } else {
                return Status(ErrorCodes::BadValue,
                              "Unrecognized disabledProtocols '" + token + "'");
            }
        }
    }

    if (params.count("net.ssl.weakCertificateValidation")) {
        sslGlobalParams.sslWeakCertificateValidation =
            params["net.ssl.weakCertificateValidation"].as<bool>();
    } else if (params.count("net.ssl.allowConnectionsWithoutCertificates")) {
        sslGlobalParams.sslWeakCertificateValidation =
            params["net.ssl.allowConnectionsWithoutCertificates"].as<bool>();
    }
    if (params.count("net.ssl.allowInvalidHostnames")) {
        sslGlobalParams.sslAllowInvalidHostnames =
            params["net.ssl.allowInvalidHostnames"].as<bool>();
    }
    if (params.count("net.ssl.allowInvalidCertificates")) {
        sslGlobalParams.sslAllowInvalidCertificates =
            params["net.ssl.allowInvalidCertificates"].as<bool>();
    }
    if (params.count("net.ssl.FIPSMode")) {
        sslGlobalParams.sslFIPSMode = params["net.ssl.FIPSMode"].as<bool>();
    }

#ifdef MONGO_CONFIG_SSL_CERTIFICATE_SELECTORS
    if (params.count("net.ssl.certificateSelector")) {
        const auto status =
            parseCertificateSelector(&sslGlobalParams.sslCertificateSelector,
                                     "net.ssl.certificateSelector",
                                     params["net.ssl.certificateSelector"].as<std::string>());
        if (!status.isOK()) {
            return status;
        }
    }
    if (params.count("net.ssl.ClusterCertificateSelector")) {
        const auto status = parseCertificateSelector(
            &sslGlobalParams.sslClusterCertificateSelector,
            "net.ssl.clusterCertificateSelector",
            params["net.ssl.clusterCertificateSelector"].as<std::string>());
        if (!status.isOK()) {
            return status;
        }
    }
#endif

    int clusterAuthMode = serverGlobalParams.clusterAuthMode.load();
    if (sslGlobalParams.sslMode.load() != SSLParams::SSLMode_disabled) {
        if (sslGlobalParams.sslPEMKeyFile.size() == 0) {
            return Status(ErrorCodes::BadValue, "need sslPEMKeyFile when SSL is enabled");
        }
        if (!sslGlobalParams.sslCRLFile.empty() && sslGlobalParams.sslCAFile.empty()) {
            return Status(ErrorCodes::BadValue, "need sslCAFile with sslCRLFile");
        }
        std::string sslCANotFoundError(
            "No SSL certificate validation can be performed since"
            " no CA file has been provided; please specify an"
            " sslCAFile parameter");

        if (sslGlobalParams.sslCAFile.empty() &&
            clusterAuthMode == ServerGlobalParams::ClusterAuthMode_x509) {
            return Status(ErrorCodes::BadValue, sslCANotFoundError);
        }
    } else if (sslGlobalParams.sslPEMKeyFile.size() || sslGlobalParams.sslPEMKeyPassword.size() ||
               sslGlobalParams.sslClusterFile.size() || sslGlobalParams.sslClusterPassword.size() ||
               sslGlobalParams.sslCAFile.size() || sslGlobalParams.sslCRLFile.size() ||
               sslGlobalParams.sslCipherConfig.size() ||
               sslGlobalParams.sslDisabledProtocols.size() ||
               sslGlobalParams.sslWeakCertificateValidation) {
        return Status(ErrorCodes::BadValue,
                      "need to enable SSL via the sslMode flag when "
                      "using SSL configuration parameters");
    }
    if (clusterAuthMode == ServerGlobalParams::ClusterAuthMode_sendKeyFile ||
        clusterAuthMode == ServerGlobalParams::ClusterAuthMode_sendX509 ||
        clusterAuthMode == ServerGlobalParams::ClusterAuthMode_x509) {
        if (sslGlobalParams.sslMode.load() == SSLParams::SSLMode_disabled) {
            return Status(ErrorCodes::BadValue, "need to enable SSL via the sslMode flag");
        }
    }
    if (sslGlobalParams.sslMode.load() == SSLParams::SSLMode_allowSSL) {
        // allowSSL and x509 is valid only when we are transitioning to auth.
        if (clusterAuthMode == ServerGlobalParams::ClusterAuthMode_sendX509 ||
            (clusterAuthMode == ServerGlobalParams::ClusterAuthMode_x509 &&
             !serverGlobalParams.transitionToAuth)) {
            return Status(ErrorCodes::BadValue,
                          "cannot have x.509 cluster authentication in allowSSL mode");
        }
    }
    return Status::OK();
}

Status storeSSLClientOptions(const moe::Environment& params) {
    if (params.count("ssl") && params["ssl"].as<bool>() == true) {
        sslGlobalParams.sslMode.store(SSLParams::SSLMode_requireSSL);
    }
    if (params.count("ssl.PEMKeyFile")) {
        sslGlobalParams.sslPEMKeyFile = params["ssl.PEMKeyFile"].as<std::string>();
    }
    if (params.count("ssl.PEMKeyPassword")) {
        sslGlobalParams.sslPEMKeyPassword = params["ssl.PEMKeyPassword"].as<std::string>();
    }
    if (params.count("ssl.CAFile")) {
        sslGlobalParams.sslCAFile = params["ssl.CAFile"].as<std::string>();
    }
    if (params.count("ssl.CRLFile")) {
        sslGlobalParams.sslCRLFile = params["ssl.CRLFile"].as<std::string>();
    }
    if (params.count("net.ssl.allowInvalidHostnames")) {
        sslGlobalParams.sslAllowInvalidHostnames =
            params["net.ssl.allowInvalidHostnames"].as<bool>();
    }
    if (params.count("ssl.allowInvalidCertificates")) {
        sslGlobalParams.sslAllowInvalidCertificates = true;
    }
    if (params.count("ssl.FIPSMode")) {
        sslGlobalParams.sslFIPSMode = true;
    }
#ifdef MONGO_CONFIG_SSL_CERTIFICATE_SELECTORS
    if (params.count("ssl.certificateSelector")) {
        const auto status =
            parseCertificateSelector(&sslGlobalParams.sslCertificateSelector,
                                     "ssl.certificateSelector",
                                     params["ssl.certificateSelector"].as<std::string>());
        if (!status.isOK()) {
            return status;
        }
    }
#endif
    return Status::OK();
}

}  // namespace mongo
