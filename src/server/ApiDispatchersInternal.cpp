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

#include "..\host\ntprivapi.hpp"

#include "..\interactivity\inc\ServiceLocator.hpp"

HRESULT ApiDispatchers::ServerDeprecatedApi(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const /*pbReplyPending*/)
{
    // log if we hit a deprecated API.
    RETURN_HR_MSG(E_NOTIMPL, "Deprecated API attempted: 0x%08x", m->Descriptor.Function);
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
    if (SUCCEEDED(ServiceLocator::LocateGlobals()->getConsoleInformation()->ProcessHandleList.GetProcessList(lpdwProcessList, &cProcessList)))
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
        ProcessHandle = ServiceLocator::LocateGlobals()->getConsoleInformation()->ProcessHandleList.FindProcessByGroupId(a->ProcessGroupId);
        if (ProcessHandle == nullptr)
        {
            ULONG ProcessId = a->ProcessGroupId;

            // We didn't find a process with that group ID.
            // Let's see if the process with that ID exists and has a parent that is a member of this console.
            RETURN_IF_NTSTATUS_FAILED((NtPrivApi::s_GetProcessParentId(&ProcessId)));
            ProcessHandle = ServiceLocator::LocateGlobals()->getConsoleInformation()->ProcessHandleList.FindProcessInList(ProcessId);
            RETURN_HR_IF_NULL(E_INVALIDARG, ProcessHandle);
            RETURN_IF_FAILED(ServiceLocator::LocateGlobals()->getConsoleInformation()->ProcessHandleList.AllocProcessData(a->ProcessGroupId,
                                                                                       0,
                                                                                       a->ProcessGroupId,
                                                                                       ProcessHandle,
                                                                                       nullptr));
        }
    }

    ServiceLocator::LocateGlobals()->getConsoleInformation()->LimitingProcessId = a->ProcessGroupId;
    HandleCtrlEvent(a->CtrlEvent);

    return S_OK;
}
