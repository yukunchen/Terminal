/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "WaitQueue.h"
#include "WaitBlock.h"

#include "..\host\globals.h"
#include "..\host\utils.hpp"

BOOL ConsoleWaitQueue::ConsoleNotifyWait(_In_ const BOOL fSatisfyAll)
{
    return ConsoleNotifyWait(fSatisfyAll, WaitTerminationReason::NoReason);
}

BOOL ConsoleWaitQueue::ConsoleNotifyWait(_In_ const BOOL fSatisfyAll, 
                                         _In_ WaitTerminationReason TerminationReason)
{
    BOOL Result = FALSE;

    for (auto it = _blocks.cbegin(); it != _blocks.cend();)
    {
        ConsoleWaitBlock* const WaitBlock = (*it);
        auto nextIt = std::next(it); // we have to capture next before it is potentially erased
        if (WaitBlock->WaitRoutine)
        {
            Result |= ConsoleNotifyWaitBlock(WaitBlock, TerminationReason);
            if (!fSatisfyAll)
            {
                break;
            }
        }
        it = nextIt;
    }

    return Result;
}

BOOL ConsoleWaitQueue::ConsoleNotifyWaitBlock(_In_ ConsoleWaitBlock* pWaitBlock, 
                                              _In_ WaitTerminationReason TerminationReason)
{
    if ((*pWaitBlock->WaitRoutine)(&pWaitBlock->WaitReplyMessage, pWaitBlock->WaitParameter, TerminationReason))
    {
        ReleaseMessageBuffers(&pWaitBlock->WaitReplyMessage);

        LOG_IF_FAILED(g_pDeviceComm->CompleteIo(&pWaitBlock->WaitReplyMessage.Complete));

        delete pWaitBlock;

        return TRUE;
    }
    else
    {
        // If fThreadDying is TRUE we need to make sure that we removed the pWaitBlock from the list (which we don't do on this branch).
        assert(IsFlagClear(TerminationReason, WaitTerminationReason::ThreadDying));
        return FALSE;
    }
}

_Success_(return == TRUE)
static BOOL ConsoleInitializeWait(_In_ CONSOLE_WAIT_ROUTINE pfnWaitRoutine,
                                  _Inout_ PCONSOLE_API_MSG pWaitReplyMessage,
                                  _In_ PVOID pvWaitParameter,
                                  _Outptr_ ConsoleWaitBlock** ppWaitBlock)
{
    PCONSOLE_PROCESS_HANDLE const ProcessData = GetMessageProcess(pWaitReplyMessage);
    assert(ProcessData != nullptr);

    ConsoleWaitQueue* pProcessQueue = &ProcessData->WaitBlockQueue;

    ConsoleHandleData* const pHandleData = GetMessageObject(pWaitReplyMessage);
    assert(pHandleData != nullptr);

    ConsoleWaitQueue* pObjectQueue;
    pHandleData->GetWaitQueue(&pObjectQueue);
    assert(pObjectQueue != nullptr);

    ConsoleWaitBlock* WaitBlock;
    try
    {
        WaitBlock = new ConsoleWaitBlock(pProcessQueue, pObjectQueue);
    }
    catch(...)
    {
        HRESULT hr = wil::ResultFromCaughtException();
        SetReplyStatus(pWaitReplyMessage, NTSTATUS_FROM_HRESULT(hr));
        return FALSE;
    }
    
    if (WaitBlock == nullptr)
    {
        SetReplyStatus(pWaitReplyMessage, STATUS_NO_MEMORY);
        return FALSE;
    }

    WaitBlock->WaitParameter = pvWaitParameter;
    WaitBlock->WaitRoutine = pfnWaitRoutine;
    memmove(&WaitBlock->WaitReplyMessage, pWaitReplyMessage, sizeof(CONSOLE_API_MSG));

    if (pWaitReplyMessage->Complete.Write.Data != nullptr)
    {
        WaitBlock->WaitReplyMessage.Complete.Write.Data = &WaitBlock->WaitReplyMessage.u;
    }

    *ppWaitBlock = WaitBlock;

    return TRUE;
}

BOOL ConsoleWaitQueue::s_ConsoleCreateWait(_In_ CONSOLE_WAIT_ROUTINE pfnWaitRoutine,
                                           _Inout_ PCONSOLE_API_MSG pWaitReplyMessage,
                                           _In_ PVOID pvWaitParameter)
{
    assert(g_ciConsoleInformation.IsConsoleLocked());

    ConsoleWaitBlock* WaitBlock;
    if (!ConsoleInitializeWait(pfnWaitRoutine, pWaitReplyMessage, pvWaitParameter, &WaitBlock))
    {
        return FALSE;
    }

    return TRUE;
}

void ConsoleWaitQueue::FreeBlocks()
{
    for (auto it = _blocks.cbegin(); it != _blocks.cend(); std::next(it))
    {
        ConsoleNotifyWaitBlock(*it, WaitTerminationReason::ThreadDying);
    }
}
