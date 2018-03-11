/**
 *    Copyright (C) 2016 MongoDB Inc.
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

#include "mongo/db/server_parameters.h"

#include "mongo/base/status.h"
#include "mongo/util/fail_point.h"

namespace mongo {

class FailPointServerParameter : public ServerParameter {
public:
    // When set via --setParameter on the command line, failpoint names must include this prefix.
    static const std::string failPointPrefix;

    FailPointServerParameter(std::string name, FailPoint* failpoint);

    void append(OperationContext* opCtx, BSONObjBuilder& b, const std::string& name) override;
    Status set(const BSONElement& newValueElement) override;
    Status setFromString(const std::string& str) override;

private:
    FailPoint* _failpoint;  // not owned here
    std::string _failPointName;
};

}  // namespace mongo
