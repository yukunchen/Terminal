#pragma once

#include "IApiResponders.h"
#include "DeviceProtocol.h"

DWORD DoApiCall(_In_ DeviceProtocol* Server, _In_ IApiResponders* pResponders, _In_ CONSOLE_API_MSG* const pMsg)
