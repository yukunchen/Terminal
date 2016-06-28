#include "stdafx.h"

#include "ApiMessage.h"
#include "ObjectHandle.h"

#include "IoThread.h"

#include "DeviceComm.h"
#include "DeviceProtocol.h"

#include "ProcessHandle.h"

#include "IApiResponders.h"
#include "ApiResponderEmpty.h"
#include "ApiSorter.h"
#include "Win32Control.h"

IoThread::IoThread(_In_ HANDLE Server) :
    _Server(Server),
    _Thread(s_IoLoop, this)
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
    ApiResponderEmpty Responder;

    bool fExiting = false;

    while (!fExiting)
    {
        // Attempt to read API call from wire
        CONSOLE_API_MSG ReceiveMsg;
        DWORD Result = Prot.GetApiCall(&ReceiveMsg);

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

        // Route function to handler
        switch (ReceiveMsg.Descriptor.Function)
        {
        case CONSOLE_IO_USER_DEFINED:
        {
            Result = LookupAndDoApiCall(&Responder, &ReceiveMsg);
            break;
        }
        case CONSOLE_IO_CONNECT:
        {
            Result = _IoConnect(&Responder, &Prot, &ReceiveMsg);
            break;
        }
        case CONSOLE_IO_DISCONNECT:
        {
            Result = _IoDisconnect(&ReceiveMsg);
            break;
        }
        case CONSOLE_IO_CREATE_OBJECT:
        {
            // TODO.
            Result = _IoDefault();
            break;
        }
        case CONSOLE_IO_CLOSE_OBJECT:
        {
            // TODO.
            Result = _IoDefault();
            break;
        }
        case CONSOLE_IO_RAW_WRITE:
        {
            // TODO.
            Result = _IoDefault();
            break;
        }
        case CONSOLE_IO_RAW_READ:
        {
            Result = DoRawReadCall(&Responder, &ReceiveMsg);
            break;
        }
        case CONSOLE_IO_RAW_FLUSH:
        {
            // TODO.
            Result = _IoDefault();
            break;
        }
        default:
        {
            Result = _IoDefault();
            break;
        }
        }

        // Return whatever status code we got back to the caller.
        ReceiveMsg.SetCompletionStatus(Result);

        // Write reply message signaling API call is complete.
        Prot.CompleteApiCall(&ReceiveMsg);
    }

    ExitProcess(STATUS_SUCCESS);
}

DWORD IoThread::_IoDisconnect(_In_ CONSOLE_API_MSG* const pMsg)
{
    // FreeConsole called.

    // NotifyWinEvent

    delete pMsg->GetProcessHandle();

    return STATUS_SUCCESS;
}

DWORD IoThread::_IoConnect(_In_ IApiResponders* const pResponder, _In_ DeviceProtocol* Server, _In_ CONSOLE_API_MSG* const pMsg)
{
    DWORD Result = 0;

    // Retrieve connection payload
    CONSOLE_SERVER_MSG* pServerData;
    ULONG ServerDataSize;
    pMsg->GetInputBuffer<CONSOLE_SERVER_MSG>(&pServerData, &ServerDataSize);

    // Input setup
    // TODO: This handle is currently leaked.
    HANDLE InputEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    Result = Server->SetInputAvailableEvent(InputEvent);

    if (SUCCEEDED(Result))
    {
        // Handle setup
        IConsoleInputObject* pInputObj;
        IConsoleOutputObject* pOutputObj;
        Result = pResponder->CreateInitialObjects(&pInputObj, &pOutputObj);

        if (SUCCEEDED(Result))
        {
            ConsoleHandleData* pInputHandle;
            ConsoleHandleData* pOutputHandle;

            Result = ConsoleHandleData::CreateHandle(pInputObj, &pInputHandle);
            Result = ConsoleHandleData::CreateHandle(pOutputObj, &pOutputHandle);

            ConsoleProcessHandle* pProcessHandle = new ConsoleProcessHandle((DWORD)pMsg->Descriptor.Process,
                                                                            (DWORD)pMsg->Descriptor.Object,
                                                                            pServerData->ProcessGroupId,
                                                                            pInputHandle,
                                                                            pOutputHandle);

            if (SUCCEEDED(Result))
            {
                Result = Server->SetConnectionInformation(pMsg,
                                                          (ULONG_PTR)(pProcessHandle),
                                                          (ULONG_PTR)(pInputHandle),
                                                          (ULONG_PTR)(pOutputHandle));
            }
        }
    }

    // Notify app type
    if (SUCCEEDED(Result))
    {
        // Notify Win32k that this process is attached to a special console application type.
        // Don't do this for AttachConsole case (they already are a Win32 app type.)
        // ConsoleApp == FALSE means AttachConsole instead of the initial create.
        if (pServerData->ConsoleApp)
        {
            Win32Control::NotifyConsoleTypeApplication((HANDLE)pMsg->Descriptor.Process);
        }
    }

    return Result;
}

DWORD IoThread::_IoDefault()
{
    return STATUS_UNSUCCESSFUL;
}
