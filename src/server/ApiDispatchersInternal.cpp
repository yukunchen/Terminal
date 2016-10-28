/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "ApiDispatchers.h"

#include "..\host\globals.h"
#include "..\host\handle.h"
#include "..\host\server.h"
#include "..\host\telemetry.hpp"

HRESULT ApiDispatchers::ServerDeprecatedApi(_Inout_ CONSOLE_API_MSG * const /*m*/, _Inout_ BOOL* const /*pbReplyPending*/)
{
    // assert if we hit a deprecated API.
    assert(false);

    return E_NOTIMPL;
}

HRESULT ApiDispatchers::ServerGetConsoleProcessList(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const /*pbReplyPending*/)
{
    PCONSOLE_GETCONSOLEPROCESSLIST_MSG const a = &m->u.consoleMsgL3.GetConsoleProcessList;
    Telemetry::Instance().LogApiCall(Telemetry::ApiCall::GetConsoleProcessList);

    PVOID Buffer;
    ULONG BufferSize;
    RETURN_IF_FAILED(m->GetOutputBuffer(&Buffer, &BufferSize));

    a->dwProcessCount = BufferSize / sizeof(ULONG);

    LockConsole();
    auto Unlock = wil::ScopeExit([&] { UnlockConsole(); });

    /*
    * If there's not enough space in the array to hold all the pids, we'll
    * inform the user of that by returning a number > than a->dwProcessCount
    * (but we still return S_OK).
    */

    LPDWORD lpdwProcessList = (PDWORD)Buffer;
    size_t cProcessList = a->dwProcessCount;
    if (SUCCEEDED(g_ciConsoleInformation.ProcessHandleList.GetProcessList(lpdwProcessList, &cProcessList)))
    {
        m->SetReplyInformation(cProcessList * sizeof(ULONG));
    }

    a->dwProcessCount = (ULONG)cProcessList;

    return S_OK;
}

HRESULT ApiDispatchers::ServerGetConsoleLangId(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const /*pbReplyPending*/)
{
    CONSOLE_LANGID_MSG* const a = &m->u.consoleMsgL1.GetConsoleLangId;
    Telemetry::Instance().LogApiCall(Telemetry::ApiCall::GetConsoleLangId);

    // TODO: MSFT: 9115192 - This should probably just ask through GetOutputCP and convert it ourselves on this side.
    return m->_pApiRoutines->GetConsoleLangIdImpl(&a->LangId);
}

NTSTATUS GetProcessParentId(_Inout_ PULONG ProcessId)
{
    // TODO: Get Parent current not really available without winternl + NtQueryInformationProcess. http://osgvsowi/8394495
    // TODO: This is being fixed in bugfix 9535136.

    //OBJECT_ATTRIBUTES oa;
    //InitializeObjectAttributes(&oa, nullptr, 0, nullptr, nullptr);

    //CLIENT_ID ClientId;
    //ClientId.UniqueProcess = UlongToHandle(*ProcessId);
    //ClientId.UniqueThread = 0;

    //HANDLE ProcessHandle;
    //NTSTATUS Status = NtOpenProcess(&ProcessHandle, PROCESS_QUERY_LIMITED_INFORMATION, &oa, &ClientId);

    //PROCESS_BASIC_INFORMATION BasicInfo = { 0 };
    //if (NT_SUCCESS(Status))
    //{
    //Status = NtQueryInformationProcess(ProcessHandle, ProcessBasicInformation, &BasicInfo, sizeof(BasicInfo), nullptr);
    //NtClose(ProcessHandle);
    //}

    //if (!NT_SUCCESS(Status))
    //{
    //*ProcessId = 0;
    //return Status;
    //}

    //*ProcessId = (ULONG) BasicInfo.InheritedFromUniqueProcessId;
    //return STATUS_SUCCESS;

    *ProcessId = 0;
    return STATUS_UNSUCCESSFUL;
}

HRESULT ApiDispatchers::ServerGenerateConsoleCtrlEvent(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const /*pbReplyPending*/)
{
    CONSOLE_CTRLEVENT_MSG* const a = &m->u.consoleMsgL2.GenerateConsoleCtrlEvent;
    Telemetry::Instance().LogApiCall(Telemetry::ApiCall::GenerateConsoleCtrlEvent);

    LockConsole();
    auto Unlock = wil::ScopeExit([&] { UnlockConsole(); });

    // Make sure the process group id is valid.
    if (a->ProcessGroupId != 0)
    {
        ConsoleProcessHandle* ProcessHandle;
        ProcessHandle = g_ciConsoleInformation.ProcessHandleList.FindProcessByGroupId(a->ProcessGroupId);
        if (ProcessHandle == nullptr)
        {
            ULONG ProcessId = a->ProcessGroupId;

            // We didn't find a process with that group ID.
            // Let's see if the process with that ID exists and has a parent that is a member of this console.
            RETURN_IF_NTSTATUS_FAILED((GetProcessParentId(&ProcessId)));
            ProcessHandle = g_ciConsoleInformation.ProcessHandleList.FindProcessInList(ProcessId);
            RETURN_HR_IF_NULL(E_INVALIDARG, ProcessHandle);
            RETURN_IF_FAILED(g_ciConsoleInformation.ProcessHandleList.AllocProcessData(a->ProcessGroupId,
                                                                                       0,
                                                                                       a->ProcessGroupId,
                                                                                       ProcessHandle,
                                                                                       nullptr));
        }
    }

    g_ciConsoleInformation.LimitingProcessId = a->ProcessGroupId;
    HandleCtrlEvent(a->CtrlEvent);

    return S_OK;
}
