#pragma once

#include "DeviceProtocol.h"
#include "..\ServerBaseApi\IApiResponders.h"

class IoThread
{
public:
    IoThread(_In_ HANDLE Server,
             _In_ IApiResponders* const pResponder);
    ~IoThread();

    static void s_IoLoop(_In_ IoThread* const pIoThread);
    void IoLoop();

private:
    HANDLE const _Server;
    IApiResponders* const _pResponder;
    std::thread _Thread;
};

