/*++
Copyright (c) Microsoft Corporation

Module Name:
- WaitQueue.h

Abstract:
- This file manages a queue of wait blocks

Author:
- Michael Niksa (miniksa) 17-Oct-2016

Revision History:
- Adapted from original items in handle.h
--*/

#pragma once

#include <list>

#include "..\host\conapi.h"

#include "WaitBlock.h"
#include "WaitTerminationReason.h"

class ConsoleWaitQueue
{
public:
    ConsoleWaitQueue();

    ~ConsoleWaitQueue();

    bool NotifyWaiters(_In_ bool const fNotifyAll);

    bool NotifyWaiters(_In_ bool const fNotifyAll,
                       _In_ WaitTerminationReason const TerminationReason);

    static HRESULT s_CreateWait(_Inout_ CONSOLE_API_MSG* const pWaitReplyMessage,
                                _In_ ConsoleWaitRoutine const pfnWaitRoutine,
                                _In_ PVOID const pvWaitContext);

private:
    bool _NotifyBlock(_In_ ConsoleWaitBlock* pWaitBlock,
                      _In_ WaitTerminationReason const TerminationReason);

    std::list<ConsoleWaitBlock*> _blocks;

    friend class ConsoleWaitBlock; // Blocks live in multiple queues so we let them manage the lifetime.
};
