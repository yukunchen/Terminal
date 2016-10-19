/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "ProcessList.h"

#include "..\host\conwinuserrefs.h"
#include "..\host\globals.h"
#include "..\host\telemetry.hpp"
#include "..\host\userprivapi.hpp"

HRESULT ConsoleProcessList::AllocProcessData(_In_ CLIENT_ID const * const ClientId,
                                             _In_ ULONG const ulProcessGroupId,
                                             _In_opt_ CONSOLE_PROCESS_HANDLE* const pParentProcessData,
                                             _Outptr_opt_ CONSOLE_PROCESS_HANDLE** const ppProcessData)
{
    assert(g_ciConsoleInformation.IsConsoleLocked());

    CONSOLE_PROCESS_HANDLE* pProcessData = FindProcessInList(ClientId->UniqueProcess);
    if (nullptr != pProcessData)
    {
        // In the GenerateConsoleCtrlEvent it's OK for this process to already have a ProcessData object. However, the other case is someone
        // connecting to our LPC port and they should only do that once, so we fail subsequent connection attempts.
        if (nullptr == pParentProcessData)
        {
            RETURN_HR(E_FAIL);
        }
        else
        {
            if (nullptr != ppProcessData)
            {
                *ppProcessData = pProcessData;
            }
            RETURN_HR(S_OK);
        }
    }

    try
    {
        pProcessData = new CONSOLE_PROCESS_HANDLE(ClientId, ulProcessGroupId);

        _processes.push_back(pProcessData);

        if (nullptr != ppProcessData)
        {
            *ppProcessData = pProcessData;
        }
    }
    CATCH_RETURN();
    
    RETURN_HR(S_OK);
}

// Routine Description:
// - This routine frees any per-process data allocated by the console.
// Arguments:
// - ProcessData - Pointer to the per-process data structure.
// Return Value:
void ConsoleProcessList::FreeProcessData(_In_ CONSOLE_PROCESS_HANDLE* const pProcessData)
{
    assert(g_ciConsoleInformation.IsConsoleLocked());

    _processes.remove(pProcessData);

    delete pProcessData;
}

// Calling FindProcessInList(nullptr) means you want the root process.
CONSOLE_PROCESS_HANDLE* ConsoleProcessList::FindProcessInList(_In_opt_ HANDLE hProcess) const
{
    auto it = _processes.cbegin();

    while (it != _processes.cend())
    {
        CONSOLE_PROCESS_HANDLE* const pProcessHandleRecord = *it;

        if (0 != hProcess)
        {
            if (pProcessHandleRecord->ClientId.UniqueProcess == hProcess)
            {
                return pProcessHandleRecord;
            }
        }
        else
        {
            if (pProcessHandleRecord->RootProcess)
            {
                return pProcessHandleRecord;
            }
        }

        it = std::next(it);
    }

    return nullptr;
}

CONSOLE_PROCESS_HANDLE* ConsoleProcessList::FindProcessByGroupId(_In_ ULONG ProcessGroupId) const
{
    auto it = _processes.cbegin();

    while (it != _processes.cend())
    {
        CONSOLE_PROCESS_HANDLE* const pProcessHandleRecord = *it;
        if (pProcessHandleRecord->ProcessGroupId == ProcessGroupId)
        {
            return pProcessHandleRecord;
        }

        it = std::next(it);
    }

    return nullptr;
}

HRESULT ConsoleProcessList::GetProcessList(_Inout_updates_(*pcProcessList) DWORD* const pProcessList,
                                           _Inout_ DWORD* const pcProcessList) const
{
    HRESULT hr = S_OK;

    DWORD const cProcesses = (DWORD)_processes.size();

    // If we can fit inside the given list space, copy out the data.
    if (cProcesses <= *pcProcessList)
    {
        DWORD cFilled = 0;

        // Loop over the list of processes and fill in the caller's buffer.
        auto it = _processes.cbegin();
        while (it != _processes.cend())
        {
            pProcessList[cFilled++] = HandleToULong((*it)->ClientId.UniqueProcess);
            it = std::next(it);
        }
    }
    else
    {
        hr = E_NOT_SUFFICIENT_BUFFER;
    }

    // Return how many items were copied (or how many values we would need to fit).
    *pcProcessList = cProcesses;

    RETURN_HR(hr);
}

