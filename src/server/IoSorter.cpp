/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "IoSorter.h"

#include "IoDispatchers.h"
#include "ApiDispatchers.h"

#include "..\host\globals.h"

#include "..\host\getset.h"
#include "..\host\stream.h"

void IoSorter::ServiceIoOperation(_In_ CONSOLE_API_MSG* const pMsg,
                                  _Out_ CONSOLE_API_MSG** ReplyMsg)
{
    NTSTATUS Status;
    BOOL ReplyPending = FALSE;

    ZeroMemory(&pMsg->State, sizeof(pMsg->State));
    ZeroMemory(&pMsg->Complete, sizeof(CD_IO_COMPLETE));

    pMsg->Complete.Identifier = pMsg->Descriptor.Identifier;

    switch (pMsg->Descriptor.Function)
    {
    case CONSOLE_IO_USER_DEFINED:
        *ReplyMsg = IoDispatchers::ConsoleDispatchRequest(pMsg);
        break;

    case CONSOLE_IO_CONNECT:
        *ReplyMsg = IoDispatchers::ConsoleHandleConnectionRequest(pMsg);
        break;

    case CONSOLE_IO_DISCONNECT:
        IoDispatchers::ConsoleClientDisconnectRoutine(pMsg->GetProcessHandle());
        pMsg->SetReplyStatus(STATUS_SUCCESS);
        *ReplyMsg = pMsg;
        break;

    case CONSOLE_IO_CREATE_OBJECT:
        *ReplyMsg = IoDispatchers::ConsoleCreateObject(pMsg);
        break;

    case CONSOLE_IO_CLOSE_OBJECT:
        SrvCloseHandle(pMsg);
        pMsg->SetReplyStatus(STATUS_SUCCESS);
        *ReplyMsg = pMsg;
        break;

    case CONSOLE_IO_RAW_WRITE:
        ZeroMemory(&pMsg->u.consoleMsgL1.WriteConsole, sizeof(CONSOLE_WRITECONSOLE_MSG));

        ReplyPending = FALSE;
        Status = SrvWriteConsole(pMsg, &ReplyPending);
        if (ReplyPending)
        {
            *ReplyMsg = nullptr;

        }
        else
        {
            pMsg->SetReplyStatus(Status);
            *ReplyMsg = pMsg;
        }
        break;

    case CONSOLE_IO_RAW_READ:
        ZeroMemory(&pMsg->u.consoleMsgL1.ReadConsole, sizeof(CONSOLE_READCONSOLE_MSG));
        pMsg->u.consoleMsgL1.ReadConsole.ProcessControlZ = TRUE;
        ReplyPending = FALSE;
        Status = SrvReadConsole(pMsg, &ReplyPending);
        if (ReplyPending)
        {
            *ReplyMsg = nullptr;

        }
        else
        {
            pMsg->SetReplyStatus(Status);
            *ReplyMsg = pMsg;
        }
        break;

    case CONSOLE_IO_RAW_FLUSH:
        ReplyPending = FALSE;
        
        Status = NTSTATUS_FROM_HRESULT(ApiDispatchers::ServerFlushConsoleInputBuffer(pMsg, &ReplyPending));
        assert(!ReplyPending);
        pMsg->SetReplyStatus(Status);
        *ReplyMsg = pMsg;
        break;

    default:
        pMsg->SetReplyStatus(STATUS_UNSUCCESSFUL);
        *ReplyMsg = pMsg;
    }
}
