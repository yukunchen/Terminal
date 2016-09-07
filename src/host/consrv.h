/*++
Copyright (c) Microsoft Corporation

Module Name:
- consrv.h

Abstract:
- This module contains the include files and definitions for the console server DLL.

Author:
- Therese Stowell (ThereseS) 16-Nov-1990

Revision History:
- Many items removed into individual classes relevant to individual components (MiNiksa, PaulCam - 2014)
--*/

#pragma once

#include "cmdline.h"
#include "globals.h"
#include "server.h"
#include "settings.hpp"
#include "tracing.hpp"

#define NT_TESTNULL(var) (((var) == nullptr) ? STATUS_NO_MEMORY : STATUS_SUCCESS)
#define NT_TESTNULL_GLE(var) (((var) == nullptr) ? NTSTATUS_FROM_WIN32(GetLastError()) : STATUS_SUCCESS);

extern "C"
{
    NTSTATUS ConsoleControl(_In_ CONSOLECONTROL ConsoleCommand,
                            _In_reads_bytes_opt_(ConsoleInformationLength) PVOID ConsoleInformation,
                            _In_ DWORD ConsoleInformationLength);
};

// TODO: Clean up tagging and memory macros. See http://osgvsowi/8394405
#define TMP_TAG 0
#define BMP_TAG 0
#define ALIAS_TAG 0
#define HISTORY_TAG 0
#define TITLE_TAG 0
#define HANDLE_TAG 0
#define CONSOLE_TAG 0
#define ICON_TAG 0
#define BUFFER_TAG 0
#define WAIT_TAG 0
#define WAITBLOCK_TAG 0
#define FONT_TAG 0
#define SCREEN_TAG 0
#define TMP_DBCS_TAG 0
#define SCREEN_DBCS_TAG 0
#define EUDC_TAG 0
#define CONVAREA_TAG 0
#define IME_TAG 0
#define PAYLOAD_TAG 0

#define ConsoleHeapAlloc(FlagsAndTags, Size) HeapAlloc(GetProcessHeap(), ((FlagsAndTags) & 0xF), (Size))
#define ConsoleHeapReAlloc(FlagsAndTags, Addr, Size) HeapReAlloc(GetProcessHeap(), ((FlagsAndTags) & 0xF), (Addr), (Size))
#define ConsoleHeapFree(Addr) HeapFree(GetProcessHeap(), 0, (Addr))
#define ConsoleHeapSize(Addr) HeapSize(GetProcessHeap(), 0, (Addr))

/*
 * Used to store some console attributes for the console.  This is a means
 * to cache the color in the extra-window-bytes, so USER/KERNEL can get
 * at it for hungapp drawing.  The window-offsets are defined in NTUSER\INC.
 *
 * The other macros are just convenient means for setting the other window
 * bytes.
 */

#define PACKCOORD(pt)   (MAKELONG(((pt).X), ((pt).Y)))

typedef struct _CONSOLE_API_CONNECTINFO
{
    Settings ConsoleInfo;
    BOOLEAN ConsoleApp;
    BOOLEAN WindowVisible;
    DWORD ProcessGroupId;
    DWORD TitleLength;
    WCHAR Title[MAX_PATH + 1];
    DWORD AppNameLength;
    WCHAR AppName[128];
    DWORD CurDirLength;
    WCHAR CurDir[MAX_PATH + 1];
} CONSOLE_API_CONNECTINFO, *PCONSOLE_API_CONNECTINFO;
