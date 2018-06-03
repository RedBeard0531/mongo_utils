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

#include "mongo/transport/session.h"

#include "mongo/platform/atomic_word.h"
#include "mongo/transport/transport_layer.h"
#include "mongo/util/net/ssl_types.h"

namespace mongo {
namespace transport {

namespace {

AtomicUInt64 sessionIdCounter(0);

}  // namespace

Session::Session() : _id(sessionIdCounter.addAndFetch(1)), _tags(kPending) {}

void Session::setTags(TagMask tagsToSet) {
    mutateTags([tagsToSet](TagMask originalTags) { return (originalTags | tagsToSet); });
}

void Session::unsetTags(TagMask tagsToUnset) {
    mutateTags([tagsToUnset](TagMask originalTags) { return (originalTags & ~tagsToUnset); });
}

void Session::mutateTags(const stdx::function<TagMask(TagMask)>& mutateFunc) {
    TagMask oldValue, newValue;
    do {
        oldValue = _tags.load();
        newValue = mutateFunc(oldValue);

        // Any change to the session tags automatically clears kPending status.
        newValue &= ~kPending;
    } while (_tags.compareAndSwap(oldValue, newValue) != oldValue);
}

Session::TagMask Session::getTags() const {
    return _tags.load();
}

}  // namespace transport
}  // namespace mongo
