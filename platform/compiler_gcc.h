/*
 * Copyright 2012 10gen Inc.
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

/**
 * Compiler-specific implementations for gcc.
 *
 * Refer to mongo/platform/compiler.h for usage documentation.
 */

#pragma once


#ifdef __clang__
// Our minimum clang version (3.4) doesn't support the "cold" attribute. We could try to use it with
// clang versions that support the attribute, but since Apple uses weird version numbers on clang
// and the main goal with the attribute is to improve our production builds with gcc, it didn't seem
// worth it.
#define MONGO_COMPILER_COLD_FUNCTION
#define MONGO_COMPILER_NORETURN __attribute__((__noreturn__))
// MONGO_WARN_UNUSED_RESULT is only supported in the semantics we want for classes in Clang, not in
// GCC < 7.
#define MONGO_WARN_UNUSED_RESULT_CLASS [[gnu::warn_unused_result]]
#else
#define MONGO_COMPILER_COLD_FUNCTION __attribute__((__cold__))
#define MONGO_COMPILER_NORETURN __attribute__((__noreturn__, __cold__))

// GCC 7 added support for [[nodiscard]] with the semantics we want.
#if defined(__has_cpp_attribute) && __has_cpp_attribute(nodiscard)
#define MONGO_WARN_UNUSED_RESULT_CLASS [[nodiscard]]
#else
#define MONGO_WARN_UNUSED_RESULT_CLASS
#endif

#endif

#define MONGO_COMPILER_VARIABLE_UNUSED __attribute__((__unused__))

#define MONGO_COMPILER_ALIGN_TYPE(ALIGNMENT) __attribute__((__aligned__(ALIGNMENT)))

#define MONGO_COMPILER_ALIGN_VARIABLE(ALIGNMENT) __attribute__((__aligned__(ALIGNMENT)))

// NOTE(schwerin): These visibility and calling-convention macro definitions assume we're not using
// GCC/CLANG to target native Windows. If/when we decide to do such targeting, we'll need to change
// compiler flags on Windows to make sure we use an appropriate calling convention, and configure
// MONGO_COMPILER_API_EXPORT, MONGO_COMPILER_API_IMPORT and MONGO_COMPILER_API_CALLING_CONVENTION
// correctly.  I believe "correctly" is the following:
//
// #ifdef _WIN32
// #define MONGO_COMIPLER_API_EXPORT __attribute__(( __dllexport__ ))
// #define MONGO_COMPILER_API_IMPORT __attribute__(( __dllimport__ ))
// #ifdef _M_IX86
// #define MONGO_COMPILER_API_CALLING_CONVENTION __attribute__((__cdecl__))
// #else
// #define MONGO_COMPILER_API_CALLING_CONVENTION
// #endif
// #else ... fall through to the definitions below.

#define MONGO_COMPILER_API_EXPORT __attribute__((__visibility__("default")))
#define MONGO_COMPILER_API_IMPORT
#define MONGO_COMPILER_API_CALLING_CONVENTION

#define MONGO_likely(x) static_cast<bool>(__builtin_expect(static_cast<bool>(x), 1))
#define MONGO_unlikely(x) static_cast<bool>(__builtin_expect(static_cast<bool>(x), 0))

#define MONGO_COMPILER_ALWAYS_INLINE [[gnu::always_inline]]

#define MONGO_COMPILER_UNREACHABLE __builtin_unreachable()
