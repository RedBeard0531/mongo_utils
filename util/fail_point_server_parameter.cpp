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

#include "mongo/platform/basic.h"

#include "mongo/util/fail_point_server_parameter.h"

#include "mongo/base/status.h"
#include "mongo/bson/json.h"
#include "mongo/util/fail_point.h"
#include "mongo/util/fail_point_registry.h"
#include "mongo/util/fail_point_service.h"

namespace mongo {

const std::string FailPointServerParameter::failPointPrefix = "failpoint.";

FailPointServerParameter::FailPointServerParameter(std::string name, FailPoint* failpoint)
    : ServerParameter(ServerParameterSet::getGlobal(),
                      failPointPrefix + name,
                      true /* allowedToChangeAtStartup */,
                      false /* allowedToChangeAtRuntime */),
      _failpoint(failpoint),
      _failPointName(name) {}

void FailPointServerParameter::append(OperationContext* opCtx,
                                      BSONObjBuilder& b,
                                      const std::string& name) {
    b << name << _failpoint->toBSON();
}

Status FailPointServerParameter::set(const BSONElement& newValueElement) {
    return {ErrorCodes::InternalError,
            "FailPointServerParameter::setFromString() should be used instead of "
            "FailPointServerParameter::set()"};
}

Status FailPointServerParameter::setFromString(const std::string& str) {
    FailPointRegistry* registry = getGlobalFailPointRegistry();
    FailPoint* failPoint = registry->getFailPoint(_failPointName);
    if (failPoint == NULL) {
        return {ErrorCodes::BadValue,
                str::stream() << _failPointName << " not found in fail point registry"};
    }

    BSONObj failPointOptions;
    try {
        failPointOptions = fromjson(str);
    } catch (DBException& ex) {
        return ex.toStatus();
    }

    FailPoint::Mode mode;
    FailPoint::ValType val;
    BSONObj data;
    auto swParsedOptions = FailPoint::parseBSON(failPointOptions);
    if (!swParsedOptions.isOK()) {
        return swParsedOptions.getStatus();
    }
    std::tie(mode, val, data) = std::move(swParsedOptions.getValue());

    failPoint->setMode(mode, val, data);

    return Status::OK();
}

}  // namespace mongo
