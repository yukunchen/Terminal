/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/
#include "precomp.h"

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

void SetUpExternalCJKTestEnvironment(PCWSTR pwszPathToConsoleBinary, CJKInnerTest pfnTest)
{
    BOOL fSuccess;

    // Use job to prevent child process from running away.
    Log::Comment(L"Set up job object to prevent child process from running away.");
    HANDLE const hJob = CreateJobObjectW(nullptr, nullptr);
    if (CheckLastError(hJob, L"CreateJobObjectW"))
    {
        STARTUPINFOW si = { 0 };
        si.cb = sizeof(STARTUPINFOW);

        PROCESS_INFORMATION pi = { 0 };

        Log::Comment(L"Create suspended process under test.");
        fSuccess = CreateProcessW(pwszPathToConsoleBinary,
                                  nullptr,
                                  nullptr,
                                  nullptr,
                                  FALSE,
                                  CREATE_NEW_CONSOLE | CREATE_SUSPENDED, // in a new window, not the TE window. suspended so we can attach to job.
                                  nullptr,
                                  nullptr,
                                  &si,
                                  &pi);

        if (CheckLastError(fSuccess, L"CreateProcessW"))
        {
            Log::Comment(L"Attach child process to job.");
            fSuccess = AssignProcessToJobObject(hJob, pi.hProcess);
            if (CheckLastError(fSuccess, L"AssignProcessToJobObject"))
            {
                Log::Comment(L"Resume the child process so it can get started.");
                DWORD dwResult = ResumeThread(pi.hThread);
                if (CheckLastErrorNegativeOneFail(dwResult, L"ResumeThread"))
                {
                    Log::Comment(L"Wait a short time for the child to get started.");
                    Sleep(TIMEOUT);

                    Log::Comment(L"Free the console session attached to the test runner by default.");
                    fSuccess = FreeConsole();
                    if (CheckLastError(fSuccess, L"FreeConsole"))
                    {
                        Log::Comment(L"Attach to child process under test's console session.");
                        fSuccess = AttachConsole(pi.dwProcessId);
                        if (CheckLastError(fSuccess, L"AttachConsole"))
                        {
                            Log::Comment(L"Get console output handle.");
                            HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
                            if (CheckLastError(hOut, L"GetStdHandle"))
                            {
                                Log::Comment(L"Get console input handle.");
                                HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
                                if (CheckLastError(hIn, L"GetStdHandle"))
                                {
                                    Log::Comment(L"---THE TEST NOW THAT EVERYTHING IS READY---");
                                    // THE TEST, now that everything is setup.
                                    {
                                        pfnTest(hOut, hIn);
                                    }

                                    Log::Comment(L"Free console input handle.");
                                    CheckLastError(CloseHandle(hIn), L"CloseHandle");
                                }

                                Log::Comment(L"Free console output handle.");
                                CheckLastError(CloseHandle(hOut), L"CloseHandle");
                            }

                            Log::Comment(L"Release child process under test's console session.");
                            CheckLastError(FreeConsole(), L"FreeConsole");
                        }
                    }
                }
            }
        }

        Log::Comment(L"Terminate all processes in job (process under test and any it may have created).");
        CheckLastError(TerminateJobObject(hJob, 0), L"TerminateJobObject");

        Log::Comment(L"Close job handle.");
        CheckLastError(CloseHandle(hJob), L"CloseHandle");
    }
}
