#include "stdafx.h"
#include "ProcessHandle.h"



std::vector<ConsoleProcessHandle*> ConsoleProcessHandle::s_KnownProcesses;

ConsoleProcessHandle::ConsoleProcessHandle(_In_ DWORD ProcessId,
                                           _In_ DWORD ThreadId,
                                           _In_ ULONG ProcessGroupId,
                                           _In_ ConsoleHandleData* const pInputHandle,
                                           _In_ ConsoleHandleData* const pOutputHandle) :
    _ProcessId(ProcessId),
    _ThreadId(ThreadId),
    _ProcessGroupId(ProcessGroupId),
    _RootProcess(s_KnownProcesses.empty()),
    _ProcessHandle(OpenProcess(MAXIMUM_ALLOWED, FALSE, ProcessId)), // null OK on failure. We'll just work without it.
    _pInputHandle(pInputHandle),
    _pOutputHandle(pOutputHandle)
{
    s_KnownProcesses.push_back(this);
}

ConsoleProcessHandle::~ConsoleProcessHandle()
{
    // Find the matching element.
    auto it = find(s_KnownProcesses.begin(), s_KnownProcesses.end(), this);

    // If it's not the last one, swap it to the last one to make removal more efficient.
    if (it != s_KnownProcesses.end())
    {
        swap(*it, s_KnownProcesses.back());
    }

    // Remove the last element.
    s_KnownProcesses.pop_back();

    // Free input/output handles
    delete _pInputHandle;
    delete _pOutputHandle;

    // Close process handle
    if (_ProcessHandle != nullptr)
    {
        CloseHandle(_ProcessHandle);
    }
}

ULONG ConsoleProcessHandle::GetProcessId() const
{
    return _ProcessId;
}
