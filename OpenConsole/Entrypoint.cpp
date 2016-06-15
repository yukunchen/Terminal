#include "stdafx.h"
#include "Entrypoint.h"
#include "DeviceComm.h"
#include "DeviceProtocol.h"

#include "IApiResponders.h"
#include "ApiResponderEmpty.h"
#include "ApiSorter.h"

using namespace std;

DWORD DoConnect(_In_ DeviceProtocol* Server, _In_ CD_IO_DESCRIPTOR* const pMsg)
{
    HANDLE InputEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    DWORD result = Server->SetInputAvailableEvent(InputEvent);
    
    // TODO: These are junk handles for now to various info that we want to get back when other calls come in.
    result = Server->SetConnectionInformation(pMsg, 0x14, 0x18, 0x1a);

    return result;
}

DWORD DoDefault(_In_ DeviceProtocol* Server, _In_ CD_IO_DESCRIPTOR* const pMsg)
{
    return Server->SendCompletion(pMsg, STATUS_UNSUCCESSFUL, nullptr, 0);
}

void IoLoop(_In_ HANDLE Server)
{
    DeviceComm Comm(Server);
    DeviceProtocol Prot(&Comm);
    ApiResponderEmpty Responder;

    bool fExiting = false;

    while (!fExiting)
    {
        CONSOLE_API_MSG ReceiveMsg;
        DWORD Error = Prot.GetReadIo(&ReceiveMsg);

        if (ERROR_PIPE_NOT_CONNECTED == Error)
        {
            fExiting = true;
            continue;
        }
        else if (S_OK != Error)
        {
            DebugBreak();
        }

        // Route function to handler
        switch (ReceiveMsg.Descriptor.Function)
        {
        case CONSOLE_IO_USER_DEFINED:
        {
            DoApiCall(&Prot, &Responder, &ReceiveMsg);
            break;
        }
        case CONSOLE_IO_CONNECT:
        {
            DoConnect(&Prot, &ReceiveMsg.Descriptor);
            break;
        }
        default:
        {
            DoDefault(&Prot, &ReceiveMsg.Descriptor);
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
