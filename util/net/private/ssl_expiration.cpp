/*    Copyright 2014 10gen Inc.
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

#include "mongo/util/net/private/ssl_expiration.h"

#include <string>

#include "mongo/util/log.h"
#include "mongo/util/time_support.h"

namespace mongo {

static const auto oneDay = Hours(24);

CertificateExpirationMonitor::CertificateExpirationMonitor(Date_t date)
    : _certExpiration(date), _lastCheckTime(Date_t::now()) {}

std::string CertificateExpirationMonitor::taskName() const {
    return "CertificateExpirationMonitor";
}

void CertificateExpirationMonitor::taskDoWork() {
    const Milliseconds timeSinceLastCheck = Date_t::now() - _lastCheckTime;

    if (timeSinceLastCheck < oneDay)
        return;

    const Date_t now = Date_t::now();
    _lastCheckTime = now;

    if (_certExpiration <= now) {
        // The certificate has expired.
        warning() << "Server certificate is now invalid. It expired on "
                  << dateToISOStringUTC(_certExpiration);
        return;
    }

    const auto remainingValidDuration = _certExpiration - now;

    if (remainingValidDuration <= 30 * oneDay) {
        // The certificate will expire in the next 30 days.
        warning() << "Server certificate will expire on " << dateToISOStringUTC(_certExpiration)
                  << " in " << durationCount<Hours>(remainingValidDuration) / 24 << " days.";
    }
}

}  // namespace mongo
