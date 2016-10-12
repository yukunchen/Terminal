/*++
Copyright (c) Microsoft Corporation

Module Name:
- server.h

Abstract:
- This module contains the internal structures and definitions used by the console server.

Author:
- Therese Stowell (ThereseS) 12-Nov-1990

Revision History:
--*/

#pragma once

#include "screenInfo.hpp"

#include "settings.hpp"
#include "window.hpp"

#include "conimeinfo.h"
#include "..\terminal\adapter\terminalInput.hpp"
#include "..\terminal\adapter\MouseInput.hpp"

// Flags flags
#define CONSOLE_IS_ICONIC               0x00000001
#define CONSOLE_OUTPUT_SUSPENDED        0x00000002
#define CONSOLE_HAS_FOCUS               0x00000004
#define CONSOLE_IGNORE_NEXT_MOUSE_INPUT 0x00000008
#define CONSOLE_SELECTING               0x00000010
#define CONSOLE_SCROLLING               0x00000020
// unused (CONSOLE_DISABLE_CLOSE)       0x00000040
// unused (CONSOLE_USE_POLY_TEXT)       0x00000080
#define CONSOLE_NO_WINDOW               0x00000100
// unused (CONSOLE_VDM_REGISTERED)      0x00000200
#define CONSOLE_UPDATING_SCROLL_BARS    0x00000400
#define CONSOLE_QUICK_EDIT_MODE         0x00000800
#define CONSOLE_CONNECTED_TO_EMULATOR   0x00002000
// unused (CONSOLE_FULLSCREEN_NOPAINT)  0x00004000
#define CONSOLE_QUIT_POSTED             0x00008000
#define CONSOLE_AUTO_POSITION           0x00010000
#define CONSOLE_IGNORE_NEXT_KEYUP       0x00020000
// unused (CONSOLE_WOW_REGISTERED)      0x00040000
#define CONSOLE_HISTORY_NODUP           0x00100000
#define CONSOLE_SCROLLBAR_TRACKING      0x00200000
#define CONSOLE_SETTING_WINDOW_SIZE     0x00800000
// unused (CONSOLE_VDM_HIDDEN_WINDOW)   0x01000000
// unused (CONSOLE_OS2_REGISTERED)      0x02000000
// unused (CONSOLE_OS2_OEM_FORMAT)      0x04000000
// unused (CONSOLE_JUST_VDM_UNREGISTERED)  0x08000000
// unused (CONSOLE_FULLSCREEN_INITIALIZED) 0x10000000
#define CONSOLE_USE_PRIVATE_FLAGS       0x20000000
// unused (CONSOLE_TSF_ACTIVATED)       0x40000000
#define CONSOLE_INITIALIZED             0x80000000

#define CONSOLE_SUSPENDED (CONSOLE_OUTPUT_SUSPENDED)

class COOKED_READ_DATA;

class CONSOLE_INFORMATION : public Settings
{
public:
    CONSOLE_INFORMATION();
    ~CONSOLE_INFORMATION();

    LIST_ENTRY ProcessHandleList;
    PINPUT_INFORMATION pInputBuffer;

    Window* pWindow;
    PSCREEN_INFORMATION CurrentScreenBuffer;
    PSCREEN_INFORMATION ScreenBuffers;  // singly linked list
    HWND hWnd;
    HMENU hMenu;    // handle to system menu
    HMENU hHeirMenu;    // handle to menu we append to system menu
    LIST_ENTRY OutputQueue;
    LIST_ENTRY CommandHistoryList;
    LIST_ENTRY ExeAliasList;
    UINT NumCommandHistories;

    LPWSTR OriginalTitle;
    LPWSTR Title;
    LPWSTR LinkTitle;   // Path to .lnk file, can be nullptr

    DWORD Flags;

    WORD PopupCount;

    // the following fields are used for ansi-unicode translation
    UINT CP;
    UINT OutputCP;

    ULONG CtrlFlags;    // indicates outstanding ctrl requests
    ULONG LimitingProcessId;

    LIST_ENTRY MessageQueue;

