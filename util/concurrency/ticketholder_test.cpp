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


#define MONGO_LOG_DEFAULT_COMPONENT ::mongo::logger::LogComponent::kDefault

#include "mongo/platform/basic.h"

#include "mongo/unittest/unittest.h"
#include "mongo/util/concurrency/ticketholder.h"

namespace {
using namespace mongo;

TEST(TicketholderTest, BasicTimeout) {
    TicketHolder holder(1);
    ASSERT_EQ(holder.used(), 0);
    ASSERT_EQ(holder.available(), 1);
    ASSERT_EQ(holder.outof(), 1);

    {
        ScopedTicket ticket(&holder);
        ASSERT_EQ(holder.used(), 1);
        ASSERT_EQ(holder.available(), 0);
        ASSERT_EQ(holder.outof(), 1);

        ASSERT_FALSE(holder.tryAcquire());
        ASSERT_FALSE(holder.waitForTicketUntil(Date_t::now()));
        ASSERT_FALSE(holder.waitForTicketUntil(Date_t::now() + Milliseconds(1)));
        ASSERT_FALSE(holder.waitForTicketUntil(Date_t::now() + Milliseconds(42)));
    }

    ASSERT_EQ(holder.used(), 0);
    ASSERT_EQ(holder.available(), 1);
    ASSERT_EQ(holder.outof(), 1);

    ASSERT(holder.waitForTicketUntil(Date_t::now()));
    holder.release();

    ASSERT_EQ(holder.used(), 0);

    ASSERT(holder.waitForTicketUntil(Date_t::now() + Milliseconds(20)));
    ASSERT_EQ(holder.used(), 1);

    ASSERT_FALSE(holder.waitForTicketUntil(Date_t::now() + Milliseconds(2)));
    holder.release();
    ASSERT_EQ(holder.used(), 0);
}
}  // namespace
