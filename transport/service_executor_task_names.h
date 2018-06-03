/**
 *    Copyright (C) 2017 MongoDB Inc.
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

namespace mongo {
namespace transport {
enum class ServiceExecutorTaskName {
    kSSMProcessMessage,
    kSSMSourceMessage,
    kSSMExhaustMessage,
    kSSMStartSession,
    kMaxTaskName
};

constexpr auto kSSMProcessMessageName = "processMessage"_sd;
constexpr auto kSSMSourceMessageName = "sourceMessage"_sd;
constexpr auto kSSMExhaustMessageName = "exhaustMessage"_sd;
constexpr auto kSSMStartSessionName = "startSession"_sd;

inline StringData taskNameToString(ServiceExecutorTaskName taskName) {
    switch (taskName) {
        case ServiceExecutorTaskName::kSSMProcessMessage:
            return kSSMProcessMessageName;
        case ServiceExecutorTaskName::kSSMSourceMessage:
            return kSSMSourceMessageName;
        case ServiceExecutorTaskName::kSSMExhaustMessage:
            return kSSMExhaustMessageName;
        case ServiceExecutorTaskName::kSSMStartSession:
            return kSSMStartSessionName;
        default:
            MONGO_UNREACHABLE;
    }
}
}  // namespace transport
}  // namespace mongo
