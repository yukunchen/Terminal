/*++
Copyright (c) Microsoft Corporation

Module Name:
- WaitBlock.h

Abstract:
- This file defines a queued operation when a console buffer object cannot currently satisfy the request.

Author:
- Michael Niksa (miniksa) 12-Oct-2016

Revision History:
- Adapted from original items in handle.h
--*/

#pragma once

#include "..\host\conapi.h"

#include "WaitTerminationReason.h"

class ConsoleWaitQueue;

typedef BOOL(*CONSOLE_WAIT_ROUTINE) (_In_ PCONSOLE_API_MSG pWaitReplyMessage,
                                     _In_ PVOID pvWaitParameter,
                                     _In_ WaitTerminationReason TerminationReason,
                                     _In_ BOOL fThreadDying);

class ConsoleWaitBlock
{
public:
    PVOID WaitParameter;
    CONSOLE_WAIT_ROUTINE WaitRoutine;
    CONSOLE_API_MSG WaitReplyMessage;

    ConsoleWaitBlock(_In_ ConsoleWaitQueue* const pProcessQueue,
                        _In_ ConsoleWaitQueue* const pObjectQueue);
    ~ConsoleWaitBlock();
    

private:
    ConsoleWaitQueue* const _pProcessQueue;
    std::_List_const_iterator<std::_List_val<std::_List_simple_types<ConsoleWaitBlock*>>> _itProcessQueue;

    ConsoleWaitQueue* const _pObjectQueue;
    std::_List_const_iterator<std::_List_val<std::_List_simple_types<ConsoleWaitBlock*>>> _itObjectQueue;

};
