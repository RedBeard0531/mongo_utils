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
 * Compiler-specific implementations for MSVC.
 *
 * Refer to mongo/platform/compiler.h for usage documentation.
 */

#pragma once


// Microsoft seems opposed to implementing this:
// https://connect.microsoft.com/VisualStudio/feedback/details/804542
#define MONGO_COMPILER_COLD_FUNCTION

#define MONGO_COMPILER_NORETURN __declspec(noreturn)

#define MONGO_COMPILER_VARIABLE_UNUSED

#define MONGO_COMPILER_ALIGN_TYPE(ALIGNMENT) __declspec(align(ALIGNMENT))

#define MONGO_COMPILER_ALIGN_VARIABLE(ALIGNMENT) __declspec(align(ALIGNMENT))

#define MONGO_COMPILER_API_EXPORT __declspec(dllexport)
#define MONGO_COMPILER_API_IMPORT __declspec(dllimport)

#define MONGO_WARN_UNUSED_RESULT_CLASS

#ifdef _M_IX86
// 32-bit x86 supports multiple of calling conventions.  We build supporting the cdecl convention
// (most common).  By labeling our exported and imported functions as such, we do a small favor to
// 32-bit Windows developers.
#define MONGO_COMPILER_API_CALLING_CONVENTION __cdecl
#else
#define MONGO_COMPILER_API_CALLING_CONVENTION
#endif

#define MONGO_likely(x) bool(x)
#define MONGO_unlikely(x) bool(x)

#define MONGO_COMPILER_ALWAYS_INLINE __forceinline

#define MONGO_COMPILER_UNREACHABLE __assume(false)
