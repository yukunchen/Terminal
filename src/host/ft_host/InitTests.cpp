/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/
#include "precomp.h"

const DWORD _dwMaxMillisecondsToWaitOnStartup = 120 * 1000;
const DWORD _dwStartupWaitPollingIntervalInMilliseconds = 200;

// This class is intended to set up the testing environment against the produced binary
// instead of using the Windows-default copy of console host.

wil::unique_handle hJob;

static FILE* std_out = nullptr;
static FILE* std_in = nullptr;

// This will automatically try to terminate the job object (and all of the
// binaries under test that are children) whenever this class gets shut down.
// also closes the FILE pointers created by reopening stdin and stdout.
auto OnAppExitKillJob = wil::ScopeExit([&] {
    if (std_out != nullptr)
    {
        fclose(std_out);
    }
    if (std_in != nullptr)
    {
        fclose(std_in);
    }
    if (nullptr != hJob.get())
    {
        THROW_LAST_ERROR_IF_FALSE(TerminateJobObject(hJob.get(), S_OK));
    }
});

MODULE_SETUP(ModuleSetup)
{
    // Retrieve location of directory that the test was deployed to.
    // We're going to look for OpenConsole.exe in the same directory.
    String value;
    VERIFY_SUCCEEDED_RETURN(RuntimeParameters::TryGetValue(L"TestDeploymentDir", value));

    #ifdef __INSIDE_WINDOWS
    value = value.Append(L"Nihilist.exe");
    #else
    value = value.Append(L"OpenConsole.exe Nihilist.exe");
    #endif

    // Must make mutable string of appropriate length to feed into args.
    size_t const cchNeeded = value.GetLength() + 1;

    // We use regular new (not a smart pointer) and a scope exit delete because CreateProcess needs mutable space
    // and it'd be annoying to const_cast the smart pointer's .get() just for the sake of.
    PWSTR str = new WCHAR[cchNeeded];
    auto cleanStr = wil::ScopeExit([&] { if (nullptr != str) { delete[] str; }});

    VERIFY_SUCCEEDED_RETURN(StringCchCopyW(str, cchNeeded, (WCHAR*)value.GetBuffer()));

    // Create a job object to hold the OpenConsole.exe process and the child it creates
    // so we can terminate it easily when we exit.
    hJob.reset(CreateJobObjectW(nullptr, nullptr));
    VERIFY_WIN32_BOOL_SUCCEEDED_RETURN(nullptr != hJob);

    // Setup and call create process.
    STARTUPINFOW si = { 0 };
    si.cb = sizeof(STARTUPINFOW);
    wil::unique_process_information pi;

    // We start suspended so we can put it in the job before it does anything
    // We say new console so it doesn't run in the same window as our test.
    VERIFY_WIN32_BOOL_SUCCEEDED_RETURN(CreateProcessW(nullptr,
                                                      str,
                                                      nullptr,
                                                      nullptr,
                                                      FALSE,
                                                      CREATE_NEW_CONSOLE | CREATE_SUSPENDED,
                                                      nullptr,
                                                      nullptr,
                                                      &si,
                                                      pi.addressof()));

    // Put the new OpenConsole process into the job. The default Job system means when OpenConsole
    // calls CreateProcess, its children will automatically join the job.
    VERIFY_WIN32_BOOL_SUCCEEDED_RETURN(AssignProcessToJobObject(hJob.get(), pi.hProcess));

    // Let the thread run
    VERIFY_WIN32_BOOL_SUCCEEDED_RETURN(-1 != ResumeThread(pi.hThread));

    // We have to enter a wait loop here to compensate for Code Coverage instrumentation that might be
    // injected into the process. That takes a while.
    DWORD dwTotalWait = 0;

    JOBOBJECT_BASIC_PROCESS_ID_LIST pids;
    pids.NumberOfAssignedProcesses = 2;
    while (dwTotalWait < _dwMaxMillisecondsToWaitOnStartup)
    {
        QueryInformationJobObject(hJob.get(),
                                  JobObjectBasicProcessIdList,
                                  &pids,
                                  sizeof(pids),
                                  nullptr);

        // When there is >1 process in the job, OpenConsole has finally got around to starting cmd.exe.
        // It was held up on instrumentation most likely.
        if (pids.NumberOfAssignedProcesses > 1)
        {
            break;
        }
        else if (pids.NumberOfAssignedProcesses < 1)
        {
            VERIFY_FAIL();
        }

        Sleep(_dwStartupWaitPollingIntervalInMilliseconds);
        dwTotalWait += _dwStartupWaitPollingIntervalInMilliseconds;
    }
    // If it took too long, throw so the test ends here.
    VERIFY_IS_LESS_THAN(dwTotalWait, _dwMaxMillisecondsToWaitOnStartup);

    // Now retrieve the actual list of process IDs in the job.
    DWORD cbRequired = sizeof(JOBOBJECT_BASIC_PROCESS_ID_LIST) + sizeof(ULONG_PTR) * pids.NumberOfAssignedProcesses;
    PJOBOBJECT_BASIC_PROCESS_ID_LIST pPidList = reinterpret_cast<PJOBOBJECT_BASIC_PROCESS_ID_LIST>(HeapAlloc(GetProcessHeap(),
                                                                                                             0,
                                                                                                             cbRequired));
    VERIFY_IS_NOT_NULL(pPidList);
    auto scopeExit = wil::ScopeExit([&]() { HeapFree(GetProcessHeap(), 0, pPidList); });

    VERIFY_WIN32_BOOL_SUCCEEDED_RETURN(QueryInformationJobObject(hJob.get(),
                                                                 JobObjectBasicProcessIdList,
                                                                 pPidList,
                                                                 cbRequired,
                                                                 nullptr));

    VERIFY_ARE_EQUAL(pids.NumberOfAssignedProcesses, pPidList->NumberOfProcessIdsInList);

    // Dig through the list to find the one that isn't the OpenConsole window and assume it's CMD.exe
    DWORD dwFindPid = 0;
    for (size_t i = 0; i < pPidList->NumberOfProcessIdsInList; i++)
    {
        ULONG_PTR const pidCandidate = pPidList->ProcessIdList[i];

        if (pidCandidate != pi.dwProcessId && 0 != pidCandidate)
        {
            dwFindPid = static_cast<DWORD>(pidCandidate);
            break;
        }
    }

    #ifdef __INSIDE_WINDOWS
    dwFindPid = pi.dwProcessId;
    #endif

    // Verify we found a valid pid.
    VERIFY_ARE_NOT_EQUAL(0u, dwFindPid);

    // Now detach from our current console (if we have one) and instead attach
    // to the one that belongs to the CMD.exe in the new OpenConsole.exe window.
    VERIFY_WIN32_BOOL_SUCCEEDED_RETURN(FreeConsole());

    // Wait a moment for the driver to be ready after freeing to attach.
    Sleep(1000);

    VERIFY_WIN32_BOOL_SUCCEEDED_RETURN(AttachConsole(dwFindPid));

    // Replace CRT handles
    // These need to be reopened as read/write or they can affect some of the tests.
    //
    // std_out and std_in need to be closed when tests are finished, this is handled by the wil::ScopeExit at the
    // top of this file.
    errno_t err = 0;
    err = freopen_s(&std_out, "CONOUT$", "w+", stdout);
    VERIFY_ARE_EQUAL(0, err);
    err = freopen_s(&std_in, "CONIN$", "r+", stdin);
    VERIFY_ARE_EQUAL(0, err);

    return true;
}

MODULE_CLEANUP(ModuleCleanup)
{
    return true;
}
