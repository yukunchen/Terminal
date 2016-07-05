#pragma once

#include "..\ServerBaseApi\IApiResponders.h"
#include "DeviceProtocol.h"

namespace ApiSorter
{
    ULONG PendingPermitted(_In_ CONSOLE_API_MSG* const pMsg);

    NTSTATUS LookupAndDoApiCall(_In_ IApiResponders* pResponders, _In_ CONSOLE_API_MSG* const pMsg);
    NTSTATUS DoRawReadCall(_In_ IApiResponders* pResponders, _In_ CONSOLE_API_MSG* const pMsg);
    NTSTATUS DoRawWriteCall(_In_ IApiResponders* pResponders, _In_ CONSOLE_API_MSG* const pMsg);
};