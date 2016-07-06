#pragma once

#include <Win32K\winuserk.h>

class Win32Control
{
public:
    static NTSTATUS NotifyConsoleTypeApplication(_In_ HANDLE const ProcessHandle);
    static NTSTATUS NotifyWindowOwner(_In_ HANDLE const ProcessHandle, _In_ HANDLE const ThreadHandle, _In_ HWND const WindowHandle);
    static NTSTATUS NotifyForegroundProcess(_In_ HANDLE const ProcessHandle, _In_ BOOL IsForeground);
    static NTSTATUS NotifyCaretPosition(_In_ HWND const WindowHandle, _In_ RECT const CursorPixelsInClientArea);
    static NTSTATUS NotifyEndTask(_In_ HANDLE const ProcessHandle, _In_ HWND const WindowHandle, _In_ ULONG const EventType, _In_ ULONG const EventFlags);

    ~Win32Control();

private:
    Win32Control();

    Win32Control(Win32Control const&) = delete;
    void operator=(Win32Control const&) = delete;

    static Win32Control& GetInstance();

    wil::unique_hmodule const _User32Dll;

    typedef NTSTATUS(CALLBACK* PfnConsoleControl)(CONSOLECONTROL, PVOID, DWORD);
    PfnConsoleControl const _ConsoleControl;

};
