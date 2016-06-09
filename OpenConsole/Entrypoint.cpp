#include "stdafx.h"
#include "Entrypoint.h"

using namespace std;

void SetReplyStatus(_Inout_ CONSOLE_API_MSG* const pMessage, _In_ const NTSTATUS Status)
{
    pMessage->Complete.IoStatus.Status = Status;
}

void SetReplyInformation(_Inout_ CONSOLE_API_MSG* const pMessage, _In_ ULONG_PTR pData)
{
    pMessage->Complete.IoStatus.Information = pData;
}

void DoConnect(_In_ HANDLE Server, _Inout_ CONSOLE_API_MSG* const pMsg, _Out_ CONSOLE_API_MSG* const pReply)
{

    HANDLE InputEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    CD_IO_SERVER_INFORMATION ServerInfo;
    ServerInfo.InputAvailableEvent = InputEvent;

    DWORD Written = 0;

    DeviceIoControl(Server,
                    IOCTL_CONDRV_SET_SERVER_INFORMATION,
                    &ServerInfo,
                    sizeof(ServerInfo),
                    nullptr,
                    0,
                    &Written,
                    nullptr);

    // ----

    SetReplyStatus(pMsg, STATUS_SUCCESS);
    SetReplyInformation(pMsg, sizeof(CD_CONNECTION_INFORMATION));

    CD_CONNECTION_INFORMATION ConnectionInfo;
    pMsg->Complete.Write.Data = &ConnectionInfo;
    pMsg->Complete.Write.Size = sizeof(CD_CONNECTION_INFORMATION);

    // TODO: These are junk handles for now to various info that we want to get back when other calls come in.
    ConnectionInfo.Process = 0x14;
    ConnectionInfo.Input = 0x18;
    ConnectionInfo.Output = 0x1a;

    Written = 0;

    BOOL Succeeded = DeviceIoControl(Server,
                                     IOCTL_CONDRV_COMPLETE_IO,
                                     &pMsg->Complete,
                                     sizeof(pMsg->Complete),
                                     nullptr,
                                     0,
                                     &Written,
                                     nullptr);

    if (FALSE == Succeeded)
    {
        DWORD Error = GetLastError();

        if (0 != Error)
        {
            DebugBreak();
        }
    }
}

void IoLoop(_In_ HANDLE Server)
{
    bool fExiting = false;
    CONSOLE_API_MSG* pReplyMsg = nullptr;
    CONSOLE_API_MSG ReceiveMsg;

    while (!fExiting)
    {
        DWORD Written = 0;

        //DebugBreak();

        BOOL Succeeded = DeviceIoControl(Server,
                                         IOCTL_CONDRV_READ_IO,
                                         pReplyMsg == nullptr ? nullptr : &pReplyMsg->Complete,
                                         pReplyMsg == nullptr ? 0 : sizeof(pReplyMsg->Complete),
                                         &ReceiveMsg.Descriptor,
                                         sizeof(CONSOLE_API_MSG) - FIELD_OFFSET(CONSOLE_API_MSG, Descriptor),
                                         &Written,
                                         nullptr);

        DWORD Error = 0;
        if (FALSE == Succeeded)
        {
            Error = GetLastError();
        }

        // TODO: Wait for pending

        if (ERROR_PIPE_NOT_CONNECTED == Error)
        {
            fExiting = true;
            continue;
        }
        else if (S_OK != Error)
        {
            DebugBreak();
        }

        // Clear out state and completion status.
        ZeroMemory(&ReceiveMsg.State, sizeof(ReceiveMsg.State));
        ZeroMemory(&ReceiveMsg.Complete, sizeof(ReceiveMsg.Complete));

        // Mark completion identifier with the same code as the received message
        ReceiveMsg.Complete.Identifier = ReceiveMsg.Descriptor.Identifier;

        // Route function to handler
        switch (ReceiveMsg.Descriptor.Function)
        {
        case CONSOLE_IO_CONNECT:
        {
            DoConnect(Server, &ReceiveMsg, pReplyMsg);
        }
        default:
        {
            SetReplyStatus(&ReceiveMsg, STATUS_SUCCESS);
            pReplyMsg = &ReceiveMsg;
            break;
        }
        }

    }

    ExitProcess(STATUS_SUCCESS);
}

extern "C"
{
    _declspec(dllexport)
        NTSTATUS ConsoleCreateIoThread(_In_ HANDLE Server)
    {
        thread ioThread(IoLoop, Server);
        ioThread.detach();

        return STATUS_SUCCESS;
    }
}

