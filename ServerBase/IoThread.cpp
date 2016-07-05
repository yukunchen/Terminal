#include "stdafx.h"

#include "IoThread.h"
#include "IoDispatchers.h"

#include "ApiMessage.h"

#include "..\ServerBaseApi\IApiResponders.h"

IoThread::IoThread(_In_ HANDLE Server, _In_ IApiResponders* const pResponder) :
    _Thread(s_IoLoop, this),
    _pResponder(pResponder),
    _Comm(Server),
    _Prot(&_Comm),
    _InputEvent(CreateEventW(nullptr, TRUE, FALSE, nullptr))
{
    THROW_IF_NTSTATUS_FAILED(_Prot.SetInputAvailableEvent(_InputEvent.get()));

    // TODO: is this necessary? there's some weird thread handling going on here...
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
    bool fExiting = false;

    IoDispatchers Dispatcher(&_Prot, _pResponder);

    while (!fExiting)
    {
        // Attempt to read API call from wire
        CONSOLE_API_MSG ReceiveMsg;
        HRESULT Result = _Prot.GetApiCall(&ReceiveMsg);

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
        _Prot.GetInputBuffer(&ReceiveMsg);
        _Prot.GetOutputBuffer(&ReceiveMsg);

        // Service the IO request.
        Dispatcher.ServiceIoOperation(&ReceiveMsg);
    }

    ExitProcess(STATUS_SUCCESS);
}

