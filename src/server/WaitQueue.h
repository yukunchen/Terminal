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
    BOOL ConsoleNotifyWait(_In_ const BOOL fSatisfyAll);

    BOOL ConsoleNotifyWait(_In_ const BOOL fSatisfyAll,
                           _In_ WaitTerminationReason TerminationReason);

    BOOL ConsoleNotifyWaitBlock(_In_ ConsoleWaitBlock* pWaitBlock,
                                _In_ WaitTerminationReason TerminationReason,
                                _In_ BOOL fThreadDying);

    static BOOL s_ConsoleCreateWait(_In_ CONSOLE_WAIT_ROUTINE pfnWaitRoutine,
                                    _Inout_ PCONSOLE_API_MSG pWaitReplyMessage,
                                    _In_ PVOID pvWaitParameter);

    void FreeBlocks();

    ConsoleWaitQueue()
    {

    }

    ~ConsoleWaitQueue()
    {

    }

public:
    std::list<ConsoleWaitBlock*> _blocks;
};
