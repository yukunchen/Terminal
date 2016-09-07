/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include <precomp.h>
#include "newdelete.hpp"
#include "telemetry.hpp"

extern "C" BOOL DllMain(_In_ HINSTANCE hInstance, _In_ DWORD dwReason, _Reserved_ LPVOID /*lpReserved*/)
{
    BOOL fRet = TRUE;
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        {
            DisableThreadLibraryCalls(hInstance);
            fRet = EnsureHeap();
            break;
        }

    case DLL_PROCESS_DETACH:
        {
            DestroyHeap();
            break;
        }

    default:
        {
            break;
        }
    }

    return fRet;
}
