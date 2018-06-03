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

#define MONGO_LOG_DEFAULT_COMPONENT ::mongo::logger::LogComponent::kDefault

#include "mongo/platform/basic.h"

#include "mongo/transport/thread_idle_callback.h"

#include "mongo/util/assert_util.h"
#include "mongo/util/log.h"

namespace mongo {
namespace {

ThreadIdleCallback threadIdleCallback;

}  // namespace

void registerThreadIdleCallback(ThreadIdleCallback callback) {
    invariant(!threadIdleCallback);
    threadIdleCallback = callback;
}

void markThreadIdle() {
    if (!threadIdleCallback) {
        return;
    }
    try {
        threadIdleCallback();
    } catch (...) {
        severe() << "Exception escaped from threadIdleCallback";
        fassertFailedNoTrace(28603);
    }
}

}  // namespace mongo