    CPINFO CPInfo;
    CPINFO OutputCPInfo;

    DWORD ReadConInpNumBytesUnicode;

    DWORD WriteConOutNumBytesUnicode;
    DWORD WriteConOutNumBytesTemp;

    COOKED_READ_DATA* lpCookedReadData;

    ConsoleImeInfo ConsoleIme;

    Microsoft::Console::VirtualTerminal::TerminalInput termInput;
    Microsoft::Console::VirtualTerminal::MouseInput terminalMouseInput;

    void LockConsole();
    void UnlockConsole();
    bool IsConsoleLocked() const;
    ULONG GetCSRecursionCount();

private:
    CRITICAL_SECTION _csConsoleLock;   // serialize input and output using this
};

// input handle flags
#define HANDLE_CLOSING 1
#define HANDLE_INPUT_PENDING 2
#define HANDLE_MULTI_LINE_INPUT 4

#include "..\server\ProcessHandle.h"

#include "..\server\WaitBlock.h"

#define ConsoleLocked() (g_ciConsoleInformation.ConsoleLock.OwningThread == NtCurrentTeb()->ClientId.UniqueThread)

#define CONSOLE_STATUS_WAIT 0xC0030001
#define CONSOLE_STATUS_READ_COMPLETE 0xC0030002
#define CONSOLE_STATUS_WAIT_NO_BLOCK 0xC0030003

#include "..\server\ObjectHandle.h"

BOOL ConsoleCreateWait(_In_ PLIST_ENTRY pWaitQueue,
                       _In_ CONSOLE_WAIT_ROUTINE pfnWaitRoutine,
                       _Inout_ PCONSOLE_API_MSG pWaitReplyMessage,
                       _In_ PVOID pvWaitParameter);
BOOL ConsoleNotifyWaitBlock(_In_ PCONSOLE_WAIT_BLOCK pWaitBlock, _In_ PLIST_ENTRY pWaitQueue, _In_ PVOID pvSatisfyParameter, _In_ BOOL fThreadDying);
BOOL ConsoleNotifyWait(_In_ PLIST_ENTRY pWaitQueue, _In_ const BOOL fSatisfyAll, _In_ PVOID pvSatisfyParameter);

NTSTATUS ReadMessageInput(_In_ PCONSOLE_API_MSG pMessage, _In_ const ULONG ulOffset, _Out_writes_bytes_(cbSize) PVOID pvBuffer, _In_ const ULONG cbSize);
NTSTATUS GetAugmentedOutputBuffer(_Inout_ PCONSOLE_API_MSG pMessage,
                                  _In_ const ULONG ulFactor,
                                  _Outptr_result_bytebuffer_(*pcbSize) PVOID * ppvBuffer,
                                  _Out_ PULONG pcbSize);
NTSTATUS GetOutputBuffer(_Inout_ PCONSOLE_API_MSG pMessage, _Outptr_result_bytebuffer_(*pcbSize) PVOID * const ppvBuffer, _Out_ ULONG * const pcbSize);
NTSTATUS GetInputBuffer(_In_ PCONSOLE_API_MSG pMessage, _Outptr_result_bytebuffer_(*pcbSize) PVOID * const ppvBuffer, _Out_ ULONG * const pcbSize);

void ReleaseMessageBuffers(_Inout_ PCONSOLE_API_MSG pMessage);

void HandleTerminalKeyEventCallback(_In_reads_(cInput) INPUT_RECORD* rgInput, _In_ DWORD cInput);

NTSTATUS SetActiveScreenBuffer(_Inout_ PSCREEN_INFORMATION pScreenInfo);

struct _INPUT_INFORMATION;
typedef _INPUT_INFORMATION INPUT_INFORMATION;
typedef _INPUT_INFORMATION *PINPUT_INFORMATION;

PINPUT_INFORMATION GetInputBufferFromHandle(const CONSOLE_HANDLE_DATA* HandleData);
PSCREEN_INFORMATION GetScreenBufferFromHandle(const CONSOLE_HANDLE_DATA* HandleData);