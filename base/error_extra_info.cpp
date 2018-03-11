/**
 *    Copyright 2017 MongoDB, Inc.
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

#include "mongo/base/error_extra_info.h"

#include "mongo/base/init.h"
#include "mongo/db/jsobj.h"

namespace mongo {

bool ErrorExtraInfoExample::isParserEnabledForTest = false;

void ErrorExtraInfoExample::serialize(BSONObjBuilder* builder) const {
    builder->append("data", data);
}

std::shared_ptr<const ErrorExtraInfo> ErrorExtraInfoExample::parse(const BSONObj& obj) {
    uassert(
        40681, "ErrorCodes::ForTestingErrorExtraInfo is only for testing", isParserEnabledForTest);

    return std::make_shared<ErrorExtraInfoExample>(obj["data"].Int());
}

MONGO_INIT_REGISTER_ERROR_EXTRA_INFO(ErrorExtraInfoExample);

}  // namespace mongo
