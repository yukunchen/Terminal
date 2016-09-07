//
//    Copyright (C) Microsoft.  All rights reserved.
//
#include <precomp.h>
#include "windows.h"

int __cdecl wmain(int /*argc*/, WCHAR* /*argv[]*/)
{
    putchar('A');    
    putchar('\x82');
    putchar('\xa0');
    putchar('Z');
    getchar();
    
    return 0;
}
