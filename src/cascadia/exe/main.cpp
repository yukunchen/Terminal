/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"


// The following default masks are used in creating windows
// Make sure that these flags match when switching to fullscreen and back
#define CONSOLE_WINDOW_FLAGS (WS_OVERLAPPEDWINDOW | WS_HSCROLL | WS_VSCROLL)
#define CONSOLE_WINDOW_EX_FLAGS (WS_EX_WINDOWEDGE | WS_EX_ACCEPTFILES | WS_EX_APPWINDOW | WS_EX_LAYERED)

// Window class name
#define CONSOLE_WINDOW_CLASS (L"ConsoleWindowClass")

ATOM g_atomConsoleWindowClass = 0;
bool g_autoPosition = false;
HWND g_hwnd = 0;


LRESULT CALLBACK _WindowProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    // // Save the pointer here to the specific window instance when one is created
    // if (Message == WM_CREATE)
    // {
    //     const CREATESTRUCT* const pCreateStruct = reinterpret_cast<CREATESTRUCT*>(lParam);

    //     Window* const pWindow = reinterpret_cast<Window*>(pCreateStruct->lpCreateParams);
    //     SetWindowLongPtrW(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pWindow));
    // }

    // // Dispatch the message to the specific class instance
    // Window* const pWindow = reinterpret_cast<Window*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));
    // if (pWindow != nullptr)
    // {
    //     return pWindow->ConsoleWindowProc(hWnd, Message, wParam, lParam);
    // }

    // If we get this far, call the default window proc
    return DefWindowProcW(hWnd, Message, wParam, lParam);
}



NTSTATUS _RegisterWindowClass()
{
    NTSTATUS status = STATUS_SUCCESS;

    // Today we never call this more than once.
    // In the future, if we need multiple windows (for tabs, etc.) we will need to make this thread-safe.
    // As such, the window class should always be 0 when we are entering this the first and only time.
    FAIL_FAST_IF(!(g_atomConsoleWindowClass == 0));

    // Only register if we haven't already registered
    if (g_atomConsoleWindowClass == 0)
    {
        // Prepare window class structure
        WNDCLASSEX wc = { 0 };
        wc.cbSize = sizeof(WNDCLASSEX);
        wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC | CS_DBLCLKS;
        wc.lpfnWndProc = _WindowProc;
        wc.cbClsExtra = 0;
        // wc.cbWndExtra = GWL_CONSOLE_WNDALLOC; // See WinUserp.h, but I don't think we need this
        wc.cbWndExtra = 0;
        wc.hInstance = nullptr;
        wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        wc.hbrBackground = nullptr; // We don't want the background painted. It will cause flickering.
        wc.lpszMenuName = nullptr;
        wc.lpszClassName = CONSOLE_WINDOW_CLASS;

        // // Load icons
        // status = Icon::Instance().GetIcons(&wc.hIcon, &wc.hIconSm);

        if (NT_SUCCESS(status))
        {
            g_atomConsoleWindowClass = RegisterClassExW(&wc);

            if (g_atomConsoleWindowClass == 0)
            {
                status = NTSTATUS_FROM_WIN32(GetLastError());
            }
        }
    }

    return status;
}

NTSTATUS _MakeWindow()
{
    const auto x = 100;
    const auto y = 100;
    const auto w = 100;
    const auto h = 100;

    RECT rectProposed = { x, y, x+w, y+h };
    // Attempt to create window
    HWND hWnd = CreateWindowExW(
        CONSOLE_WINDOW_EX_FLAGS,
        CONSOLE_WINDOW_CLASS,
        L"Project Cascadia",
        CONSOLE_WINDOW_FLAGS,
        g_autoPosition ? CW_USEDEFAULT : rectProposed.left,
        rectProposed.top, // field is ignored if CW_USEDEFAULT was chosen above
        RECT_WIDTH(&rectProposed),
        RECT_HEIGHT(&rectProposed),
        HWND_DESKTOP,
        nullptr,
        nullptr,
        // this // handle to this window class, passed to WM_CREATE to help dispatching to this instance
        nullptr
    );

    if (hWnd == nullptr)
    {
        DWORD const gle = GetLastError();
        RIPMSG1(RIP_WARNING, "CreateWindow failed with gle = 0x%x", gle);
        status = NTSTATUS_FROM_WIN32(gle);
    }
    g_hwnd = hWnd;
}

// Routine Description:
// - Main entry point for EXE version of console launching.
//   This can be used as a debugging/diagnostics tool as well as a method of testing the console without
//   replacing the system binary.
// Arguments:
// - hInstance - This module instance pointer is saved for resource lookups.
// - hPrevInstance - Unused pointer to the module instances. See wWinMain definitions @ MSDN for more details.
// - pwszCmdLine - Unused variable. We will look up the command line using GetCommandLineW().
// - nCmdShow - Unused variable specifying window show/hide state for Win32 mode applications.
// Return value:
// - [[noreturn]] - This function will not return. It will kill the thread we were called from and the console server threads will take over.
int CALLBACK wWinMain(
     HINSTANCE hInstance,
     HINSTANCE /*hPrevInstance*/,
     PWSTR /*pwszCmdLine*/,
     int /*nCmdShow*/)
{
    HRESULT hr = S_OK;

    _RegisterWindowClass();
    _MakeWindow();

    // Only do this if startup was successful. Otherwise, this will leave conhost.exe running with no hosted application.
    if (SUCCEEDED(hr))
    {
        // Since the lifetime of conhost.exe is inextricably tied to the lifetime of its client processes we set our process
        // shutdown priority to zero in order to effectively opt out of shutdown process enumeration. Conhost will exit when
        // all of its client processes do.
        SetProcessShutdownParameters(0, 0);

        ExitThread(hr);
    }

    return hr;
}
