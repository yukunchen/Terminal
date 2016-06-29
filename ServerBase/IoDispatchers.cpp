#include "stdafx.h"
#include "IoDispatchers.h"

#include "ApiMessage.h"
#include "ObjectHandle.h"

#include "DeviceProtocol.h"

#include "ProcessHandle.h"

#include "..\ServerBaseApi\IApiResponders.h"
#include "ApiSorter.h"
#include "Win32Control.h"


IoDispatchers::IoDispatchers(_In_ DeviceProtocol* const pProtocol,
                             _In_ IApiResponders* const pResponder) :
    _pProtocol(pProtocol),
    _pResponder(pResponder)
{
    _pResponder->SetApiService(this);
}

IoDispatchers::~IoDispatchers()
{
}

void IoDispatchers::NotifyInputReadWait()
{
    _NotifyWait(&_QueuedReadInput);
}

void IoDispatchers::NotifyOutputWriteWait()
{
    _NotifyWait(&_QueuedWriteOutput);
}

void IoDispatchers::_NotifyWait(_In_ queue<CONSOLE_API_MSG>* const pQueue)
{
    ioOperation.lock();
    {
        while (!pQueue->empty())
        {
            // Retrieve first queued item
            CONSOLE_API_MSG msg = pQueue->front();

            // Attempt to service it.
            DWORD Result = _ServiceIoOperation(&msg);

            // If it's pending, give up here and we'll try again later.
            if (Result == ERROR_IO_PENDING)
            {
                break;
            }

            // If we're good to go, pop it and keep going down the queue.
            pQueue->pop();
        }
    }
    ioOperation.unlock();
}

void IoDispatchers::ServiceIoOperation(_In_ CONSOLE_API_MSG* const pMsg)
{
    ioOperation.lock();
    {
        // Try to service the operation.
        DWORD Result = _ServiceIoOperation(pMsg);

        // If the call was pending, push it to be serviced later.
        if (Result == ERROR_IO_PENDING)
        {
            DWORD PendingCode = PendingPermitted(pMsg);

            // Queue pending calls to be serviced later when we're notified.
            // Copy the contents of pMsg when storing.
            if (PendingCode == 0x1000005) // ReadConsole
            {
                _QueuedReadInput.push(*pMsg);
            }
            else if (PendingCode == 0x1000006) // WriteConsole
            {
                _QueuedWriteOutput.push(*pMsg);
            }
            else
            {
                Result = ERROR_ASSERTION_FAILURE;
            }
        }
    }
    ioOperation.unlock();
}

DWORD IoDispatchers::_ServiceIoOperation(_In_ CONSOLE_API_MSG* const pMsg)
{
    DWORD Result = 0;

    // Route function to handler
    switch (pMsg->Descriptor.Function)
    {
    case CONSOLE_IO_USER_DEFINED:
    {
        Result = LookupAndDoApiCall(_pResponder, pMsg);
        break;
    }
    case CONSOLE_IO_CONNECT:
    {
        Result = _IoConnect(_pResponder, _pProtocol, pMsg);
        break;
    }
    case CONSOLE_IO_DISCONNECT:
    {
        Result = _IoDisconnect(pMsg);
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
        Result = DoRawWriteCall(_pResponder, pMsg);
        break;
    }
    case CONSOLE_IO_RAW_READ:
    {
        Result = DoRawReadCall(_pResponder, pMsg);
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
    pMsg->SetCompletionStatus(Result);

    // Do not reply to pending calls.
    if (Result != ERROR_IO_PENDING)
    {
        // Write reply message signaling API call is complete.
        _pProtocol->CompleteApiCall(pMsg);
    }

    return Result;
}

DWORD IoDispatchers::_IoDisconnect(_In_ CONSOLE_API_MSG* const pMsg)
{
    // FreeConsole called.

    // NotifyWinEvent

    delete pMsg->GetProcessHandle();

    return STATUS_SUCCESS;
}

DWORD IoDispatchers::_IoConnect(_In_ IApiResponders* const pResponder, _In_ DeviceProtocol* Server, _In_ CONSOLE_API_MSG* const pMsg)
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

DWORD IoDispatchers::_IoDefault()
{
    return STATUS_UNSUCCESSFUL;
}
