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

#pragma once

#include "mongo/util/net/ssl_manager.h"

#include <vector>

#include "mongo/base/status.h"
#include "mongo/config.h"

namespace mongo {

#if (MONGO_CONFIG_SSL_PROVIDER == SSL_PROVIDER_WINDOWS) || \
    (MONGO_CONFIG_SSL_PROVIDER == SSL_PROVIDER_APPLE)
#define MONGO_CONFIG_SSL_CERTIFICATE_SELECTORS 1
#endif

namespace optionenvironment {
class OptionSection;
class Environment;
}  // namespace optionenvironment

struct SSLParams {
    enum class Protocols { TLS1_0, TLS1_1, TLS1_2 };
    AtomicInt32 sslMode;            // --sslMode - the SSL operation mode, see enum SSLModes
    std::string sslPEMTempDHParam;  // --setParameter OpenSSLDiffieHellmanParameters=file : PEM file
                                    // with DH parameters.
    std::string sslPEMKeyFile;      // --sslPEMKeyFile
    std::string sslPEMKeyPassword;  // --sslPEMKeyPassword
    std::string sslClusterFile;     // --sslInternalKeyFile
    std::string sslClusterPassword;  // --sslInternalKeyPassword
    std::string sslCAFile;           // --sslCAFile
    std::string sslCRLFile;          // --sslCRLFile
    std::string sslCipherConfig;     // --sslCipherConfig

    struct CertificateSelector {
        std::string subject;
        std::vector<uint8_t> thumbprint;
        std::vector<uint8_t> serial;
    };
#ifdef MONGO_CONFIG_SSL_CERTIFICATE_SELECTORS
    CertificateSelector sslCertificateSelector;         // --sslCertificateSelector
    CertificateSelector sslClusterCertificateSelector;  // --sslClusterCertificateSelector
#endif

    std::vector<Protocols> sslDisabledProtocols;  // --sslDisabledProtocols
    bool sslWeakCertificateValidation = false;    // --sslWeakCertificateValidation
    bool sslFIPSMode = false;                     // --sslFIPSMode
    bool sslAllowInvalidCertificates = false;     // --sslAllowInvalidCertificates
    bool sslAllowInvalidHostnames = false;        // --sslAllowInvalidHostnames
    bool disableNonSSLConnectionLogging =
        false;  // --setParameter disableNonSSLConnectionLogging=true

    SSLParams() {
        sslMode.store(SSLMode_disabled);
    }

    enum SSLModes {
        /**
        * Make unencrypted outgoing connections and do not accept incoming SSL-connections.
        */
        SSLMode_disabled,

        /**
        * Make unencrypted outgoing connections and accept both unencrypted and SSL-connections.
        */
        SSLMode_allowSSL,

        /**
        * Make outgoing SSL-connections and accept both unecrypted and SSL-connections.
        */
        SSLMode_preferSSL,

        /**
        * Make outgoing SSL-connections and only accept incoming SSL-connections.
        */
        SSLMode_requireSSL
    };
};

extern SSLParams sslGlobalParams;

Status addSSLServerOptions(mongo::optionenvironment::OptionSection* options);

Status addSSLClientOptions(mongo::optionenvironment::OptionSection* options);

Status storeSSLServerOptions(const mongo::optionenvironment::Environment& params);

Status parseCertificateSelector(SSLParams::CertificateSelector* selector,
                                StringData name,
                                StringData value);
/**
 * Canonicalize SSL options for the given environment that have different representations with
 * the same logical meaning.
 */
Status canonicalizeSSLServerOptions(mongo::optionenvironment::Environment* params);

Status validateSSLServerOptions(const mongo::optionenvironment::Environment& params);

Status storeSSLClientOptions(const mongo::optionenvironment::Environment& params);
}  // namespace mongo
