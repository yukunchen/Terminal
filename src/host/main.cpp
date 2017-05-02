/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"
#include "newdelete.hpp"

#include "..\interactivity\inc\ServiceLocator.hpp"

extern "C" BOOL __stdcall DllMain(_In_ HINSTANCE hInstance, _In_ DWORD dwReason, _Reserved_ LPVOID /*lpReserved*/)
{
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        {
            EnsureHeap();
            DisableThreadLibraryCalls(hInstance);

            ServiceLocator::LocateGlobals()->hInstance = hInstance;
            
            break;
        }

    default:
        {
            break;
        }
    }

    return TRUE;
}
