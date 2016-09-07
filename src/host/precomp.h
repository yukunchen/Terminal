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

// Define and then undefine WIN32_NO_STATUS because windows.h has no guard to prevent it from double defing certain statuses
// when included with ntstatus.h
#define WIN32_NO_STATUS
#include <windows.h>
#undef WIN32_NO_STATUS

// From ntdef.h, but that can't be included or it'll fight over PROBE_ALIGNMENT and other such arch specific defs
typedef _Return_type_success_(return >= 0) LONG NTSTATUS;
/*lint -save -e624 */  // Don't complain about different typedefs.
typedef NTSTATUS *PNTSTATUS;
/*lint -restore */  // Resume checking for different typedefs.
#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)

// End From ntdef.h

#include <ntstatus.h>

#include <assert.h>

#define SCREEN_BUFFER_POINTER(X,Y,XSIZE,CELLSIZE) (((XSIZE * (Y)) + (X)) * (ULONG)CELLSIZE)
#include <initguid.h>
#include <shlobj.h>
#include <winuser.h>
#include <shellapi.h>

// Only remaining item from w32gdip.h. There's probably a better way to do this as well.
#define IS_ANY_DBCS_CHARSET( CharSet )                              \
                   ( ((CharSet) == SHIFTJIS_CHARSET)    ? TRUE :    \
                     ((CharSet) == HANGEUL_CHARSET)     ? TRUE :    \
                     ((CharSet) == CHINESEBIG5_CHARSET) ? TRUE :    \
                     ((CharSet) == GB2312_CHARSET)      ? TRUE : FALSE )

#include <securityappcontainer.h>

#include "conddkrefs.h"
#include "conwinuserrefs.h"

#include "condrv.h"

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
#include <immp.h>

// STL
#include <string>
#include <list>

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
#include <conpropsp.hpp>

BOOL IsConsoleFullWidth(_In_ HDC hDC, _In_ DWORD CodePage, _In_ WCHAR wch);

// Comment to build against the private SDK.
#define CON_BUILD_PUBLIC

#ifdef CON_BUILD_PUBLIC
    #define CON_USERPRIVAPI_INDIRECT
    #define CON_DPIAPI_INDIRECT
#endif
