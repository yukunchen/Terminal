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
    DWORD _IoConnect(_In_ IApiResponders* const pResponder, _In_ DeviceProtocol* Server, _In_ CONSOLE_API_MSG* const pMsg);
    DWORD _IoDisconnect(_In_ CONSOLE_API_MSG* const pMsg);
    DWORD _IoDefault();

    HANDLE _Server;
    std::thread _Thread;
};

