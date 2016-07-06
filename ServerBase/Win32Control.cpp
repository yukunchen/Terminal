#include "stdafx.h"
#include "Win32Control.h"

Win32Control::Win32Control() : 
    _User32Dll(THROW_LAST_ERROR_IF_NULL(LoadLibraryW(L"user32.dll"))),
    _ConsoleControl(reinterpret_cast<PfnConsoleControl>(THROW_LAST_ERROR_IF_NULL(GetProcAddress(_User32Dll.get(), "ConsoleControl"))))
{
}

Win32Control::~Win32Control()
{

}

Win32Control& Win32Control::GetInstance()
{
    static Win32Control Instance;
    return Instance;
}

NTSTATUS Win32Control::NotifyConsoleTypeApplication(_In_ HANDLE const ProcessHandle)
{
    CONSOLE_PROCESS_INFO cpi;
    cpi.dwProcessID = HandleToUlong(ProcessHandle);
    cpi.dwFlags = CPI_NEWPROCESSWINDOW;
    return GetInstance()._ConsoleControl(ConsoleNotifyConsoleApplication, &cpi, sizeof(cpi));
}

NTSTATUS Win32Control::NotifyWindowOwner(_In_ HANDLE const ProcessHandle, _In_ HANDLE const ThreadHandle, _In_ HWND const WindowHandle)
{
    CONSOLEWINDOWOWNER cwo;
    cwo.ProcessId = HandleToULong(ProcessHandle);
    cwo.ThreadId = HandleToULong(ThreadHandle);
    cwo.hwnd = WindowHandle;
    return GetInstance()._ConsoleControl(ConsoleSetWindowOwner, &cwo, sizeof(cwo));
}

NTSTATUS Win32Control::NotifyForegroundProcess(_In_ HANDLE const ProcessHandle, _In_ BOOL IsForeground)
{
    CONSOLESETFOREGROUND csf;
    csf.hProcess = ProcessHandle;
    csf.bForeground = IsForeground;
    return GetInstance()._ConsoleControl(ConsoleSetForeground, &csf, sizeof(csf));
}

NTSTATUS Win32Control::NotifyCaretPosition(_In_ HWND const WindowHandle, _In_ RECT const CursorPixelsInClientArea)
{
    CONSOLE_CARET_INFO cci;
    cci.hwnd = WindowHandle;
    cci.rc = CursorPixelsInClientArea;
    return GetInstance()._ConsoleControl(ConsoleSetCaretInfo, &cci, sizeof(cci));
}

NTSTATUS Win32Control::NotifyEndTask(_In_ HANDLE const ProcessHandle,
                                     _In_ HWND const WindowHandle,
                                     _In_ ULONG const EventType,
                                     _In_ ULONG const EventFlags)
{
    CONSOLEENDTASK cet;
    cet.ProcessId = ProcessHandle;
    cet.hwnd = WindowHandle;
    cet.ConsoleEventCode = EventType;
    cet.ConsoleFlags = EventFlags;
    return GetInstance()._ConsoleControl(ConsoleEndTask, &cet, sizeof(cet));
}
