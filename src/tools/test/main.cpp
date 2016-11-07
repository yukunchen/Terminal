//
//    Copyright (C) Microsoft.  All rights reserved.
//
#include <precomp.h>
#include <windows.h>
#include <wincon.h>

int __cdecl wmain(int /*argc*/, WCHAR* /*argv[]*/)
{
    HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
    if (hIn == INVALID_HANDLE_VALUE)
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }
    
    DWORD dwInputModes;
    if (!GetConsoleMode(hIn, &dwInputModes))
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }
    
    DWORD const dwEnableVirtualTerminalInput = 0x200; // Until the new wincon.h is published
    if (!SetConsoleMode(hIn, dwInputModes | dwEnableVirtualTerminalInput))
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    while (hIn != nullptr)
    {
        //DWORD dwRead;
        //char ach;
        //ReadConsoleA(hIn, &ach, 1, &dwRead, nullptr);

        int ch = getchar();
        printf("0x%x\r\n", ch);
    }

    return 0;
}

