#include "stdafx.h"

#include "ApiMessage.h"

#include "IoThread.h"
#include "IoDispatchers.h"

#include "DeviceComm.h"
#include "DeviceProtocol.h"

#include "..\ServerBaseApi\IApiResponders.h"

IoThread::IoThread(_In_ HANDLE Server, _In_ IApiResponders* const pResponder) :
    _Server(Server),
    _Thread(s_IoLoop, this),
    _pResponder(pResponder)
{
    _Thread.detach();
}

IoThread::~IoThread()
{
    // detached threads free when they stop running.
}

void IoThread::s_IoLoop(_In_ IoThread* const pIoThread)
{
    pIoThread->IoLoop();
}

void IoThread::IoLoop()
{
    DeviceComm Comm(_Server);
    DeviceProtocol Prot(&Comm);
    IoDispatchers Dispatcher(&Prot, _pResponder);

    bool fExiting = false;

    while (!fExiting)
    {
        // Attempt to read API call from wire
        CONSOLE_API_MSG ReceiveMsg;
        DWORD Result = Prot.GetApiCall(&ReceiveMsg);

        // If we're disconnected or something goes wrong, bail
        if (ERROR_PIPE_NOT_CONNECTED == Result)
        {
            fExiting = true;
            continue;
        }
        else if (S_OK != Result)
        {
            DebugBreak();
        }

        // Retrieve additional input/output buffer data if available
        // TODO: determine what to do with errors from here.
        Prot.GetInputBuffer(&ReceiveMsg);
        Prot.GetOutputBuffer(&ReceiveMsg);

        // Service the IO request.
        Dispatcher.ServiceIoOperation(&ReceiveMsg);
    }

    ExitProcess(STATUS_SUCCESS);
}

