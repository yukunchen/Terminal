#pragma once

#include "DeviceProtocol.h"
#include "IApiResponders.h"

class IoThread
{
public:
    IoThread(_In_ HANDLE Server);
    ~IoThread();

    static void s_IoLoop(_In_ IoThread* const pIoThread);
    void IoLoop();

private:
    HANDLE _Server;
    std::thread _Thread;
};

