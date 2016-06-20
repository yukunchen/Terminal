#include "stdafx.h"
#include "Entrypoint.h"

#include "IoThread.h"

extern "C"
{
	std::vector<IoThread*> ioThreads;

    _declspec(dllexport)
        NTSTATUS ConsoleCreateIoThread(_In_ HANDLE Server)
    {
		IoThread* const pNewThread = new IoThread(Server);
		ioThreads.push_back(pNewThread);

        return STATUS_SUCCESS;
    }
}
