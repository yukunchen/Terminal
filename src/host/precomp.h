/*++
Copyright (c) Microsoft Corporation

Module Name:
- precomp.h

Abstract:
- Contains external headers to include in the precompile phase of console build process.
- Avoid including internal project headers. Instead include them only in the classes that need them (helps with test project building).
--*/

#pragma once

#define DEFINE_CONSOLEV2_PROPERTIES

// Ignore checked iterators warning from VC compiler.

#ifndef _CRT_SECURE_NO_WARNINGS // Windows build system already defines this, so use ifndef to prevent collision
#define _CRT_SECURE_NO_WARNINGS
#endif

#define _SCL_SECURE_NO_WARNINGS

// This includes a lot of common headers needed by both the host and the propsheet
// including: windows.h, winuser, ntstatus, assert, and the DDK
#include "../CommonIncludes.h"

#define SCREEN_BUFFER_POINTER(X,Y,XSIZE,CELLSIZE) (((XSIZE * (Y)) + (X)) * (ULONG)CELLSIZE)
#include <shellapi.h>

#include <securityappcontainer.h>

#include <condrv.h>

#include <conmsgl1.h>
#include <conmsgl2.h>
#include <conmsgl3.h>

#include <winuser.h>
#include <winconp.h>
#include <ntcon.h>
#include <windowsx.h>
#include <dde.h>
#include "consrv.h"

#include "conv.h"
#include <imm.h>

// STL
#include <string>
#include <list>
#include <memory>
#include <utility>
#include <vector>
#include <memory>

// WIL
// We'll probably want to use this, but there's a conflict in utils.cpp right now.
//#define WIL_SUPPORT_BITOPERATION_PASCAL_NAMES
#include <wil\Common.h>
#include <wil\Result.h>
#include <wil\ResultException.h>

#pragma prefast(push)
#pragma prefast(disable:26071, "Range violation in Intsafe. Not ours.")
#define ENABLE_INTSAFE_SIGNED_FUNCTIONS // Only unsigned intsafe math/casts available without this def
#include <intsafe.h>
#pragma prefast(pop)
#include <strsafe.h>
#include <wchar.h>
#include <mmsystem.h>
#include "utils.hpp"

// Including TraceLogging essentials for the binary
#include <TraceLoggingProvider.h>
#include <winmeta.h>
TRACELOGGING_DECLARE_PROVIDER(g_hConhostV2EventTraceProvider);
#include <telemetry\microsofttelemetry.h>
#include <TraceLoggingActivity.h>
#include "telemetry.hpp"
#include "tracing.hpp"

#ifdef BUILDING_INSIDE_WINIDE
#define DbgRaiseAssertionFailure() __int2c()
#endif

extern "C"
{
    BOOLEAN IsPlaySoundPresent();
};

#include <ShellScalingApi.h>
#include "..\propslib\conpropsp.hpp"

BOOL IsConsoleFullWidth(_In_ HDC hDC, _In_ DWORD CodePage, _In_ WCHAR wch);

// Comment to build against the private SDK.
#define CON_BUILD_PUBLIC

#ifdef CON_BUILD_PUBLIC
    #define CON_USERPRIVAPI_INDIRECT
    #define CON_DPIAPI_INDIRECT
#endif
