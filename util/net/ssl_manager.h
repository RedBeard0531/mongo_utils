/*    Copyright 2009 10gen Inc.
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

#include <boost/optional.hpp>
#include <memory>
#include <string>

#include "mongo/config.h"

#ifdef MONGO_CONFIG_SSL

#include "mongo/base/disallow_copying.h"
#include "mongo/base/string_data.h"
#include "mongo/bson/bsonobj.h"
#include "mongo/util/decorable.h"
#include "mongo/util/net/sock.h"
#include "mongo/util/net/ssl_types.h"
#include "mongo/util/time_support.h"

// SChannel implementation
#if MONGO_CONFIG_SSL_PROVIDER == SSL_PROVIDER_OPENSSL
#include <openssl/err.h>
#include <openssl/ssl.h>
#endif
#endif  // #ifdef MONGO_CONFIG_SSL

namespace mongo {
/*
 * @return the SSL version std::string prefixed with prefix and suffixed with suffix
 */
const std::string getSSLVersion(const std::string& prefix, const std::string& suffix);
}

#ifdef MONGO_CONFIG_SSL
namespace mongo {
struct SSLParams;

#if MONGO_CONFIG_SSL_PROVIDER == SSL_PROVIDER_OPENSSL
typedef SSL_CTX* SSLContextType;
typedef SSL* SSLConnectionType;
#elif MONGO_CONFIG_SSL_PROVIDER == SSL_PROVIDER_WINDOWS
typedef SCHANNEL_CRED* SSLContextType;
typedef PCtxtHandle SSLConnectionType;
#else
#error "Unknown SSL Provider"
#endif

/**
 * Maintain per connection SSL state for the Sock class. Used by SSLManagerInterface to perform SSL
 * operations.
 */
class SSLConnectionInterface {
public:
    virtual ~SSLConnectionInterface();

    virtual std::string getSNIServerName() const = 0;
};

struct SSLConfiguration {
    SSLConfiguration() : serverSubjectName(""), clientSubjectName("") {}
    SSLConfiguration(const std::string& serverSubjectName,
                     const std::string& clientSubjectName,
                     const Date_t& serverCertificateExpirationDate)
        : serverSubjectName(serverSubjectName),
          clientSubjectName(clientSubjectName),
          serverCertificateExpirationDate(serverCertificateExpirationDate) {}

    bool isClusterMember(StringData subjectName) const;
    BSONObj getServerStatusBSON() const;
    std::string serverSubjectName;
    std::string clientSubjectName;
    Date_t serverCertificateExpirationDate;
    bool hasCA = false;
};

/**
 * Stores information about a globally unique OID.
 */
class ASN1OID {
public:
    ASN1OID(std::string identifier, std::string shortDescription, std::string longDescription)
        : identifier(std::move(identifier)),
          shortDescription(std::move(shortDescription)),
          longDescription(std::move(longDescription)) {}

    std::string identifier;        // An OID
    std::string shortDescription;  // A brief description of the entity associated with the OID
    std::string longDescription;   // A long form description of the entity associated with the OID
};
const ASN1OID mongodbRolesOID("1.3.6.1.4.1.34601.2.1.1",
                              "MongoRoles",
                              "Sequence of MongoDB Database Roles");

class SSLManagerInterface : public Decorable<SSLManagerInterface> {
public:
    static std::unique_ptr<SSLManagerInterface> create(const SSLParams& params, bool isServer);

    virtual ~SSLManagerInterface();

    /**
     * Initiates a TLS connection.
     * Throws SocketException on failure.
     * @return a pointer to an SSLConnectionInterface. Resources are freed in
     * SSLConnectionInterface's destructor
     */
    virtual SSLConnectionInterface* connect(Socket* socket) = 0;

    /**
     * Waits for the other side to initiate a TLS connection.
     * Throws SocketException on failure.
     * @return a pointer to an SSLConnectionInterface. Resources are freed in
     * SSLConnectionInterface's destructor
     */
    virtual SSLConnectionInterface* accept(Socket* socket, const char* initialBytes, int len) = 0;

    /**
     * Fetches a peer certificate and validates it if it exists
     * Throws NetworkException on failure
     * @return a std::string containing the certificate's subject name.
     *
     * This version of parseAndValidatePeerCertificate is deprecated because it throws a
     * NetworkException upon failure. New code should prefer the version that returns
     * a StatusWith instead.
     */
    virtual SSLPeerInfo parseAndValidatePeerCertificateDeprecated(
        const SSLConnectionInterface* conn, const std::string& remoteHost) = 0;

    /**
     * Gets the SSLConfiguration containing all information about the current SSL setup
     * @return the SSLConfiguration
     */
    virtual const SSLConfiguration& getSSLConfiguration() const = 0;

    /**
    * Fetches the error text for an error code, in a thread-safe manner.
    */
    static std::string getSSLErrorMessage(int code);

    /**
     * SSL wrappers
     */
    virtual int SSL_read(SSLConnectionInterface* conn, void* buf, int num) = 0;

    virtual int SSL_write(SSLConnectionInterface* conn, const void* buf, int num) = 0;

    virtual int SSL_shutdown(SSLConnectionInterface* conn) = 0;

    enum class ConnectionDirection { kIncoming, kOutgoing };

    /**
     * Initializes an OpenSSL context according to the provided settings. Only settings which are
     * acceptable on non-blocking connections are set. "direction" specifies whether the SSL_CTX
     * will be used to make outgoing connections or accept incoming connections.
     */
    virtual Status initSSLContext(SSLContextType context,
                                  const SSLParams& params,
                                  ConnectionDirection direction) = 0;

    /**
     * Fetches a peer certificate and validates it if it exists. If validation fails, but weak
     * validation is enabled, boost::none will be returned. If validation fails, and invalid
     * certificates are not allowed, a non-OK status will be returned. If validation is successful,
     * an engaged optional containing the certificate's subject name, and any roles acquired by
     * X509 authorization will be returned.
     */
    virtual StatusWith<boost::optional<SSLPeerInfo>> parseAndValidatePeerCertificate(
        SSLConnectionType ssl, const std::string& remoteHost) = 0;
};

// Access SSL functions through this instance.
SSLManagerInterface* getSSLManager();

extern bool isSSLServer;

/**
 * The global SSL configuration. This should be accessed only after global initialization has
 * completed. If it must be accessed in an initializer, the initializer should have
 * "EndStartupOptionStorage" as a prerequisite.
 */
const SSLParams& getSSLGlobalParams();


/**
 * Returns true if the `nameToMatch` is a valid match against the `certHostName` requirement from an
 * x.509 certificate.  Matches a remote host name to an x.509 host name, including wildcards.
 */
bool hostNameMatchForX509Certificates(std::string nameToMatch, std::string certHostName);

/**
 * Parse a binary blob of DER encoded ASN.1 into a set of RoleNames.
 */
StatusWith<stdx::unordered_set<RoleName>> parsePeerRoles(ConstDataRange cdrExtension);

}  // namespace mongo
#endif  // #ifdef MONGO_CONFIG_SSL
