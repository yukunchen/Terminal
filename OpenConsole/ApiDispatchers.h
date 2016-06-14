#pragma once

#include "IApiResponders.h"

namespace ApiDispatchers
{
    NTSTATUS ServeGetConsoleCursorInfo(_In_ IApiResponders* const _pResponders, _Inout_ CONSOLE_API_MSG* const m);
};

