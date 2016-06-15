#include "stdafx.h"
#include "ApiResponderEmpty.h"


ApiResponderEmpty::ApiResponderEmpty()
{
}


ApiResponderEmpty::~ApiResponderEmpty()
{
}

DWORD ApiResponderEmpty::GetConsoleCursorInfoImpl(_In_ void* const pContext, 
                                                  _Out_ ULONG* const pCursorSize, 
                                                  _Out_ BOOLEAN* const pIsVisible)
{
    UNREFERENCED_PARAMETER(pContext);
    *pCursorSize = 60;
    *pIsVisible = TRUE;
    return 0;
}


