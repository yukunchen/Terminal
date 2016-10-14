/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"
#include "inputReadHandleData.h"

INPUT_READ_HANDLE_DATA::INPUT_READ_HANDLE_DATA()
{
    InitializeCriticalSection(&_csReadCountLock);
}

INPUT_READ_HANDLE_DATA::~INPUT_READ_HANDLE_DATA()
{
    DeleteCriticalSection(&_csReadCountLock);
}

_Acquires_lock_(&_csReadCountLock) void INPUT_READ_HANDLE_DATA::LockReadCount()
{
    EnterCriticalSection(&_csReadCountLock);
}

_Releases_lock_(&_csReadCountLock) void INPUT_READ_HANDLE_DATA::UnlockReadCount()
{
    LeaveCriticalSection(&_csReadCountLock);
}

void INPUT_READ_HANDLE_DATA::IncrementReadCount()
{
    LockReadCount();
    _ulReadCount++;
    UnlockReadCount();
}

void INPUT_READ_HANDLE_DATA::DecrementReadCount()
{
    LockReadCount();
    _ulReadCount--;
    UnlockReadCount();
}

_Requires_lock_held_(&_csReadCountLock) ULONG INPUT_READ_HANDLE_DATA::GetReadCount() const
{
    return _ulReadCount;
}



