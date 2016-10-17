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

class ConsoleWaitQueue;

typedef BOOL(*CONSOLE_WAIT_ROUTINE) (_In_ PCONSOLE_API_MSG pWaitReplyMessage,
                                     _In_ PVOID pvWaitParameter,
                                     _In_ PVOID pvSatisfyParameter,
                                     _In_ BOOL fThreadDying);

typedef struct _CONSOLE_WAIT_BLOCK
{
public:
    PVOID WaitParameter;
    CONSOLE_WAIT_ROUTINE WaitRoutine;
    CONSOLE_API_MSG WaitReplyMessage;

    _CONSOLE_WAIT_BLOCK(_In_ ConsoleWaitQueue* const pProcessQueue,
                        _In_ ConsoleWaitQueue* const pObjectQueue);
    ~_CONSOLE_WAIT_BLOCK();
    

private:
    ConsoleWaitQueue* const _pProcessQueue;
    std::_List_const_iterator<std::_List_val<std::_List_simple_types<_CONSOLE_WAIT_BLOCK*>>> _itProcessQueue;

    ConsoleWaitQueue* const _pObjectQueue;
    std::_List_const_iterator<std::_List_val<std::_List_simple_types<_CONSOLE_WAIT_BLOCK*>>> _itObjectQueue;

} CONSOLE_WAIT_BLOCK, *PCONSOLE_WAIT_BLOCK;
