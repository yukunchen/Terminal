#pragma once

#include "IApiResponders.h"
#include "DeviceProtocol.h"

DWORD PendingPermitted(_In_ CONSOLE_API_MSG* const pMsg);

DWORD LookupAndDoApiCall(_In_ IApiResponders* pResponders, _In_ CONSOLE_API_MSG* const pMsg);
DWORD DoRawReadCall(_In_ IApiResponders* pResponders, _In_ CONSOLE_API_MSG* const pMsg);
DWORD DoRawWriteCall(_In_ IApiResponders* pResponders, _In_ CONSOLE_API_MSG* const pMsg);