HRESULT ConsoleProcessList::GetTerminationRecordsByGroupId(_In_ DWORD const LimitingProcessId,
                                                           _In_ bool const fCtrlClose,
                                                           _Outptr_result_buffer_all_(*pcRecords) CONSOLE_PROCESS_TERMINATION_RECORD** prgRecords,
                                                           _Out_ size_t* const pcRecords) const
{
    try
    {
        std::deque<std::unique_ptr<CONSOLE_PROCESS_TERMINATION_RECORD>> TermRecords;

        // Dig through known processes looking for a match
        auto it = _processes.cbegin();
        while (it != _processes.cend())
        {
            CONSOLE_PROCESS_HANDLE* const pProcessHandleRecord = *it;

            // If no limit was specified OR if we have a match, generate a new termination record.
            if (0 == LimitingProcessId ||
                pProcessHandleRecord->ProcessGroupId == LimitingProcessId)
            {
                std::unique_ptr<CONSOLE_PROCESS_TERMINATION_RECORD> pNewRecord = std::make_unique<CONSOLE_PROCESS_TERMINATION_RECORD>();

                // If the duplicate failed, the best we can do is to skip including the process in the list and hope it goes away.
                LOG_IF_WIN32_BOOL_FALSE(DuplicateHandle(GetCurrentProcess(),
                                                        pProcessHandleRecord->ProcessHandle.get(),
                                                        GetCurrentProcess(),
                                                        &pNewRecord->ProcessHandle,
                                                        0,
                                                        0,
                                                        DUPLICATE_SAME_ACCESS));

                pNewRecord->ProcessID = pProcessHandleRecord->ClientId.UniqueProcess;

                // If we're hard closing the window, increment the counter.
                if (fCtrlClose)
                {
                    pProcessHandleRecord->TerminateCount++;
                }

                pNewRecord->TerminateCount = pProcessHandleRecord->TerminateCount;

                TermRecords.push_back(std::move(pNewRecord));
            }

            it = std::next(it);
        }

        // From all found matches, convert to C-style array to return
        size_t const cchRetVal = TermRecords.size();
        CONSOLE_PROCESS_TERMINATION_RECORD* pRetVal = new CONSOLE_PROCESS_TERMINATION_RECORD[cchRetVal];
        RETURN_IF_NULL_ALLOC(pRetVal);

        for (size_t i = 0; i < cchRetVal; i++)
        {
            pRetVal[i] = *TermRecords.at(i);
        }

        *prgRecords = pRetVal;
        *pcRecords = cchRetVal;
    }
    CATCH_RETURN();

    RETURN_HR(S_OK);
}

CONSOLE_PROCESS_HANDLE* ConsoleProcessList::GetRootProcess() const
{
    if (!_processes.empty())
    {
        return _processes.front();
    }

    return nullptr;
}

void SetProcessFocus(_In_ HANDLE ProcessHandle, _In_ BOOL Foreground)
{
    SetPriorityClass(ProcessHandle,
                     Foreground ? PROCESS_MODE_BACKGROUND_END : PROCESS_MODE_BACKGROUND_BEGIN);
}

void SetProcessForegroundRights(_In_ const HANDLE hProcess, _In_ const BOOL fForeground)
{
    CONSOLESETFOREGROUND Flags;
    Flags.hProcess = hProcess;
    Flags.bForeground = fForeground;

    UserPrivApi::s_ConsoleControl(UserPrivApi::CONSOLECONTROL::ConsoleSetForeground, &Flags, sizeof(Flags));
}

void ConsoleProcessList::ModifyConsoleProcessFocus(_In_ const BOOL fForeground)
{
    auto it = _processes.cbegin();
    while (it != _processes.cend())
    {
        CONSOLE_PROCESS_HANDLE* const pProcessHandle = *it;

        if (pProcessHandle->ProcessHandle != nullptr)
        {
            SetProcessFocus(pProcessHandle->ProcessHandle.get(), fForeground);
            SetProcessForegroundRights(pProcessHandle->ProcessHandle.get(), fForeground);
        }

        it = std::next(it);
    }

    // Do this for conhost.exe itself, too.
    SetProcessForegroundRights(GetCurrentProcess(), fForeground);
}

bool ConsoleProcessList::IsEmpty() const
{
    return _processes.size() == 0;
}
