//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//

#include <windows.h>
#include <Stdlib.h>
#include <stdio.h>
#include <appmodel.h>
#include <strsafe.h>

#include "util.h"

#pragma optimize("", off)
#pragma warning(disable: 4748)

int __cdecl wmain(int /*argc*/, __in_ecount(argc) PCWSTR* /*argv*/)
{
    TestLibFunc();

    return 0;
}
