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

#pragma once

#include "mongo/util/background.h"
#include "mongo/util/time_support.h"

namespace mongo {

class CertificateExpirationMonitor : public PeriodicTask {
public:
    explicit CertificateExpirationMonitor(Date_t date);

    /**
     * Gets the PeriodicTask's name.
     * @return CertificateExpirationMonitor's name.
     */
    virtual std::string taskName() const;

    /**
     * Wakes up every minute as it is a PeriodicTask.
     * Checks once a day if the server certificate has expired
     * or will expire in the next 30 days and sends a warning
     * to the log accordingly.
     */
    virtual void taskDoWork();

private:
    const Date_t _certExpiration;
    Date_t _lastCheckTime;
};

}  // namespace mongo
