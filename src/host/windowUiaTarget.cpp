/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"
#include "windowUiaTarget.hpp"
#include "windowUiaProvider.hpp"

#define CONSOLE_UIA_TARGET_WINDOW_CLASS (L"ConsoleUiaTarget")

#define CONSOLE_WINDOW_FLAGS (WS_OVERLAPPEDWINDOW | WS_HSCROLL | WS_VSCROLL)
#define CONSOLE_WINDOW_EX_FLAGS (WS_EX_WINDOWEDGE | WS_EX_ACCEPTFILES | WS_EX_APPWINDOW )

ATOM WindowUiaTarget::s_atomWindowUiaTarget = 0;

WindowUiaTarget::WindowUiaTarget(_In_ HWND hwndParent, _In_ Window* const pWindow) :
    _hwndParent(hwndParent),
    _pWindow(pWindow)
{
    THROW_IF_FAILED(_MakeWindow());
}

WindowUiaTarget::~WindowUiaTarget()
{

}

HRESULT WindowUiaTarget::_MakeWindow()
{
    RETURN_HR_IF_FALSE(E_FAIL, IsWindow(_hwndParent));

    RETURN_IF_FAILED(s_RegisterWindowClass());

    _hwnd = CreateWindowExW(CONSOLE_WINDOW_EX_FLAGS,
                            CONSOLE_UIA_TARGET_WINDOW_CLASS,
                            CONSOLE_UIA_TARGET_WINDOW_CLASS,
                            CONSOLE_WINDOW_FLAGS,
                            0,
                            0,
                            100,
                            100,
                            _hwndParent,
                            nullptr,
                            nullptr,
                            this);

    RETURN_LAST_ERROR_IF_NULL(_hwnd);

    return S_OK;
}

HRESULT WindowUiaTarget::s_RegisterWindowClass()
{
    if (s_atomWindowUiaTarget == 0)
    {
        WNDCLASSEX wc = { 0 };
        wc.cbSize = sizeof(WNDCLASSEX);
        wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC | CS_DBLCLKS;
        wc.lpfnWndProc = s_WindowProc;
        wc.cbClsExtra = 0;
        wc.cbWndExtra = 0;
        wc.hInstance = nullptr;
        wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        wc.hbrBackground = nullptr; // We don't want the background painted. It will cause flickering.
        wc.lpszMenuName = nullptr;
        wc.lpszClassName = CONSOLE_UIA_TARGET_WINDOW_CLASS;
        wc.hIcon = nullptr;
        wc.hIconSm = nullptr;

        s_atomWindowUiaTarget = RegisterClassExW(&wc);
        RETURN_LAST_ERROR_IF_TRUE(0 == s_atomWindowUiaTarget);
    }

    return S_OK;
}

LRESULT CALLBACK WindowUiaTarget::s_WindowProc(_In_ HWND hWnd, _In_ UINT Message, _In_ WPARAM wParam, _In_ LPARAM lParam)
{
    // Save the pointer here to the specific window instance when one is created
    if (Message == WM_CREATE)
    {
        const CREATESTRUCT* const pCreateStruct = reinterpret_cast<CREATESTRUCT*>(lParam);

        WindowUiaTarget* const pWindow = reinterpret_cast<WindowUiaTarget*>(pCreateStruct->lpCreateParams);
        SetWindowLongPtrW(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pWindow));
    }

    // Dispatch the message to the specific class instance
    WindowUiaTarget* const pWindow = reinterpret_cast<WindowUiaTarget*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));
    if (pWindow != nullptr)
    {
        return pWindow->WindowProc(hWnd, Message, wParam, lParam);
    }

    // If we get this far, call the default window proc
    return DefWindowProcW(hWnd, Message, wParam, lParam);
}

LRESULT CALLBACK WindowUiaTarget::WindowProc(_In_ HWND hWnd, _In_ UINT Message, _In_ WPARAM wParam, _In_ LPARAM lParam)
{
    switch (Message)
    {
    case WM_GETOBJECT:
    {
        LRESULT retVal = 0;

        // If we are receiving a request from Microsoft UI Automation framework, then return the basic UIA COM interface.
        if (static_cast<long>(lParam) == static_cast<long>(UiaRootObjectId))
        {
            retVal = UiaReturnRawElementProvider(hWnd, wParam, lParam, new WindowUiaProvider(_pWindow));
        }
        // Otherwise, return 0. We don't implement MS Active Accessibility (the other framework that calls WM_GETOBJECT).

        return retVal;
    }
    }

    return DefWindowProcW(hWnd, Message, wParam, lParam);
}
