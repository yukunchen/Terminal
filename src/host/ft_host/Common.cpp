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

void DoFailure(PCWSTR pwszFunc, DWORD dwErrorCode)
{
    Log::Comment(NoThrowString().Format(L"'%s' call failed with error 0x%x", pwszFunc, dwErrorCode));
    VERIFY_FAIL();
}

void GlePattern(PCWSTR pwszFunc)
{
    DoFailure(pwszFunc, GetLastError());
}

bool CheckLastErrorNegativeOneFail(DWORD dwReturn, PCWSTR pwszFunc)
{
    if (dwReturn == -1)
    {
        GlePattern(pwszFunc);
        return false;
    }
    else
    {
        return true;
    }
}

bool CheckLastErrorZeroFail(int iValue, PCWSTR pwszFunc)
{
    if (iValue == 0)
    {
        GlePattern(pwszFunc);
        return false;
    }
    else
    {
        return true;
    }
}

bool CheckLastErrorWait(DWORD dwReturn, PCWSTR pwszFunc)
{
    if (CheckLastErrorNegativeOneFail(dwReturn, pwszFunc))
    {
        if (dwReturn == STATUS_WAIT_0)
        {
            return true;
        }
        else
        {
            DoFailure(pwszFunc, dwReturn);
            return false;
        }
    }
    else
    {
        return false;
    }
}

bool CheckLastError(HRESULT hr, PCWSTR pwszFunc)
{
    if (!SUCCEEDED(hr))
    {
        DoFailure(pwszFunc, hr);
        return false;
    }
    else
    {
        return true;
    }
}

bool CheckLastError(BOOL fSuccess, PCWSTR pwszFunc)
{
    if (!fSuccess)
    {
        GlePattern(pwszFunc);
        return false;
    }
    else
    {
        return true;
    }
}

bool CheckLastError(HANDLE handle, PCWSTR pwszFunc)
{
    if (handle == INVALID_HANDLE_VALUE)
    {
        GlePattern(pwszFunc);
        return false;
    }
    else
    {
        return true;
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
