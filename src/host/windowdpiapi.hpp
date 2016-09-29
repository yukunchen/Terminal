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

#ifdef CON_DPIAPI_INDIRECT
    ~WindowDpiApi();
private:
    static WindowDpiApi& _Instance();
    HMODULE _hUser32;

    WindowDpiApi();
#endif
};