#include "stdafx.h"
#include "Entrypoint.h"

using namespace std;

void SetReplyStatus(_Inout_ CONSOLE_API_MSG* const pMessage, _In_ const NTSTATUS Status)
{
    pMessage->Complete.IoStatus.Status = Status;
}

void IoLoop(_In_ HANDLE Server)
{
    bool fExiting = false;
    CONSOLE_API_MSG* pReplyMsg = nullptr;
    CONSOLE_API_MSG ReceiveMsg;

    while (!fExiting)
    {
        DWORD Written = 0;

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

        // TODO: Detect disconnect and exit loop
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

