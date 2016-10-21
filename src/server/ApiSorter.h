/*++
Copyright (c) Microsoft Corporation

Module Name:
- IoSorter.h

Abstract:
- This file sorts out the various console host serviceable APIs.

Author:
- Michael Niksa (miniksa) 12-Oct-2016

Revision History:
- Adapted from original items in srvinit.cpp 
--*/

#pragma once

#include "ApiMessage.h"

typedef NTSTATUS(*PCONSOLE_API_ROUTINE) (_Inout_ PCONSOLE_API_MSG m, _Inout_ PBOOL ReplyPending);

class ApiSorter
{
public:
    // Routine Description:
    // - This routine validates a user IO and dispatches it to the appropriate worker routine.
    // Arguments:
    // - Message - Supplies the message representing the user IO.
    // Return Value:
    // - A pointer to the reply message, if this message is to be completed inline; nullptr if this message will pend now and complete later.
    static PCONSOLE_API_MSG ConsoleDispatchRequest(_Inout_ PCONSOLE_API_MSG Message);
};