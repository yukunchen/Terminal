//
//    Copyright (C) Microsoft.  All rights reserved.
//
#include <windows.h>
#include <wil\Common.h>
#include <wil\result.h>
#include <wil\resource.h>
#include <wil\wistd_functional.h>
#include <wil\wistd_memory.h>
#include <stdlib.h>     /* srand, rand */
#include <time.h>       /* time */

#include <deque>
#include <memory>
#include <vector>
#include <string>
#include <sstream>
#include <assert.h>

// This wmain exists for help in writing scratch programs while debugging.
int __cdecl wmain(int /*argc*/, WCHAR* /*argv[]*/)
{
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD n = 0;
    printf("After both strings, the cursor should be right after the word.\n");

    WriteConsoleW(hOut, L"GoodX", 5, &n, nullptr);
    WriteConsoleW(hOut, L"\b", 1, &n, nullptr);
    WriteConsoleW(hOut, L" ", 1, &n, nullptr);
    WriteConsoleW(hOut, L"\b", 1, &n, nullptr);
    Sleep(2000);

    WriteConsoleW(hOut, L"\n", 1, &n, nullptr);

    WriteConsoleW(hOut, L"badX", 4, &n, nullptr);
    WriteConsoleW(hOut, L"\b \b", 3, &n, nullptr);
    Sleep(2000);


    printf("\nEnabling VT mode...\n");

    DWORD dwMode = 0;
    GetConsoleMode(hOut, &dwMode);
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    dwMode |= DISABLE_NEWLINE_AUTO_RETURN;
    SetConsoleMode(hOut, dwMode);

    WriteConsoleW(hOut, L"GoodX", 5, &n, nullptr);
    WriteConsoleW(hOut, L"\b", 1, &n, nullptr);
    WriteConsoleW(hOut, L" ", 1, &n, nullptr);
    WriteConsoleW(hOut, L"\b", 1, &n, nullptr);
    Sleep(2000);

    WriteConsoleW(hOut, L"\n", 1, &n, nullptr);

    WriteConsoleW(hOut, L"badX", 4, &n, nullptr);
    WriteConsoleW(hOut, L"\b \b", 3, &n, nullptr);
    Sleep(2000);
    return 0;
}
