#pragma once

extern "C"
{
#include <DDK\wdm.h>
}

#include "ObjectHandle.h"

class ConsoleProcessHandle
{
public:

    ConsoleProcessHandle(_In_ DWORD ProcessId,
                         _In_ DWORD ThreadId,
                         _In_ ULONG ProcessGroupId,
                         //_In_ HANDLE InputEvent,
                         _In_ ConsoleHandleData* const pInputHandle,
                         _In_ ConsoleHandleData* const pOutputHandle);

    ~ConsoleProcessHandle();

    static std::vector<ConsoleProcessHandle*> ConsoleProcessHandle::s_KnownProcesses;

    ULONG GetProcessId() const;

private:

    

    //LIST_ENTRY WaitBlockQueue;
    //ULONG TerminateCount;
    //HANDLE const _InputEvent;
    

    HANDLE const _ProcessHandle;
    DWORD const _ProcessId;
    DWORD const _ThreadId;
    ULONG const _ProcessGroupId;

    BOOL const _RootProcess;
    
    ConsoleHandleData* const _pInputHandle;
    ConsoleHandleData* const _pOutputHandle;

    
};

