/*++
Copyright (c) Microsoft Corporation

Module Name:
- InputReadHandleData.h

Abstract:
- This file defines counters and state information related to reading input from the internal buffers
  when called from a particular input handle that has been given to a client application.
  It's used to know where the next bit of continuation should be if the same input handle doesn't have a big
  enough buffer and must split data over multiple returns.

Author:
- Michael Niksa (miniksa) 12-Oct-2016

Revision History:
- Adapted from original items in handle.h
--*/


#pragma once 

class INPUT_READ_HANDLE_DATA
{
public:
    INPUT_READ_HANDLE_DATA();
    ~INPUT_READ_HANDLE_DATA();

    INPUT_READ_HANDLE_DATA(const INPUT_READ_HANDLE_DATA&) = delete;
    INPUT_READ_HANDLE_DATA(INPUT_READ_HANDLE_DATA&&) = delete;
    INPUT_READ_HANDLE_DATA& operator=(const INPUT_READ_HANDLE_DATA&) & = delete;
    INPUT_READ_HANDLE_DATA& operator=(INPUT_READ_HANDLE_DATA&&) & = delete;

    enum HandleFlags
    {
        Closing = 0x1,
        InputPending = 0x2,
        MultiLineInput = 0x4
    };

    // the following four fields are used to remember input data that wasn't returned on a cooked-mode read. we do our
    // own buffering and don't return data until the user hits enter so that she can edit the input. as a result, there
    // is often data that doesn't fit into the caller's buffer. we save it so we can return it on the next cooked-mode
    // read to this handle.

    ULONG BytesAvailable;
    PWCHAR CurrentBufPtr;
    PWCHAR BufPtr;
    HandleFlags InputHandleFlags;

    _Acquires_lock_(&_csReadCountLock) void LockReadCount();
    _Releases_lock_(&_csReadCountLock) void UnlockReadCount();
    void IncrementReadCount();
    void DecrementReadCount();
    _Requires_lock_held_(&_csReadCountLock) ULONG GetReadCount() const;

private:

    // the following fields are solely used for input reads.
    CRITICAL_SECTION _csReadCountLock;  // serializes access to read count
    _Guarded_by_(&_csReadCountLock) ULONG _ulReadCount;    // number of reads waiting
};

DEFINE_ENUM_FLAG_OPERATORS(INPUT_READ_HANDLE_DATA::HandleFlags);
