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

// This will automatically try to terminate the job object (and all of the
// binaries under test that are children) whenever this class gets shut down.
//auto OnAppExitKillJob = wil::ScopeExit([&] {
//    if (nullptr != hJob.get())
//    {
//        THROW_LAST_ERROR_IF_FALSE(TerminateJobObject(hJob.get(), S_OK));
//    }
//});

MODULE_SETUP(ModuleSetup)
{
    // Retrieve location of directory that the test was deployed to.
    // We're going to look for OpenConsole.exe in the same directory.
    String value;
    VERIFY_SUCCEEDED(RuntimeParameters::TryGetValue(L"TestDeploymentDir", value));

    value = value.Append(L"OpenConsole.exe");

    LPCWSTR str = (LPCWSTR)value.GetBuffer();

    // Create a job object to hold the OpenConsole.exe process and the child it creates
    // so we can terminate it easily when we exit.
    hJob.reset(CreateJobObjectW(nullptr, nullptr));
    THROW_LAST_ERROR_IF_NULL(hJob);

    // Setup and call create process.
    STARTUPINFOW si = { 0 };
    si.cb = sizeof(STARTUPINFOW);
    wil::unique_process_information pi;

    // We start suspended so we can put it in the job before it does anything
    // We say new console so it doesn't run in the same window as our test.
    THROW_LAST_ERROR_IF_FALSE(CreateProcessW(str,
                                             nullptr,
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
    THROW_LAST_ERROR_IF_FALSE(AssignProcessToJobObject(hJob.get(), pi.hProcess));

    // Let the thread run
    THROW_LAST_ERROR_IF(-1 == ResumeThread(pi.hThread));

    // We have to enter a wait loop here to compensate for Code Coverage instrumentation that might be
    // injected into the process. That takes a while.
    DWORD dwTotalWait = 0;

    JOBOBJECT_BASIC_PROCESS_ID_LIST pids;
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

        Sleep(_dwStartupWaitPollingIntervalInMilliseconds);
        dwTotalWait += _dwStartupWaitPollingIntervalInMilliseconds;
    }
    // If it took too long, throw so the test ends here.
    THROW_HR_IF(E_FAIL, dwTotalWait >= _dwMaxMillisecondsToWaitOnStartup);
    
    // Now retrieve the actual list of process IDs in the job.
    DWORD cbRequired = sizeof(JOBOBJECT_BASIC_PROCESS_ID_LIST) + sizeof(ULONG_PTR) * pids.NumberOfAssignedProcesses;
    PJOBOBJECT_BASIC_PROCESS_ID_LIST pPidList = reinterpret_cast<PJOBOBJECT_BASIC_PROCESS_ID_LIST>(HeapAlloc(GetProcessHeap(),
                                                                                                             0,
                                                                                                             cbRequired));

    THROW_LAST_ERROR_IF_FALSE(QueryInformationJobObject(hJob.get(),
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

    // Verify we found a valid pid.
    VERIFY_ARE_NOT_EQUAL(0, dwFindPid);

    // Now detach from our current console (if we have one) and instead attach
    // to the one that belongs to the CMD.exe in the new OpenConsole.exe window.
    THROW_LAST_ERROR_IF_FALSE(FreeConsole());

    // Wait a second to let everything shake out.
    Sleep(1000);

    THROW_LAST_ERROR_IF_FALSE(AttachConsole(dwFindPid));

    return true;
}

MODULE_CLEANUP(ModuleCleanup)
{
    return true;
}
