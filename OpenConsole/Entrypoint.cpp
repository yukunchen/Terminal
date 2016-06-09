#include "stdafx.h"
#include "Entrypoint.h"

using namespace std;

void IoLoop(_In_ HANDLE Server)
{
    bool fExiting = false;
    while (!fExiting)
    {
        CONSOLE_API_MSG ReceiveMsg;

        DWORD Written = 0;

        BOOL Succeeded = DeviceIoControl(Server,
                                         IOCTL_CONDRV_READ_IO,
                                         nullptr,
                                         0,
                                         &ReceiveMsg.Descriptor,
                                         sizeof(CONSOLE_API_MSG) - FIELD_OFFSET(CONSOLE_API_MSG, Descriptor),
                                         &Written,
                                         nullptr);


    }
}

_declspec(dllexport)
NTSTATUS ConsoleCreateIoThread(_In_ HANDLE Server)
{
    thread ioThread(IoLoop, Server);
    ioThread.detach();

    return STATUS_SUCCESS;
}

