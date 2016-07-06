#pragma once

#include "..\ServerBaseApi\IApiResponders.h"

namespace Entrypoints
{
    NTSTATUS StartConsoleForServerHandle(_In_ HANDLE const ServerHandle,
                                         _In_ IApiResponders* const pResponder);

    NTSTATUS StartConsoleForCmdLine(_In_ PCWSTR pBinaryPathAndArgs,
                                    _In_ IApiResponders* const pResponder);
};

