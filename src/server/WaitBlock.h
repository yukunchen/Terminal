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

#include <list>

class ConsoleWaitQueue;

typedef BOOL(*ConsoleWaitRoutine) (_In_ PCONSOLE_API_MSG pWaitReplyMessage,
                                   _In_ PVOID pvWaitContext,
                                   _In_ WaitTerminationReason TerminationReason);

class ConsoleWaitBlock
{
public:

    ~ConsoleWaitBlock();

    bool Notify(_In_ WaitTerminationReason const TerminationReason);

    static HRESULT s_CreateWait(_Inout_ CONSOLE_API_MSG* const pWaitReplyMessage,
                                _In_ ConsoleWaitRoutine const pfnWaitRoutine,
                                _In_ PVOID const pvWaitContext);


private:
    ConsoleWaitBlock(_In_ ConsoleWaitQueue* const pProcessQueue,
                     _In_ ConsoleWaitQueue* const pObjectQueue,
                     _In_ ConsoleWaitRoutine const pfnWaitRoutine,
                     _In_ PVOID const pvWaitContext,
                     _In_ const CONSOLE_API_MSG* const pWaitReplyMessage);

    ConsoleWaitQueue* const _pProcessQueue;
    std::_List_const_iterator<std::_List_val<std::_List_simple_types<ConsoleWaitBlock*>>> _itProcessQueue;

    ConsoleWaitQueue* const _pObjectQueue;
    std::_List_const_iterator<std::_List_val<std::_List_simple_types<ConsoleWaitBlock*>>> _itObjectQueue;

    PVOID const _pvWaitContext;
    ConsoleWaitRoutine const _pfnWaitRoutine;
    CONSOLE_API_MSG _WaitReplyMessage;
};
