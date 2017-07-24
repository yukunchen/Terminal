//
//    Copyright (C) Microsoft.  All rights reserved.
//
#include <windows.h>


int __cdecl wmain(int /*argc*/, WCHAR* /*argv[]*/)
{
    while (true)
    {
        SleepEx(INFINITE, FALSE);
    }

    return 0;
}
