/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"
#include "globals.h"
#include "server.h"
#include <intsafe.h>
#include "telemetry.hpp"
#include "utils.hpp"


CONSOLE_INFORMATION::CONSOLE_INFORMATION() :
    // ProcessHandleList initialized below
    pInputBuffer(nullptr),
    CurrentScreenBuffer(nullptr),
    ScreenBuffers(nullptr),
    hWnd(nullptr),
    hMenu(nullptr),
    hHeirMenu(nullptr),
    // OutputQueue initialized below
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
    // MessageQueue initialized below
    // CPInfo initialized below
    // OutputCPInfo initialized below
    ReadConInpNumBytesUnicode(0),
    WriteConOutNumBytesUnicode(0),
    WriteConOutNumBytesTemp(0),
    lpCookedReadData(nullptr),
    // ConsoleIme initialized below
    termInput(HandleTerminalKeyEventCallback),
    terminalMouseInput(HandleTerminalKeyEventCallback)
{
    ZeroMemory((void*)&ProcessHandleList, sizeof(ProcessHandleList));
    InitializeListHead(&g_ciConsoleInformation.ProcessHandleList);

    ZeroMemory((void*)&CommandHistoryList, sizeof(CommandHistoryList));
    InitializeListHead(&g_ciConsoleInformation.CommandHistoryList);

    ZeroMemory((void*)&ExeAliasList, sizeof(ExeAliasList));
    InitializeListHead(&g_ciConsoleInformation.ExeAliasList);

    ZeroMemory((void*)&MessageQueue, sizeof(MessageQueue));
    InitializeListHead(&g_ciConsoleInformation.MessageQueue);

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
    return _csConsoleLock.OwningThread == (HANDLE)GetCurrentThreadId();
}

#pragma prefast(suppress:26135, "Adding lock annotation spills into entire project. Future work.")
void CONSOLE_INFORMATION::LockConsole()
{
    EnterCriticalSection(&_csConsoleLock);
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

// Routine Description:
// - Handler for inserting key sequences into the buffer when the terminal emulation layer
//   has determined a key can be converted appropriately into a sequence of inputs
// Arguments:
// - rgInput - Series of input records to insert into the buffer
// - cInput - Length of input records array
// Return Value:
// - <none>
void HandleTerminalKeyEventCallback(_In_reads_(cInput) INPUT_RECORD* rgInput, _In_ DWORD cInput)
{
    WriteInputBuffer(g_ciConsoleInformation.pInputBuffer, rgInput, cInput);
}
