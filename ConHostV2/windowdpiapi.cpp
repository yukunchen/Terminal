/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

// Uncomment to build ConFans or other down-level build scenarios.
//#define CON_DPIAPI_INDIRECT

#include "windowdpiapi.hpp"

BOOL WindowDpiApi::s_EnablePerMonitorDialogScaling()
{
#ifdef CON_DPIAPI_INDIRECT
    HMODULE hUser32 = _Instance()._hUser32;

    if (hUser32 != nullptr)
    {
        FARPROC pfnFunc = GetProcAddress(hUser32, "EnablePerMonitorDialogScaling");

        if (pfnFunc == nullptr)
        {
            // If retrieve by name fails, we have to get it by the ordinal because it was made a secret.
            pfnFunc = GetProcAddress(hUser32, MAKEINTRESOURCEA(2577));
        }

        if (pfnFunc != nullptr)
        {
            return (BOOL)pfnFunc();
        }
    }

    return FALSE;
#else
    return EnablePerMonitorDialogScaling();
#endif
}

BOOL WindowDpiApi::s_EnableChildWindowDpiMessage(_In_ HWND const hwnd, _In_ BOOL const fEnable)
{
#ifdef CON_DPIAPI_INDIRECT
    HMODULE hUser32 = _Instance()._hUser32;

    if (hUser32 != nullptr)
    {
        typedef BOOL(*PfnEnableChildWindowDpiMessage)(HWND hwnd, BOOL fEnable);

        PfnEnableChildWindowDpiMessage pfn = (PfnEnableChildWindowDpiMessage)GetProcAddress(hUser32, "EnableChildWindowDpiMessage");

        if (pfn == nullptr)
        {
            // If that fails, we have to get it by the ordinal because it was made a secret in RS1.
            pfn = (PfnEnableChildWindowDpiMessage)GetProcAddress(hUser32, MAKEINTRESOURCEA(2704));
        }

        if (pfn != nullptr)
        {
            return pfn(hwnd, fEnable);
        }
    }

    return FALSE;
#else
    return EnableChildWindowDpiMessage(hwnd, fEnable);
#endif
}

int WindowDpiApi::s_GetWindowDPI(_In_ HWND const hwnd)
{
#ifdef CON_DPIAPI_INDIRECT
    HMODULE hUser32 = _Instance()._hUser32;

    if (hUser32 != nullptr)
    {
        typedef int(*PfnGetWindowDPI)(HWND hwnd);

        PfnGetWindowDPI pfn = (PfnGetWindowDPI)GetProcAddress(hUser32, "GetWindowDPI");

        if (pfn == nullptr)
        {
            // If that fails, we have to get it by the ordinal because it was made a secret in RS1.
            pfn = (PfnGetWindowDPI)GetProcAddress(hUser32, MAKEINTRESOURCEA(2707));
        }

        if (pfn != nullptr)
        {
            return pfn(hwnd);
        }
    }

    return USER_DEFAULT_SCREEN_DPI;
#else
    return GetWindowDPI(hwnd);
#endif
}

BOOL WindowDpiApi::s_AdjustWindowRectExForDpi(_Inout_ LPRECT const lpRect, 
                                              _In_ DWORD const dwStyle, 
                                              _In_ BOOL const bMenu, 
                                              _In_ DWORD const dwExStyle, 
                                              _In_ UINT const dpi)
{
#ifdef CON_DPIAPI_INDIRECT
    HMODULE hUser32 = _Instance()._hUser32;

    if (hUser32 != nullptr)
    {
        typedef BOOL(*PfnAdjustWindowRectExForDpi)(LPRECT lpRect, DWORD dwStyle, BOOL bMenu, DWORD dwExStyle, int dpi);

        // Try to retrieve it the RS1 way first
        PfnAdjustWindowRectExForDpi pfn = (PfnAdjustWindowRectExForDpi)GetProcAddress(hUser32, "AdjustWindowRectExForDpi");

        if (pfn == nullptr)
        {
            // If that fails, we have to get it by the ordinal because it was made a secret in TH/TH2.
            pfn = (PfnAdjustWindowRectExForDpi)GetProcAddress(hUser32, MAKEINTRESOURCEA(2580));
        }

        if (pfn != nullptr)
        {
            return pfn(lpRect, dwStyle, bMenu, dwExStyle, dpi);
        }
    }

    return AdjustWindowRectEx(lpRect, dwStyle, bMenu, dwExStyle);
#else
    return AdjustWindowRectExForDpi(lpRect, dwStyle, bMenu, dwExStyle, dpi);
#endif
}

int WindowDpiApi::s_GetSystemMetricsForDpi(_In_ int const nIndex, _In_ UINT const dpi)
{
#ifdef CON_DPIAPI_INDIRECT
    HMODULE hUser32 = _Instance()._hUser32;

    if (hUser32 != nullptr)
    {
        typedef int(*PfnGetDpiMetrics)(int nIndex, int dpi);

        // First try the TH1/TH2 name of the function.
        PfnGetDpiMetrics pfn = (PfnGetDpiMetrics)GetProcAddress(hUser32, "GetDpiMetrics");

        if (pfn == nullptr)
        {
            // If not, then try the RS1 name of the function.
            pfn = (PfnGetDpiMetrics)GetProcAddress(hUser32, "GetSystemMetricsForDpi");
        }

        if (pfn != nullptr)
        {
            return pfn(nIndex, dpi);
        }

    }

    return GetSystemMetrics(nIndex);
#else
    return GetSystemMetricsForDpi(nIndex, dpi);
#endif
}

#ifdef CON_DPIAPI_INDIRECT
WindowDpiApi::WindowDpiApi()
{
    _hUser32 = LoadLibraryW(L"user32.dll");
}

WindowDpiApi::~WindowDpiApi()
{
    if (_hUser32 != nullptr)
    {
        FreeLibrary(_hUser32);
        _hUser32 = nullptr;
    }
}

WindowDpiApi& WindowDpiApi::_Instance()
{
    static WindowDpiApi dpiapi;
    return dpiapi;
}
#endif
