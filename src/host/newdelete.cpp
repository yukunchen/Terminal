/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

static HANDLE g_hHeap = nullptr;

BOOL EnsureHeap()
{
    BOOL fRet = (g_hHeap != nullptr);
    if (!fRet)
    {
        g_hHeap = HeapCreate(0, 0, 0);
        fRet = (g_hHeap != nullptr);
    }

    return fRet;
}

void DestroyHeap()
{
    if (g_hHeap)
    {
        HeapDestroy(g_hHeap);
        g_hHeap = nullptr;
    }
}

void * _cdecl operator new(_In_ size_t size)
{
    return HeapAlloc(g_hHeap, HEAP_ZERO_MEMORY, size);
}

void * __cdecl operator new[](_In_ size_t cb)
{
    return operator new(cb);
}

void _cdecl operator delete(_In_ void *pv)
{
    HeapFree(g_hHeap, 0, pv);
}

void __cdecl operator delete[](_In_ void * p)
{
    operator delete(p);
}
