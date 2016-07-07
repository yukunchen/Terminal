#include "stdafx.h"

#include "..\ServerSample\ApiResponderEmpty.h"
#include "..\ServerBaseApi\Entrypoints.h"

extern "C"
{
    _declspec(dllexport)
        NTSTATUS ConsoleCreateIoThread(_In_ HANDLE Server)
    {
        ApiResponderEmpty* const pResponder = new ApiResponderEmpty();

        return Entrypoints::StartConsoleForServerHandle(Server, pResponder);
    }
}