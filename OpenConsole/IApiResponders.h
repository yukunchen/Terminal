#pragma once

class IApiResponders
{
public:
    virtual DWORD GetConsoleCursorInfoImpl(_In_ void* const pContext, 
                                           _Out_ ULONG* const pCursorSize,
                                           _Out_ BOOLEAN* const pIsVisible) = 0;
};