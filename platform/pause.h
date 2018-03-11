/**
 * Copyright (C) 2016 MongoDB Inc.
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

/**
 * MONGO_YIELD_CORE_FOR_SMT
 * - An architecture specific processor hint to allow the processor to yield. It is designed to
 *   improve the performance of spin-wait loops.
 *
 * See src/third_party/wiredtiger/src/include/gcc.h
 */
#ifndef __MSC_VER

#if defined(x86_64) || defined(__x86_64__)

#include <xmmintrin.h>

/* Pause instruction to prevent excess processor bus usage */
#define MONGO_YIELD_CORE_FOR_SMT() _mm_pause()

#elif defined(i386) || defined(__i386__)

#include <xmmintrin.h>

#define MONGO_YIELD_CORE_FOR_SMT() _mm_pause()

#elif defined(__PPC64__) || defined(PPC64)

/* ori 0,0,0 is the PPC64 noop instruction */
#define MONGO_YIELD_CORE_FOR_SMT() __asm__ volatile("ori 0,0,0" ::: "memory")

#elif defined(__aarch64__)

#define MONGO_YIELD_CORE_FOR_SMT() __asm__ volatile("yield" ::: "memory")

#elif defined(__s390x__)

#define MONGO_YIELD_CORE_FOR_SMT() __asm__ volatile("lr 0,0" ::: "memory")

#elif defined(__sparc__)

#define MONGO_YIELD_CORE_FOR_SMT() __asm__ volatile("rd %%ccr, %%g0" ::: "memory")

#else
#error "No processor pause implementation for this architecture."
#endif

#else

// On Windows, use the winnt.h YieldProcessor macro
#define MONGO_YIELD_CORE_FOR_SMT() YieldProcessor()

#endif
