#include "stdafx.h"
#include "Win32Control.h"

// in Win32Control.cpp only
extern "C"
{
#include <Win32K\winuserk.h>
}

typedef NTSTATUS(CALLBACK* PfnConsoleControl)(CONSOLECONTROL, PVOID, DWORD);

NTSTATUS DoConsoleControl(CONSOLECONTROL Command, PVOID ConsoleInformation, DWORD ConsoleInformationLength)
{
	NTSTATUS Status = STATUS_SUCCESS;

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
	return DoConsoleControl(ConsoleNotifyConsoleApplication, &cpi, sizeof(cpi));
}
