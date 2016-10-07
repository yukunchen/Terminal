/*++
Copyright (c) Microsoft Corporation

Module Name:
- windowdpiapi.hpp

Abstract:
- This module is used for abstracting calls to High DPI APIs.

Author(s):
- Michael Niksa (MiNiksa) Apr-2016
--*/
#pragma once

// This type is used in an RS1 API, so we reproduce it for downlevel builds.
#ifdef CON_DPIAPI_INDIRECT

// To avoid a break when the RS1 SDK gets dropped in, don't redef.
#ifndef _DPI_AWARENESS_CONTEXTS_

DECLARE_HANDLE(DPI_AWARENESS_CONTEXT);

typedef enum DPI_AWARENESS {
    DPI_AWARENESS_INVALID = -1,
    DPI_AWARENESS_UNAWARE = 0,
    DPI_AWARENESS_SYSTEM_AWARE = 1,
    DPI_AWARENESS_PER_MONITOR_AWARE = 2
} DPI_AWARENESS;

#define DPI_AWARENESS_CONTEXT_UNAWARE              ((DPI_AWARENESS_CONTEXT)-1)
#define DPI_AWARENESS_CONTEXT_SYSTEM_AWARE         ((DPI_AWARENESS_CONTEXT)-2)
#define DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE    ((DPI_AWARENESS_CONTEXT)-3)
#define DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 ((DPI_AWARENESS_CONTEXT)-4)

#endif
#endif

class WindowDpiApi sealed
{
public:
    static BOOL s_EnablePerMonitorDialogScaling();

    static BOOL s_EnableChildWindowDpiMessage(_In_ HWND const hwnd, _In_ BOOL const fEnable);

    static int s_GetWindowDPI(_In_ HWND const hwnd);
    
    static BOOL s_AdjustWindowRectExForDpi(_Inout_ LPRECT const lpRect, 
                                           _In_ DWORD const dwStyle, 
                                           _In_ BOOL const bMenu, 
                                           _In_ DWORD const dwExStyle, 
                                           _In_ UINT const dpi);

    static int s_GetSystemMetricsForDpi(_In_ int const nIndex, _In_ UINT const dpi);

    static BOOL s_SetProcessDpiAwarenessContext(_In_ DPI_AWARENESS_CONTEXT dpiContext);

#ifdef CON_DPIAPI_INDIRECT
    ~WindowDpiApi();
private:
    static WindowDpiApi& _Instance();
    HMODULE _hUser32;

    WindowDpiApi();
#endif
};