/*    Copyright 2015 MongoDB Inc.
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

#include <boost/preprocessor/facilities/overload.hpp>
#include <string>

#include "mongo/platform/compiler.h"
#include "mongo/util/debug_util.h"

namespace mongo {

/**
 * This include exists so that mongo/base/status_with.h can use the invariant macro without causing
 * a circular include chain. It should never be included directly in any other file other than that
 * one (and assert_util.h).
 */

#if !defined(MONGO_INCLUDE_INVARIANT_H_WHITELISTED)
#error "Include assert_util.h instead of invariant.h."
#endif

MONGO_COMPILER_NORETURN void invariantFailed(const char* expr,
                                             const char* file,
                                             unsigned line) noexcept;

// This overload is our legacy invariant, which just takes a condition to test.
//
// ex)   invariant(!condition);
//
//       Invariant failure !condition some/file.cpp 528
//
#define MONGO_invariant_1(Expression) \
    ::mongo::invariantWithLocation((Expression), #Expression, __FILE__, __LINE__)

template <typename T>
inline void invariantWithLocation(const T& testOK,
                                  const char* expr,
                                  const char* file,
                                  unsigned line) {
    if (MONGO_unlikely(!testOK)) {
        ::mongo::invariantFailed(expr, file, line);
    }
}

MONGO_COMPILER_NORETURN void invariantFailedWithMsg(const char* expr,
                                                    const std::string& msg,
                                                    const char* file,
                                                    unsigned line) noexcept;

// This invariant overload accepts a condition and a message, to be logged if the condition is
// false.
//
// ex)   invariant(!condition, "hello!");
//
//       Invariant failure !condition "hello!" some/file.cpp 528
//
#define MONGO_invariant_2(Expression, contextExpr)                                           \
    ::mongo::invariantWithContextAndLocation((Expression),                                   \
                                             #Expression,                                    \
                                             [&]() -> std::string { return (contextExpr); }, \
                                             __FILE__,                                       \
                                             __LINE__)

template <typename T, typename ContextExpr>
inline void invariantWithContextAndLocation(
    const T& testOK, const char* expr, ContextExpr&& contextExpr, const char* file, unsigned line) {
    if (MONGO_unlikely(!testOK)) {
        ::mongo::invariantFailedWithMsg(expr, contextExpr(), file, line);
    }
}

// This helper macro is necessary to make the __VAR_ARGS__ expansion work properly on MSVC.
#define MONGO_expand(x) x

#define invariant(...) \
    MONGO_expand(MONGO_expand(BOOST_PP_OVERLOAD(MONGO_invariant_, __VA_ARGS__))(__VA_ARGS__))

// Behaves like invariant in debug builds and is compiled out in release. Use for checks, which can
// potentially be slow or on a critical path.
#define MONGO_dassert(...) \
    if (kDebugBuild)       \
    invariant(__VA_ARGS__)

#define dassert MONGO_dassert

}  // namespace mongo
