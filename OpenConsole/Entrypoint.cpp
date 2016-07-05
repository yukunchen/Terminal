#include "stdafx.h"
#include "Entrypoint.h"

#include "..\ServerBase\IoThread.h"
#include "..\ServerSample\ApiResponderEmpty.h"

extern "C"
{
    std::vector<IoThread*> ioThreads;

    _declspec(dllexport)
        NTSTATUS ConsoleCreateIoThread(_In_ HANDLE Server)
    {
        ApiResponderEmpty* const pResponder = new ApiResponderEmpty();

        IoThread* const pNewThread = new IoThread(Server, pResponder);
        ioThreads.push_back(pNewThread);

        return STATUS_SUCCESS;
    }
}