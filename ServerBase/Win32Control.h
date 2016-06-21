#pragma once
namespace Win32Control
{
	DWORD NotifyConsoleTypeApplication(_In_ HANDLE const ProcessHandle);
	DWORD NotifyWindowOwner(_In_ HANDLE const ProcessHandle, _In_ HANDLE const ThreadHandle, _In_ HWND const WindowHandle);
	DWORD NotifyForegroundProcess(_In_ HANDLE const ProcessHandle, _In_ BOOL IsForeground);
	DWORD NotifyCaretPosition(_In_ HWND const WindowHandle, _In_ RECT const CursorPixelsInClientArea);
	DWORD NotifyEndTask(_In_ HANDLE const ProcessHandle, _In_ HWND const WindowHandle, _In_ ULONG const EventType, _In_ ULONG const EventFlags);
};

