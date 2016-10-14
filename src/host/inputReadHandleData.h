#pragma once 

// input handle flags
#define HANDLE_CLOSING 1
#define HANDLE_INPUT_PENDING 2
#define HANDLE_MULTI_LINE_INPUT 4

class INPUT_READ_HANDLE_DATA
{
public:
    INPUT_READ_HANDLE_DATA();
    ~INPUT_READ_HANDLE_DATA();

    // the following four fields are used to remember input data that wasn't returned on a cooked-mode read. we do our
    // own buffering and don't return data until the user hits enter so that she can edit the input. as a result, there
    // is often data that doesn't fit into the caller's buffer. we save it so we can return it on the next cooked-mode
    // read to this handle.

    ULONG BytesAvailable;
    PWCHAR CurrentBufPtr;
    PWCHAR BufPtr;
    ULONG InputHandleFlags;

    _Acquires_lock_(&_csReadCountLock) void LockReadCount();
    _Releases_lock_(&_csReadCountLock) void UnlockReadCount();
    void IncrementReadCount();
    void DecrementReadCount();
    _Requires_lock_held_(&_csReadCountLock) ULONG GetReadCount() const;
private:

    // the following seven fields are solely used for input reads.
    CRITICAL_SECTION _csReadCountLock;  // serializes access to read count
    _Guarded_by_(&_csReadCountLock) ULONG _ulReadCount;    // number of reads waiting
};
