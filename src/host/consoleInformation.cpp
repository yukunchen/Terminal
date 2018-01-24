/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include <intsafe.h>

#include "..\interactivity\inc\ServiceLocator.hpp"



CONSOLE_INFORMATION::CONSOLE_INFORMATION() :
    // ProcessHandleList initializes itself
    pInputBuffer(nullptr),
    CurrentScreenBuffer(nullptr),
    ScreenBuffers(nullptr),
    OutputQueue(),
    // CommandHistoryList initialized below
    // ExeAliasList initialized below
    NumCommandHistories(0),
    OriginalTitle(nullptr),
    Title(nullptr),
    LinkTitle(nullptr),
    Flags(0),
    PopupCount(0),
    CP(0),
    OutputCP(0),
    CtrlFlags(0),
    LimitingProcessId(0),
    // ColorTable initialized below
    // CPInfo initialized below
    // OutputCPInfo initialized below
    lpCookedReadData(nullptr),
    // ConsoleIme initialized below
    terminalMouseInput(HandleTerminalKeyEventCallback),
    _vtIo()
{
    InitializeListHead(&CommandHistoryList);
    InitializeListHead(&ExeAliasList);

    ZeroMemory((void*)&CPInfo, sizeof(CPInfo));
    ZeroMemory((void*)&OutputCPInfo, sizeof(OutputCPInfo));
    ZeroMemory((void*)&ConsoleIme, sizeof(ConsoleIme));
    InitializeCriticalSection(&_csConsoleLock);
}

CONSOLE_INFORMATION::~CONSOLE_INFORMATION()
{
    DeleteCriticalSection(&_csConsoleLock);
}

bool CONSOLE_INFORMATION::IsConsoleLocked() const
{
    // The critical section structure's OwningThread field contains the ThreadId despite having the HANDLE type.
    // This requires us to hard cast the ID to compare.
    return _csConsoleLock.OwningThread == (HANDLE)GetCurrentThreadId();
}

#pragma prefast(suppress:26135, "Adding lock annotation spills into entire project. Future work.")
void CONSOLE_INFORMATION::LockConsole()
{
    EnterCriticalSection(&_csConsoleLock);
}

#pragma prefast(suppress:26135, "Adding lock annotation spills into entire project. Future work.")
BOOL CONSOLE_INFORMATION::TryLockConsole()
{
    return TryEnterCriticalSection(&_csConsoleLock);
}

#pragma prefast(suppress:26135, "Adding lock annotation spills into entire project. Future work.")
void CONSOLE_INFORMATION::UnlockConsole()
{
    LeaveCriticalSection(&_csConsoleLock);
}

ULONG CONSOLE_INFORMATION::GetCSRecursionCount()
{
    return _csConsoleLock.RecursionCount;
}

VtIo* CONSOLE_INFORMATION::GetVtIo()
{
    return &_vtIo;
}

bool CONSOLE_INFORMATION::IsInVtIoMode() const
{
    return _vtIo.IsUsingVt();
}

// Routine Description:
// - Handler for inserting key sequences into the buffer when the terminal emulation layer
//   has determined a key can be converted appropriately into a sequence of inputs
// Arguments:
// - events - the input events to write to the input buffer
// Return Value:
// - <none>
void CONSOLE_INFORMATION::HandleTerminalKeyEventCallback(_Inout_ std::deque<std::unique_ptr<IInputEvent>>& events)
{
    ServiceLocator::LocateGlobals()->getConsoleInformation()->pInputBuffer->Write(events);
}

// Method Description:
// - Return the active screen buffer of the console.
// Arguments:
// - <none>
// Return Value:
// - the active screen buffer of the console.
SCREEN_INFORMATION* const CONSOLE_INFORMATION::GetActiveOutputBuffer() const
{
    return CurrentScreenBuffer;
}

// Method Description:
// - Return the active input buffer of the console.
// Arguments:
// - <none>
// Return Value:
// - the active input buffer of the console.
InputBuffer* const CONSOLE_INFORMATION::GetActiveInputBuffer() const
{
    return pInputBuffer;
}

// Method Description:
// - Return the default foreground color of the console.
// Arguments:
// - <none>
// Return Value:
// - the default foreground color of the console.
COLORREF CONSOLE_INFORMATION::GetDefaultForeground() const
{
    return ForegroundColor(GetFillAttribute(), GetColorTable(), GetColorTableSize());
}

// Method Description:
// - Return the default background color of the console.
// Arguments:
// - <none>
// Return Value:
// - the default background color of the console.
COLORREF CONSOLE_INFORMATION::GetDefaultBackground() const
{
    return BackgroundColor(GetFillAttribute(), GetColorTable(), GetColorTableSize());
}
