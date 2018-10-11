/*++
Copyright (c) Microsoft Corporation

Module Name:
- windowtheme.hpp

Abstract:
- This module is used for abstracting calls to set window themes.

Author(s):
- Michael Niksa (MiNiksa) Oct-2018
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

namespace Microsoft::Console::Interactivity::Win32
{
    class WindowTheme final
    {
    public:
        WindowTheme();

        HRESULT TrySetDarkMode(HWND hwnd) const;

    private:
        BOOL _IsDarkMode() const;

        bool _IsHighContrast() const;
        bool _ShouldAppsUseDarkMode() const;

        wil::unique_hmodule _module;
    };
}
