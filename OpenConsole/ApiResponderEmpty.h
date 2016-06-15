#pragma once

#include "IApiResponders.h"

class ApiResponderEmpty : public IApiResponders
{
public:
    ApiResponderEmpty();
    ~ApiResponderEmpty();
    DWORD GetConsoleCursorInfoImpl(_In_ void* const pContext,
                                   _Out_ ULONG* const pCursorSize,
                                   _Out_ BOOLEAN* const pIsVisible);
};

