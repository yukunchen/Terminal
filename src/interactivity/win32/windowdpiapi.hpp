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

#include "..\inc\IHighDpiApi.hpp"

// Uncomment to build ConFans or other down-level build scenarios.
// #define CON_DPIAPI_INDIRECT

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

#endif

// This type is being defined in RS2 but is percolating through the
// tree. Def it here if it hasn't collided with our branch yet.
#ifndef DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2
#define DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 ((DPI_AWARENESS_CONTEXT)-4)
#endif

namespace Microsoft
{
    namespace Console
    {
        namespace Interactivity
        {
            namespace Win32
            {
                class WindowDpiApi final : public IHighDpiApi
                {
                public:
                    // IHighDpi Interface
                    BOOL SetProcessDpiAwarenessContext();
                    HRESULT SetProcessPerMonitorDpiAwareness();
                    BOOL EnablePerMonitorDialogScaling();

                    // Module-internal Functions
                    BOOL SetProcessDpiAwarenessContext(_In_ DPI_AWARENESS_CONTEXT dpiContext);
                    BOOL EnableChildWindowDpiMessage(_In_ HWND const hwnd,
                                                     _In_ BOOL const fEnable);
                    BOOL AdjustWindowRectExForDpi(_Inout_ LPRECT const lpRect,
                                                  _In_ DWORD const dwStyle,
                                                  _In_ BOOL const bMenu,
                                                  _In_ DWORD const dwExStyle,
                                                  _In_ UINT const dpi);

                    int GetWindowDPI(_In_ HWND const hwnd);
                    int GetSystemMetricsForDpi(_In_ int const nIndex,
                                               _In_ UINT const dpi);

#ifdef CON_DPIAPI_INDIRECT
                    WindowDpiApi();
#endif
                    virtual ~WindowDpiApi();

                private:
                    HMODULE _hUser32;
                };
            }
        }
    }
}