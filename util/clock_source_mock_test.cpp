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

#include "mongo/unittest/unittest.h"
#include "mongo/util/clock_source_mock.h"

namespace mongo {
namespace {

TEST(ClockSourceMockTest, ClockSourceShouldReportThatItIsNotSystemClock) {
    ClockSourceMock cs;
    ASSERT(!cs.tracksSystemClock());
}

TEST(ClockSourceMockTest, ExpiredAlarmExecutesWhenSet) {
    ClockSourceMock cs;
    int alarmFiredCount = 0;
    const Date_t alarmDate = cs.now();
    const auto alarmAction = [&] { ++alarmFiredCount; };
    ASSERT_OK(cs.setAlarm(alarmDate, alarmAction));
    ASSERT_EQ(1, alarmFiredCount) << cs.now();
    alarmFiredCount = 0;
    cs.advance(Seconds{1});
    ASSERT_EQ(0, alarmFiredCount) << cs.now();
    ASSERT_OK(cs.setAlarm(alarmDate, alarmAction));
    ASSERT_EQ(1, alarmFiredCount) << cs.now();
}

TEST(ClockSourceMockTest, AlarmExecutesAfterExpirationUsingAdvance) {
    ClockSourceMock cs;
    int alarmFiredCount = 0;
    const Date_t alarmDate = cs.now() + Seconds{10};
    const auto alarmAction = [&] { ++alarmFiredCount; };
    ASSERT_OK(cs.setAlarm(alarmDate, alarmAction));
    ASSERT_EQ(0, alarmFiredCount) << cs.now();
    cs.advance(Seconds{8});
    ASSERT_EQ(0, alarmFiredCount) << cs.now();
    cs.advance(Seconds{1});
    ASSERT_EQ(0, alarmFiredCount) << cs.now();
    cs.advance(Seconds{20});
    ASSERT_EQ(1, alarmFiredCount) << cs.now();
    cs.advance(Seconds{1});
    ASSERT_EQ(1, alarmFiredCount) << cs.now();
}

TEST(ClockSourceMockTest, AlarmExecutesAfterExpirationUsingReset) {
    ClockSourceMock cs;
    int alarmFiredCount = 0;
    const Date_t startDate = cs.now();
    const Date_t alarmDate = startDate + Seconds{10};
    const auto alarmAction = [&] { ++alarmFiredCount; };
    ASSERT_OK(cs.setAlarm(alarmDate, alarmAction));
    ASSERT_EQ(0, alarmFiredCount) << cs.now();
    cs.reset(startDate + Seconds{8});
    ASSERT_EQ(0, alarmFiredCount) << cs.now();
    cs.reset(startDate + Seconds{9});
    ASSERT_EQ(0, alarmFiredCount) << cs.now();
    cs.reset(startDate + Seconds{20});
    ASSERT_EQ(1, alarmFiredCount) << cs.now();
    cs.reset(startDate + Seconds{21});
    ASSERT_EQ(1, alarmFiredCount) << cs.now();
}

TEST(ClockSourceMockTest, MultipleAlarmsWithSameDeadlineTriggeredAtSameTime) {
    ClockSourceMock cs;
    int alarmFiredCount = 0;
    const Date_t alarmDate = cs.now() + Seconds{10};
    const auto alarmAction = [&] { ++alarmFiredCount; };
    ASSERT_OK(cs.setAlarm(alarmDate, alarmAction));
    ASSERT_OK(cs.setAlarm(alarmDate, alarmAction));
    ASSERT_EQ(0, alarmFiredCount) << cs.now();
    cs.advance(Seconds{20});
    ASSERT_EQ(2, alarmFiredCount) << cs.now();
}

TEST(ClockSourceMockTest, MultipleAlarmsWithDifferentDeadlineTriggeredAtSameTime) {
    ClockSourceMock cs;
    int alarmFiredCount = 0;
    const auto alarmAction = [&] { ++alarmFiredCount; };
    ASSERT_OK(cs.setAlarm(cs.now() + Seconds{1}, alarmAction));
    ASSERT_OK(cs.setAlarm(cs.now() + Seconds{10}, alarmAction));
    ASSERT_EQ(0, alarmFiredCount) << cs.now();
    cs.advance(Seconds{20});
    ASSERT_EQ(2, alarmFiredCount) << cs.now();
}

TEST(ClockSourceMockTest, MultipleAlarmsWithDifferentDeadlineTriggeredAtDifferentTimes) {
    ClockSourceMock cs;
    int alarmFiredCount = 0;
    const auto alarmAction = [&] { ++alarmFiredCount; };
    ASSERT_OK(cs.setAlarm(cs.now() + Seconds{1}, alarmAction));
    ASSERT_OK(cs.setAlarm(cs.now() + Seconds{10}, alarmAction));
    ASSERT_EQ(0, alarmFiredCount) << cs.now();
    cs.advance(Seconds{5});
    ASSERT_EQ(1, alarmFiredCount) << cs.now();
    cs.advance(Seconds{5});
    ASSERT_EQ(2, alarmFiredCount) << cs.now();
}

TEST(ClockSourceMockTest, AlarmScheudlesExpiredAlarmWhenSignaled) {
    ClockSourceMock cs;
    const auto beginning = cs.now();
    int alarmFiredCount = 0;
    ASSERT_OK(cs.setAlarm(beginning + Seconds{1},
                          [&] {
                              ++alarmFiredCount;
                              ASSERT_OK(cs.setAlarm(beginning, [&] { ++alarmFiredCount; }));
                          }));
    ASSERT_EQ(0, alarmFiredCount);
    cs.advance(Seconds{1});
    ASSERT_EQ(2, alarmFiredCount);
}

TEST(ClockSourceMockTest, ExpiredAlarmScheudlesExpiredAlarm) {
    ClockSourceMock cs;
    const auto beginning = cs.now();
    int alarmFiredCount = 0;
    ASSERT_OK(cs.setAlarm(beginning, [&] {
        ++alarmFiredCount;
        ASSERT_OK(cs.setAlarm(beginning, [&] { ++alarmFiredCount; }));
    }));
    ASSERT_EQ(2, alarmFiredCount);
}

TEST(ClockSourceMockTest, AlarmScheudlesAlarmWhenSignaled) {
    ClockSourceMock cs;
    const auto beginning = cs.now();
    int alarmFiredCount = 0;
    ASSERT_OK(cs.setAlarm(beginning + Seconds{1},
                          [&] {
                              ++alarmFiredCount;
                              ASSERT_OK(
                                  cs.setAlarm(beginning + Seconds{2}, [&] { ++alarmFiredCount; }));
                          }));
    ASSERT_EQ(0, alarmFiredCount);
    cs.advance(Seconds{1});
    ASSERT_EQ(1, alarmFiredCount);
    cs.advance(Seconds{1});
    ASSERT_EQ(2, alarmFiredCount);
}
}
}  // namespace mongo
