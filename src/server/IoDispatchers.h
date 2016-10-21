/*++
Copyright (c) Microsoft Corporation

Module Name:
- IoDispatchers.h

Abstract:
- This file processes a majority of server-contained IO operations received from a client

Author:
- Michael Niksa (miniksa) 12-Oct-2016

Revision History:
- Adapted from original items in srvinit.cpp 
--*/

#pragma once

#include "ApiMessage.h"

class IoDispatchers
{
public:
    // TODO: temp for now. going to ApiSorter and IoDispatchers
    static PCONSOLE_API_MSG ConsoleHandleConnectionRequest(_Inout_ PCONSOLE_API_MSG ReceiveMsg);
    static PCONSOLE_API_MSG ConsoleDispatchRequest(_Inout_ PCONSOLE_API_MSG Message);
    static PCONSOLE_API_MSG ConsoleCreateObject(_In_ PCONSOLE_API_MSG Message);
    static VOID ConsoleClientDisconnectRoutine(ConsoleProcessHandle* ProcessData);
};