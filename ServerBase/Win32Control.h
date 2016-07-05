#pragma once
namespace Win32Control
{
    NTSTATUS NotifyConsoleTypeApplication(_In_ HANDLE const ProcessHandle);
    NTSTATUS NotifyWindowOwner(_In_ HANDLE const ProcessHandle, _In_ HANDLE const ThreadHandle, _In_ HWND const WindowHandle);
    NTSTATUS NotifyForegroundProcess(_In_ HANDLE const ProcessHandle, _In_ BOOL IsForeground);
    NTSTATUS NotifyCaretPosition(_In_ HWND const WindowHandle, _In_ RECT const CursorPixelsInClientArea);
    NTSTATUS NotifyEndTask(_In_ HANDLE const ProcessHandle, _In_ HWND const WindowHandle, _In_ ULONG const EventType, _In_ ULONG const EventFlags);
};

