#include "stdafx.h"
#include "Win32Control.h"

// in Win32Control.cpp only
extern "C"
{
#include <Win32K\winuserk.h>
}

typedef NTSTATUS(CALLBACK* PfnConsoleControl)(CONSOLECONTROL, PVOID, DWORD);

NTSTATUS CallConsoleControl(CONSOLECONTROL Command, PVOID ConsoleInformation, DWORD ConsoleInformationLength)
{
	NTSTATUS Status = STATUS_SUCCESS;

	// TODO: cache this, use direct link, something.
	HMODULE hModule = LoadLibraryW(L"user32.dll");
	if (hModule == nullptr)
	{
		Status = GetLastError();
	}

	if (NT_SUCCESS(Status))
	{
		PfnConsoleControl pfn = (PfnConsoleControl)GetProcAddress(hModule, "ConsoleControl");

		if (pfn == nullptr)
		{
			Status = GetLastError();
		}

		if (NT_SUCCESS(Status))
		{
			Status = pfn(Command, ConsoleInformation, ConsoleInformationLength);
		}

		FreeLibrary(hModule);
	}

	return Status;
}

DWORD Win32Control::NotifyConsoleTypeApplication(_In_ HANDLE const ProcessHandle)
{
	CONSOLE_PROCESS_INFO cpi;
	cpi.dwProcessID = HandleToUlong(ProcessHandle);
	cpi.dwFlags = CPI_NEWPROCESSWINDOW;
	return CallConsoleControl(ConsoleNotifyConsoleApplication, &cpi, sizeof(cpi));
}

DWORD Win32Control::NotifyWindowOwner(_In_ HANDLE const ProcessHandle, _In_ HANDLE const ThreadHandle, _In_ HWND const WindowHandle)
{
	CONSOLEWINDOWOWNER cwo;
	cwo.ProcessId = HandleToULong(ProcessHandle);
	cwo.ThreadId = HandleToULong(ThreadHandle);
	cwo.hwnd = WindowHandle;
	return CallConsoleControl(ConsoleSetWindowOwner, &cwo, sizeof(cwo));
}

DWORD Win32Control::NotifyForegroundProcess(_In_ HANDLE const ProcessHandle, _In_ BOOL IsForeground)
{
	CONSOLESETFOREGROUND csf;
	csf.hProcess = ProcessHandle;
	csf.bForeground = IsForeground;
	return CallConsoleControl(ConsoleSetForeground, &csf, sizeof(csf));
}

DWORD Win32Control::NotifyCaretPosition(_In_ HWND const WindowHandle, _In_ RECT const CursorPixelsInClientArea)
{
	CONSOLE_CARET_INFO cci;
	cci.hwnd = WindowHandle;
	cci.rc = CursorPixelsInClientArea;
	return CallConsoleControl(ConsoleSetCaretInfo, &cci, sizeof(cci));
}

DWORD Win32Control::NotifyEndTask(_In_ HANDLE const ProcessHandle,
								  _In_ HWND const WindowHandle,
								  _In_ ULONG const EventType,
								  _In_ ULONG const EventFlags)
{
	CONSOLEENDTASK cet;
	cet.ProcessId = ProcessHandle;
	cet.hwnd = WindowHandle;
	cet.ConsoleEventCode = EventType;
	cet.ConsoleFlags = EventFlags;
	return CallConsoleControl(ConsoleEndTask, &cet, sizeof(cet));
}
