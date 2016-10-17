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

class ConsoleWaitQueue
{
public:
    BOOL ConsoleNotifyWait(_In_ const BOOL fSatisfyAll,
                           _In_ PVOID pvSatisfyParameter);

    BOOL ConsoleNotifyWaitBlock(_In_ PCONSOLE_WAIT_BLOCK pWaitBlock,
                                _In_ PVOID pvSatisfyParameter,
                                _In_ BOOL fThreadDying);

    static BOOL s_ConsoleCreateWait(_In_ CONSOLE_WAIT_ROUTINE pfnWaitRoutine,
                                    _Inout_ PCONSOLE_API_MSG pWaitReplyMessage,
                                    _In_ PVOID pvWaitParameter);

    void FreeBlocks();

public:
    std::list<_CONSOLE_WAIT_BLOCK*> _blocks;

    void Foo()
    {

    }
};
