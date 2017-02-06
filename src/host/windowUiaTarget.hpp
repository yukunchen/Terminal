/*++
Copyright (c) Microsoft Corporation

Module Name:
- windowUiaTarget.hpp

Abstract:
- This module is used to make an additional window handle so a UIA tree can find it

Author(s):
- Michael Niksa (MiNiksa) 2017
--*/
#pragma once

class Window;

class WindowUiaTarget sealed
{
public:
    WindowUiaTarget(_In_ HWND hwndParent, _In_ Window* const pWindow);
    ~WindowUiaTarget();


private:
    static ATOM s_atomWindowUiaTarget;
    static HRESULT s_RegisterWindowClass();

    HRESULT _MakeWindow();

    // Wndproc
    static LRESULT CALLBACK s_WindowProc(_In_ HWND hwnd, _In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam);
    LRESULT CALLBACK WindowProc(_In_ HWND, _In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam);

    Window* const _pWindow;
    HWND const _hwndParent;
    HWND _hwnd;
};
