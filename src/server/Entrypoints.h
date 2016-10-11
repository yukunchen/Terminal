#pragma once

namespace Entrypoints
{
    NTSTATUS StartConsoleForServerHandle(_In_ HANDLE const ServerHandle);
    NTSTATUS StartConsoleForCmdLine(_In_ PCWSTR pwszCmdLine);
};

