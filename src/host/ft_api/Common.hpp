/*++
Copyright (c) Microsoft Corporation

Module Name:
- Common.hpp

Abstract:
- This module contains common items for the API tests

Author:
- Michael Niksa (MiNiksa)          2015
- Paul Campbell (PaulCam)          2015

Revision History:
--*/

#pragma once

#include <ConsoleTAEFTemplates.hpp>

class Common
{
public:
    static bool TestBufferSetup();
    static bool TestBufferCleanup();
    static HANDLE _hConsole;
};

// Helper to cause a VERIFY_FAIL and get the last error code for functions that return null-like things.
void VerifySucceededGLE(BOOL bResult);

//http://blogs.msdn.com/b/oldnewthing/archive/2013/10/17/10457292.aspx
BOOL UnadjustWindowRectEx(
    LPRECT prc,
    DWORD dwStyle,
    BOOL fMenu,
    DWORD dwExStyle);

HANDLE GetStdInputHandle();
HANDLE GetStdOutputHandle();

