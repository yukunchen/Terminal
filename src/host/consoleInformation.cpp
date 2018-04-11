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
    _OriginalTitle(),
    _Title(),
    _LinkTitle(),
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
    ConsoleIme{},
    terminalMouseInput(HandleTerminalKeyEventCallback),
    _vtIo()
{
    InitializeListHead(&CommandHistoryList);

    ZeroMemory((void*)&CPInfo, sizeof(CPInfo));
    ZeroMemory((void*)&OutputCPInfo, sizeof(OutputCPInfo));
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
bool CONSOLE_INFORMATION::TryLockConsole()
{
    return !!TryEnterCriticalSection(&_csConsoleLock);
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
    ServiceLocator::LocateGlobals().getConsoleInformation().pInputBuffer->Write(events);
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

// Method Description:
// - Set the console's title, and trigger a renderer update of the title.
//      This does not include the title prefix, such as "Mark", "Select", or "Scroll"
// Arguments:
// - newTitle: The new value to use for the title
// Return Value:
// - <none>
void CONSOLE_INFORMATION::SetTitle(const std::wstring& newTitle)
{
    _Title = newTitle;

    auto* const pRender = ServiceLocator::LocateGlobals().pRender;
    if (pRender)
    {
        pRender->TriggerTitleChange();
    }
}

// Method Description:
// - Set the console title's prefix, and trigger a renderer update of the title.
//      This is the part of the title shuch as "Mark", "Select", or "Scroll"
// Arguments:
// - newTitlePrefix: The new value to use for the title prefix
// Return Value:
// - <none>
void CONSOLE_INFORMATION::SetTitlePrefix(const std::wstring& newTitlePrefix)
{
    _TitlePrefix = newTitlePrefix;

    auto* const pRender = ServiceLocator::LocateGlobals().pRender;
    if (pRender)
    {
        pRender->TriggerTitleChange();
    }
}

// Method Description:
// - Set the value of the console's original title. This is the title the
//      console launched with.
// Arguments:
// - originalTitle: The new value to use for the console's original title
// Return Value:
// - <none>
void CONSOLE_INFORMATION::SetOriginalTitle(const std::wstring& originalTitle)
{
    _OriginalTitle = originalTitle;
}

// Method Description:
// - Set the value of the console's link title. If the console was launched
///     from a shortcut, this value will not be the empty string.
// Arguments:
// - linkTitle: The new value to use for the console's link title
// Return Value:
// - <none>
void CONSOLE_INFORMATION::SetLinkTitle(const std::wstring& linkTitle)
{
    _LinkTitle = linkTitle;
}

// Method Description:
// - return a reference to the console's title.
// Arguments:
// - <none>
// Return Value:
// - a reference to the console's title.
const std::wstring& CONSOLE_INFORMATION::GetTitle() const
{
    return _Title;
}

// Method Description:
// - Return a new wstring representing the actual display value of the title.
//      This is the Prefix+Title.
// Arguments:
// - <none>
// Return Value:
// - a new wstring containing the combined prefix and title.
const std::wstring CONSOLE_INFORMATION::GetTitleAndPrefix() const
{
    return _TitlePrefix + _Title;
}

// Method Description:
// - return a reference to the console's original title.
// Arguments:
// - <none>
// Return Value:
// - a reference to the console's original title.
const std::wstring& CONSOLE_INFORMATION::GetOriginalTitle() const
{
    return _OriginalTitle;
}

// Method Description:
// - return a reference to the console's link title.
// Arguments:
// - <none>
// Return Value:
// - a reference to the console's link title.
const std::wstring& CONSOLE_INFORMATION::GetLinkTitle() const
{
    return _LinkTitle;
}
