/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "WaitBlock.h"
#include "WaitQueue.h"

#include "..\host\globals.h"
#include "..\host\utils.hpp"

// Routine Description:
// - Initializes a ConsoleWaitBlock
// - ConsoleWaitBlocks will self-manage their position in their two queues.
// - They will push themselves into the head and store the iterator for constant deletion time later.
// Arguments:
// - pProcessQueue - The queue attached to the client process ID that requested this action
// - pObjectQueue - The queue attached to the console object that will service the action when data arrives
// - pfnWaitRoutine - The callback routine when attempting to service this wait later.
// - pvWaitContext - Context to send to callback routine when calling
// - pWaitReplyMessage - The original API message related to the client process's service request
ConsoleWaitBlock::ConsoleWaitBlock(_In_ ConsoleWaitQueue* const pProcessQueue,
                                   _In_ ConsoleWaitQueue* const pObjectQueue,
                                   _In_ ConsoleWaitRoutine const pfnWaitRoutine,
                                   _In_ PVOID const pvWaitContext,
                                   _In_ const CONSOLE_API_MSG* const pWaitReplyMessage) :
    _pProcessQueue(THROW_HR_IF_NULL(E_INVALIDARG, pProcessQueue)),
    _pObjectQueue(THROW_HR_IF_NULL(E_INVALIDARG, pObjectQueue)),
    _pfnWaitRoutine(THROW_HR_IF_NULL(E_INVALIDARG, pfnWaitRoutine)),
    _pvWaitContext(THROW_HR_IF_NULL(E_INVALIDARG, pvWaitContext))
{
    _pProcessQueue->_blocks.push_front(this);
    _itProcessQueue = _pProcessQueue->_blocks.cbegin();

    _pObjectQueue->_blocks.push_front(this);
    _itObjectQueue = _pObjectQueue->_blocks.cbegin();

    _WaitReplyMessage = *pWaitReplyMessage;

    if (pWaitReplyMessage->Complete.Write.Data != nullptr)
    {
        _WaitReplyMessage.Complete.Write.Data = &_WaitReplyMessage.u;
    }
}

// Routine Description:
// - Destroys a ConsolewaitBlock
// - On deletion, ConsoleWaitBlocks will erase themselves from the process and object queues in 
//   constant time with the iterator acquired on construction.
ConsoleWaitBlock::~ConsoleWaitBlock()
{
    _pProcessQueue->_blocks.erase(_itProcessQueue);
    _pObjectQueue->_blocks.erase(_itObjectQueue);
}

// Routine Description:
// - Creates and enqueues a new wait for later callback when a routine cannot be serviced at this time.
// - Will extract the process ID and the target object, enqueuing in both to know when to callback
// Arguments:
// - pWaitReplyMessage - The original API message from the client asking for servicing
// - pfnWaitRoutine - The callback routine to trigger when data becomes available at a later time
// - pvWaitContext - Parameter to pass to callback routine when it is triggered later.
// Return Value:
// - S_OK if queued and ready to go. Appropriate HRESULT value if it failed.
HRESULT ConsoleWaitBlock::s_CreateWait(_Inout_ CONSOLE_API_MSG* const pWaitReplyMessage,
                                       _In_ ConsoleWaitRoutine const pfnWaitRoutine,
                                       _In_ PVOID const pvWaitContext)
{
    HRESULT hr = S_OK;

    assert(g_ciConsoleInformation.IsConsoleLocked());

    ConsoleProcessHandle* const ProcessData = pWaitReplyMessage->GetProcessHandle();
    assert(ProcessData != nullptr);

    ConsoleWaitQueue* const pProcessQueue = ProcessData->pWaitBlockQueue.get();

    ConsoleHandleData* const pHandleData = pWaitReplyMessage->GetObjectHandle();
    assert(pHandleData != nullptr);

    ConsoleWaitQueue* pObjectQueue;
    pHandleData->GetWaitQueue(&pObjectQueue);
    assert(pObjectQueue != nullptr);

    ConsoleWaitBlock* pWaitBlock;
    try
    {
        pWaitBlock = new ConsoleWaitBlock(pProcessQueue,
                                          pObjectQueue,
                                          pfnWaitRoutine,
                                          pvWaitContext,
                                          pWaitReplyMessage);

        THROW_IF_NULL_ALLOC(pWaitBlock);
    }
    catch (...)
    {
        hr = wil::ResultFromCaughtException();
    }

    if (FAILED(hr))
    {
        pWaitReplyMessage->SetReplyStatus(NTSTATUS_FROM_HRESULT(hr));
    }

    return hr;
}

// Routine Description:
// - Used to trigger the callback routine inside this wait block.
// Arguments:
// - TerminationReason - A reason to tell the callback to terminate early or 0 if it should operate normally.
// Return Value:
// - True if the routine was able to successfully return data (or terminate). False otherwise.
bool ConsoleWaitBlock::Notify(_In_ WaitTerminationReason const TerminationReason)
{
    bool fRetVal;

    if ((_pfnWaitRoutine)(&_WaitReplyMessage, _pvWaitContext, TerminationReason))
    {
        _WaitReplyMessage.ReleaseMessageBuffers();

        LOG_IF_FAILED(g_pDeviceComm->CompleteIo(&_WaitReplyMessage.Complete));

        fRetVal = true;
    }
    else
    {
        // If fThreadDying is TRUE we need to make sure that we removed the pWaitBlock from the list (which we don't do on this branch).
        assert(IsFlagClear(TerminationReason, WaitTerminationReason::ThreadDying));
        fRetVal = false;
    }

    return fRetVal;
}
