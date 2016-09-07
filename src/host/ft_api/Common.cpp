/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

HANDLE Common::_hConsole = INVALID_HANDLE_VALUE;

void VerifySucceededGLE(BOOL bResult)
{
    if (!bResult)
    {
        VERIFY_FAIL(NoThrowString().Format(L"API call failed: 0x%x", GetLastError()));
    }
}

BOOL UnadjustWindowRectEx(
    LPRECT prc,
    DWORD dwStyle,
    BOOL fMenu,
    DWORD dwExStyle)
{
    RECT rc;
    SetRectEmpty(&rc);
    BOOL fRc = AdjustWindowRectEx(&rc, dwStyle, fMenu, dwExStyle);
    if (fRc) {
        prc->left -= rc.left;
        prc->top -= rc.top;
        prc->right -= rc.right;
        prc->bottom -= rc.bottom;
    }
    return fRc;
}

static HANDLE GetStdHandleVerify(_In_ const DWORD dwHandleType)
{
    SetVerifyOutput verifySettings(VerifyOutputSettings::LogOnlyFailures);
    const HANDLE hConsole = GetStdHandle(dwHandleType);
    VERIFY_ARE_NOT_EQUAL(hConsole, INVALID_HANDLE_VALUE, L"Ensure we got a valid console handle");
    VERIFY_IS_NOT_NULL(hConsole, L"Ensure we got a non-null console handle");

    return hConsole;
}

HANDLE GetStdOutputHandle()
{
    return GetStdHandleVerify(STD_OUTPUT_HANDLE);
}

HANDLE GetStdInputHandle()
{
    return GetStdHandleVerify(STD_INPUT_HANDLE);
}

bool Common::TestBufferSetup()
{
    // Creating a new screen buffer for each test will make sure that it doesn't interact with the output that TAEF is spewing
    // to the default output buffer at the same time.

    _hConsole = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE,
        0 /*dwShareMode*/,
        NULL /*lpSecurityAttributes*/,
        CONSOLE_TEXTMODE_BUFFER,
        NULL /*lpReserved*/);

    VERIFY_ARE_NOT_EQUAL(_hConsole, INVALID_HANDLE_VALUE, L"Creating our test screen buffer.");

    VERIFY_WIN32_BOOL_SUCCEEDED(SetConsoleActiveScreenBuffer(_hConsole), L"Applying test screen buffer to console");

    return true;
}

bool Common::TestBufferCleanup()
{
    if (_hConsole != INVALID_HANDLE_VALUE)
    {
        // Simply freeing the handle will restore the next screen buffer down in the stack.
        VERIFY_WIN32_BOOL_SUCCEEDED(CloseHandle(_hConsole), L"Removing our test screen buffer.");
    }

    return true;
}
