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

HRESULT ConsoleProcessList::AllocProcessData(_In_ DWORD const dwProcessId,
                                             _In_ DWORD const dwThreadId,
                                             _In_ ULONG const ulProcessGroupId,
                                             _In_opt_ ConsoleProcessHandle* const pParentProcessData,
                                             _Outptr_opt_ ConsoleProcessHandle** const ppProcessData)
{
    assert(g_ciConsoleInformation.IsConsoleLocked());

    ConsoleProcessHandle* pProcessData = FindProcessInList(dwProcessId);
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
        pProcessData = new ConsoleProcessHandle(dwProcessId,
                                                dwThreadId, 
                                                ulProcessGroupId);

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
void ConsoleProcessList::FreeProcessData(_In_ ConsoleProcessHandle* const pProcessData)
{
    assert(g_ciConsoleInformation.IsConsoleLocked());

    _processes.remove(pProcessData);

    delete pProcessData;
}

// Calling FindProcessInList(0) means you want the root process.
ConsoleProcessHandle* ConsoleProcessList::FindProcessInList(_In_ const DWORD dwProcessId) const
{
    auto it = _processes.cbegin();

    while (it != _processes.cend())
    {
        ConsoleProcessHandle* const pProcessHandleRecord = *it;

        if (0 != dwProcessId)
        {
            if (pProcessHandleRecord->dwProcessId == dwProcessId)
            {
                return pProcessHandleRecord;
            }
        }
        else
        {
            if (pProcessHandleRecord->fRootProcess)
            {
                return pProcessHandleRecord;
            }
        }

        it = std::next(it);
    }

    return nullptr;
}

ConsoleProcessHandle* ConsoleProcessList::FindProcessByGroupId(_In_ ULONG ProcessGroupId) const
{
    auto it = _processes.cbegin();

    while (it != _processes.cend())
    {
        ConsoleProcessHandle* const pProcessHandleRecord = *it;
        if (pProcessHandleRecord->_ulProcessGroupId == ProcessGroupId)
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
            pProcessList[cFilled++] = (*it)->dwProcessId;
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
            ConsoleProcessHandle* const pProcessHandleRecord = *it;

            // If no limit was specified OR if we have a match, generate a new termination record.
            if (0 == LimitingProcessId ||
                pProcessHandleRecord->_ulProcessGroupId == LimitingProcessId)
            {
                std::unique_ptr<CONSOLE_PROCESS_TERMINATION_RECORD> pNewRecord = std::make_unique<CONSOLE_PROCESS_TERMINATION_RECORD>();

                // If the duplicate failed, the best we can do is to skip including the process in the list and hope it goes away.
                LOG_IF_WIN32_BOOL_FALSE(DuplicateHandle(GetCurrentProcess(),
                                                        pProcessHandleRecord->_hProcess.get(),
                                                        GetCurrentProcess(),
                                                        &pNewRecord->hProcess,
                                                        0,
                                                        0,
                                                        DUPLICATE_SAME_ACCESS));

                pNewRecord->dwProcessID = pProcessHandleRecord->dwProcessId;

                // If we're hard closing the window, increment the counter.
                if (fCtrlClose)
                {
                    pProcessHandleRecord->_ulTerminateCount++;
                }

                pNewRecord->ulTerminateCount = pProcessHandleRecord->_ulTerminateCount;

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

ConsoleProcessHandle* ConsoleProcessList::GetRootProcess() const
{
    if (!_processes.empty())
    {
        return _processes.front();
    }

    return nullptr;
}

void ConsoleProcessList::ModifyConsoleProcessFocus(_In_ const bool fForeground)
{
    auto it = _processes.cbegin();
    while (it != _processes.cend())
    {
        ConsoleProcessHandle* const pProcessHandle = *it;

        if (pProcessHandle->_hProcess != nullptr)
        {
            _SetProcessFocus(pProcessHandle->_hProcess.get(), fForeground);
            _SetProcessForegroundRights(pProcessHandle->_hProcess.get(), fForeground);
        }

        it = std::next(it);
    }

    // Do this for conhost.exe itself, too.
    _SetProcessForegroundRights(GetCurrentProcess(), fForeground);
}

bool ConsoleProcessList::IsEmpty() const
{
    return _processes.size() == 0;
}

void ConsoleProcessList::_SetProcessFocus(_In_ HANDLE const hProcess, _In_ bool const fForeground) const
{
    LOG_IF_WIN32_BOOL_FALSE(SetPriorityClass(hProcess,
                                             fForeground ? PROCESS_MODE_BACKGROUND_END : PROCESS_MODE_BACKGROUND_BEGIN));
}

void ConsoleProcessList::_SetProcessForegroundRights(_In_ HANDLE const hProcess, _In_ bool const fForeground) const
{
    CONSOLESETFOREGROUND Flags;
    Flags.hProcess = hProcess;
    Flags.bForeground = fForeground;

    LOG_IF_NTSTATUS_FAILED(UserPrivApi::s_ConsoleControl(UserPrivApi::CONSOLECONTROL::ConsoleSetForeground, 
                                                         &Flags, 
                                                         sizeof(Flags)));
}
