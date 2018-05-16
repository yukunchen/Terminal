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

    ~INPUT_READ_HANDLE_DATA() = default;
    INPUT_READ_HANDLE_DATA(const INPUT_READ_HANDLE_DATA&) = delete;
    INPUT_READ_HANDLE_DATA(INPUT_READ_HANDLE_DATA&&) = delete;
    INPUT_READ_HANDLE_DATA& operator=(const INPUT_READ_HANDLE_DATA&) & = delete;
    INPUT_READ_HANDLE_DATA& operator=(INPUT_READ_HANDLE_DATA&&) & = delete;

    /*ULONG BytesAvailable;
    PWCHAR CurrentBufPtr;
    PWCHAR BufPtr;*/

    void IncrementReadCount();
    void DecrementReadCount();
    size_t GetReadCount();

    bool IsInputPending() const;
    bool IsMultilineInput() const;

    void SavePendingInput(std::string_view pending);
    void SaveMultilinePendingInput(std::string_view pending);
    void UpdatePending(std::string_view pending);

    std::string_view GetPendingInput() const;

private:

    bool _isInputPending;
    bool _isMultilineInput;

    std::string _buffer;

    // the following fields are solely used for input reads.
    wil::critical_section _readCountLock; // serializes access to read count
    size_t _readCount;    // number of reads waiting
};
