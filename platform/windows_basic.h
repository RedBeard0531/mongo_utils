// windows_basic.h

/*
 *    Copyright 2010 10gen Inc.
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

#if !defined(_WIN32)
#error "windows_basic included but _WIN32 is not defined"
#endif

// "If you define NTDDI_VERSION, you must also define _WIN32_WINNT":
// http://msdn.microsoft.com/en-us/library/windows/desktop/aa383745(v=vs.85).aspx
#if defined(NTDDI_VERSION) && !defined(_WIN32_WINNT)
#error NTDDI_VERSION defined but _WIN32_WINNT is undefined
#endif

// Ensure that _WIN32_WINNT is set to something before we include windows.h. For server builds
// both _WIN32_WINNT and NTDDI_VERSION are set as defines on the command line, but we need
// these here for things like client driver builds, where they may not already be set.
#if !defined(_WIN32_WINNT)
// Can't use symbolic versions here, since we may not have seen sdkddkver.h yet.
#if defined(_WIN64)
// 64-bit builds default to Windows 7/Windows Server 2008 R2 support.
#define _WIN32_WINNT 0x0601
#else
// 32-bit builds default to Windows 7/Windows Server 2008 R2 support.
#define _WIN32_WINNT 0x0601
#endif
#endif

// As above, but for NTDDI_VERSION. Otherwise, <windows.h> would set our NTDDI_VERSION based on
// _WIN32_WINNT, but not select the service pack revision.
#if !defined(NTDDI_VERSION)
// Can't use symbolic versions here, since we may not have seen sdkddkver.h yet.
#if defined(_WIN64)
// 64-bit builds default to Windows 7/Windows Server 2008 R2 support.
#define NTDDI_VERSION 0x06010000
#else
// 32-bit builds default to Windows 7/Windows Server 2008 R2 support.
#define NTDDI_VERSION 0x06010000
#endif
#endif

// No need to set WINVER, SdkDdkVer.h does that for us, we double check this below.

// for rand_s() usage:
#define _CRT_RAND_S
#ifndef NOMINMAX
#define NOMINMAX
#endif

// Do not complain that about standard library functions that Windows believes should have
// underscores in front of them, such as unlink().
#define _CRT_NONSTDC_NO_DEPRECATE

// tell windows.h not to include a bunch of headers we don't need:
#define WIN32_LEAN_AND_MEAN

// Tell windows.h not to define any NT status codes, so that we can
// get the definitions from ntstatus.h, which has a more complete list.
#define WIN32_NO_STATUS

#include <windows.h>
#include <winsock2.h>  //this must be included before the first windows.h include
#include <ws2tcpip.h>

// We must define SECURITY_WIN32 before include sspi.h
#define SECURITY_WIN32

#include <sspi.h>

#include <schannel.h>

#undef WIN32_NO_STATUS

// Obtain a definition for the ntstatus type.
#include <winternl.h>

// Add back in the status definitions so that macro expansions for
// things like STILL_ACTIVE and WAIT_OBJECT_O can be resolved (they
// expand to STATUS_ codes).
#include <ntstatus.h>

// Should come either from the command line, or if not set there, the inclusion of sdkddkver.h
// via windows.h above should set it based in _WIN32_WINNT, which is assuredly set by now.
#if !defined(NTDDI_VERSION)
#error "NTDDI_VERSION is not defined"
#endif

#if !defined(WINVER) || (WINVER != _WIN32_WINNT)
#error "Expected WINVER to have been defined and to equal _WIN32_WINNT"
#endif

#if !defined(NTDDI_WINBLUE)
#error "MongoDB requires Windows SDK 8.1 or higher to build"
#endif

#if !defined(NTDDI_WIN7) || NTDDI_VERSION < NTDDI_WIN7
#error "MongoDB does not support Windows versions older than Windows 7/Windows Server 2008 R2."
#endif
