//
//    Copyright (C) Microsoft.  All rights reserved.
//
#include <windows.h>
#include <wil\result.h>
#include <wil\resource.h>
#include <wil\wistd_functional.h>
#include <wil\wistd_memory.h>

#include <string>

#define PIPE_NAME L"\\\\.\\pipe\\convtpipe"

bool openConsole()
{
    wchar_t commandline[] = L"OpenConsole.exe --pipe " PIPE_NAME;
    PROCESS_INFORMATION pi = {0};
    STARTUPINFO si = {0};
    si.cb = sizeof(STARTUPINFOW);
    bool fSuccess = !!CreateProcess(
        nullptr,
        commandline,
        nullptr,    // lpProcessAttributes
        nullptr,    // lpThreadAttributes
        false,      // bInheritHandles
        0,          // dwCreationFlags
        nullptr,    // lpEnvironment
        nullptr,    // lpCurrentDirectory
        &si,        //lpStartupInfo
        &pi         //lpProcessInformation
    );


    if (!fSuccess)
    {
        wprintf(L"Failed to launch console\n");
    }
    return fSuccess;
}

bool openVtPipeIn()
{
    wchar_t commandline[] = L"/c start .\\VtPipeIn.exe";
    PROCESS_INFORMATION pi = {0};
    STARTUPINFO si = {0};
    si.cb = sizeof(STARTUPINFOW);
    bool fSuccess = !!CreateProcess(
        L"cmd.exe",
        commandline,
        nullptr,    // lpProcessAttributes
        nullptr,    // lpThreadAttributes
        false,      // bInheritHandles
        0,          // dwCreationFlags
        nullptr,    // lpEnvironment
        nullptr,    // lpCurrentDirectory
        &si,        //lpStartupInfo
        &pi         //lpProcessInformation
    );


    if (!fSuccess)
    {
        wprintf(L"Failed to launch VtPipeIn\n");
    }
    return fSuccess;
}


int __cdecl wmain(int /*argc*/, WCHAR* /*argv[]*/)
{
    HANDLE const hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    THROW_LAST_ERROR_IF_FALSE(GetConsoleMode(hOut, &dwMode));
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    dwMode |= DISABLE_NEWLINE_AUTO_RETURN;
    THROW_LAST_ERROR_IF_FALSE(SetConsoleMode(hOut, dwMode));

    wil::unique_handle pipe;
    pipe.reset(CreateNamedPipeW(PIPE_NAME, PIPE_ACCESS_DUPLEX, PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT, 1, 0, 0, 0, nullptr));
    THROW_IF_HANDLE_INVALID(pipe.get());

    // Open our backing console
    // openVtPipeIn();
    // Sleep(100);
    openConsole();

    THROW_LAST_ERROR_IF_FALSE(ConnectNamedPipe(pipe.get(), nullptr));

    byte buffer[256];
    DWORD dwRead;
    while (true)
    {
        dwRead = 0;
        THROW_LAST_ERROR_IF_FALSE(ReadFile(pipe.get(), buffer, ARRAYSIZE(buffer), &dwRead, nullptr));
        THROW_LAST_ERROR_IF_FALSE(WriteFile(hOut, buffer, dwRead, nullptr, nullptr));
    }

    return 0;
}
