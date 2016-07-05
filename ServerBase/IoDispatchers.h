#pragma once

#include "..\ServerBaseApi\IApiResponders.h"
#include "..\ServerBaseApi\IApiService.h"
#include "ApiMessage.h"
#include "DeviceProtocol.h"

class IoDispatchers : public IApiService
{
public:
    IoDispatchers(_In_ DeviceProtocol* const pProtocol,
                  _In_ IApiResponders* const pResponder);
    ~IoDispatchers();

    void ServiceIoOperation(_In_ CONSOLE_API_MSG* const pMsg);

    void NotifyInputReadWait();
    void NotifyOutputWriteWait();

private:

    void _NotifyWait(_In_ std::queue<CONSOLE_API_MSG>* const pQueue);

    NTSTATUS _ServiceIoOperation(_In_ CONSOLE_API_MSG* const pMsg);
    static NTSTATUS _IoConnect(_In_ IApiResponders* const pResponder, _In_ DeviceProtocol* Server, _In_ CONSOLE_API_MSG* const pMsg);
    static NTSTATUS _IoDisconnect(_In_ CONSOLE_API_MSG* const pMsg);
    static NTSTATUS _IoDefault();

    IApiResponders* const _pResponder;
    DeviceProtocol* const _pProtocol;

    std::mutex ioOperation;
    std::queue<CONSOLE_API_MSG> _QueuedReadInput;
    std::queue<CONSOLE_API_MSG> _QueuedWriteOutput;

};

