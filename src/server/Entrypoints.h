#pragma once

namespace Entrypoints
{
    HRESULT StartConsoleForServerHandle(_In_ HANDLE const ServerHandle);
    HRESULT StartConsoleForCmdLine(_In_ PCWSTR pwszCmdLine);
};

