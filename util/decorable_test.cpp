/**
 *    Copyright (C) 2015 MongoDB Inc.
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

#include <boost/utility.hpp>

#include "mongo/unittest/unittest.h"
#include "mongo/util/assert_util.h"
#include "mongo/util/decorable.h"
#include "mongo/util/decoration_container.h"
#include "mongo/util/decoration_registry.h"
#include "mongo/util/log.h"

namespace mongo {
namespace {

static int numConstructedAs;
static int numDestructedAs;

class A {
public:
    A() : value(0) {
        ++numConstructedAs;
    }
    ~A() {
        ++numDestructedAs;
    }
    int value;
};

class ThrowA {
public:
    ThrowA() : value(0) {
        uasserted(ErrorCodes::Unauthorized, "Throwing in a constructor");
    }

    int value;
};

class MyDecorable : public Decorable<MyDecorable> {};

TEST(DecorableTest, DecorableType) {
    const auto dd1 = MyDecorable::declareDecoration<A>();
    const auto dd2 = MyDecorable::declareDecoration<A>();
    const auto dd3 = MyDecorable::declareDecoration<int>();
    numConstructedAs = 0;
    numDestructedAs = 0;
    {
        MyDecorable decorable1;
        ASSERT_EQ(2, numConstructedAs);
        ASSERT_EQ(0, numDestructedAs);
        MyDecorable decorable2;
        ASSERT_EQ(4, numConstructedAs);
        ASSERT_EQ(0, numDestructedAs);

        ASSERT_EQ(0, dd1(decorable1).value);
        ASSERT_EQ(0, dd2(decorable1).value);
        ASSERT_EQ(0, dd1(decorable2).value);
        ASSERT_EQ(0, dd2(decorable2).value);
        ASSERT_EQ(0, dd3(decorable2));
        dd1(decorable1).value = 1;
        dd2(decorable1).value = 2;
        dd1(decorable2).value = 3;
        dd2(decorable2).value = 4;
        dd3(decorable2) = 5;
        ASSERT_EQ(1, dd1(decorable1).value);
        ASSERT_EQ(2, dd2(decorable1).value);
        ASSERT_EQ(3, dd1(decorable2).value);
        ASSERT_EQ(4, dd2(decorable2).value);
        ASSERT_EQ(5, dd3(decorable2));
    }
    ASSERT_EQ(4, numDestructedAs);
}

TEST(DecorableTest, SimpleDecoration) {
    numConstructedAs = 0;
    numDestructedAs = 0;
    DecorationRegistry<MyDecorable> registry;
    const auto dd1 = registry.declareDecoration<A>();
    const auto dd2 = registry.declareDecoration<A>();
    const auto dd3 = registry.declareDecoration<int>();

    {
        DecorationContainer<MyDecorable> decorable1(nullptr, &registry);
        ASSERT_EQ(2, numConstructedAs);
        ASSERT_EQ(0, numDestructedAs);
        DecorationContainer<MyDecorable> decorable2(nullptr, &registry);
        ASSERT_EQ(4, numConstructedAs);
        ASSERT_EQ(0, numDestructedAs);

        ASSERT_EQ(0, decorable1.getDecoration(dd1).value);
        ASSERT_EQ(0, decorable1.getDecoration(dd2).value);
        ASSERT_EQ(0, decorable2.getDecoration(dd1).value);
        ASSERT_EQ(0, decorable2.getDecoration(dd2).value);
        ASSERT_EQ(0, decorable2.getDecoration(dd3));
        decorable1.getDecoration(dd1).value = 1;
        decorable1.getDecoration(dd2).value = 2;
        decorable2.getDecoration(dd1).value = 3;
        decorable2.getDecoration(dd2).value = 4;
        decorable2.getDecoration(dd3) = 5;
        ASSERT_EQ(1, decorable1.getDecoration(dd1).value);
        ASSERT_EQ(2, decorable1.getDecoration(dd2).value);
        ASSERT_EQ(3, decorable2.getDecoration(dd1).value);
        ASSERT_EQ(4, decorable2.getDecoration(dd2).value);
        ASSERT_EQ(5, decorable2.getDecoration(dd3));
    }
    ASSERT_EQ(4, numDestructedAs);
}

#ifndef __s390x__
// TODO(SERVER-34872) Re-enable this test, when we know that s390x will have correct exception
// unwind handling.
TEST(DecorableTest, ThrowingConstructor) {
    numConstructedAs = 0;
    numDestructedAs = 0;

    DecorationRegistry<MyDecorable> registry;
    registry.declareDecoration<A>();
    registry.declareDecoration<ThrowA>();
    registry.declareDecoration<A>();

    try {
        DecorationContainer<MyDecorable> d(nullptr, &registry);
    } catch (const AssertionException& ex) {
        ASSERT_EQ(ErrorCodes::Unauthorized, ex.code());
    }
    ASSERT_EQ(1, numConstructedAs);
    ASSERT_EQ(1, numDestructedAs);
}
#endif

TEST(DecorableTest, Alignment) {
    DecorationRegistry<MyDecorable> registry;
    const auto firstChar = registry.declareDecoration<char>();
    const auto firstInt = registry.declareDecoration<int>();
    const auto secondChar = registry.declareDecoration<int>();
    const auto secondInt = registry.declareDecoration<int>();
    DecorationContainer<MyDecorable> d(nullptr, &registry);
    ASSERT_EQ(0U,
              reinterpret_cast<uintptr_t>(&d.getDecoration(firstChar)) %
                  std::alignment_of<char>::value);
    ASSERT_EQ(0U,
              reinterpret_cast<uintptr_t>(&d.getDecoration(secondChar)) %
                  std::alignment_of<char>::value);
    ASSERT_EQ(0U,
              reinterpret_cast<uintptr_t>(&d.getDecoration(firstInt)) %
                  std::alignment_of<int>::value);
    ASSERT_EQ(0U,
              reinterpret_cast<uintptr_t>(&d.getDecoration(secondInt)) %
                  std::alignment_of<int>::value);
}

struct DecoratedOwnerChecker : public Decorable<DecoratedOwnerChecker> {
    const char answer[100] = "The answer to life the universe and everything is 42";
};

// Test all 4 variations of the owner back reference: const pointer, non-const pointer, const
// reference, non-const reference.
struct DecorationWithOwner {
    DecorationWithOwner() {}

    static const DecoratedOwnerChecker::Decoration<DecorationWithOwner> get;

    std::string getTheAnswer1() const {
        // const pointer variant
        auto* const owner = get.owner(this);
        static_assert(std::is_same<const DecoratedOwnerChecker* const, decltype(owner)>::value,
                      "Type of fetched owner pointer is incorrect.");
        return owner->answer;
    }

    std::string getTheAnswer2() {
        // non-const pointer variant
        DecoratedOwnerChecker* const owner = get.owner(this);
        return owner->answer;
    }

    std::string getTheAnswer3() const {
        // const reference variant
        auto& owner = get.owner(*this);
        static_assert(std::is_same<const DecoratedOwnerChecker&, decltype(owner)>::value,
                      "Type of fetched owner reference is incorrect.");
        return owner.answer;
    }

    std::string getTheAnswer4() {
        // Non-const reference variant
        DecoratedOwnerChecker& owner = get.owner(*this);
        return owner.answer;
    }
};

const DecoratedOwnerChecker::Decoration<DecorationWithOwner> DecorationWithOwner::get =
    DecoratedOwnerChecker::declareDecoration<DecorationWithOwner>();


TEST(DecorableTest, DecorationWithOwner) {
    DecoratedOwnerChecker owner;
    const std::string answer = owner.answer;
    ASSERT_NE(answer, "");

    const std::string witness1 = DecorationWithOwner::get(owner).getTheAnswer1();
    ASSERT_EQ(answer, witness1);

    const std::string witness2 = DecorationWithOwner::get(owner).getTheAnswer2();
    ASSERT_EQ(answer, witness2);

    const std::string witness3 = DecorationWithOwner::get(owner).getTheAnswer3();
    ASSERT_EQ(answer, witness3);

    const std::string witness4 = DecorationWithOwner::get(owner).getTheAnswer4();
    ASSERT_EQ(answer, witness4);

    DecorationWithOwner& decoration = DecorationWithOwner::get(owner);
    ASSERT_EQ(&owner, &DecorationWithOwner::get.owner(decoration));
}

}  // namespace
}  // namespace mongo
