/*++
Copyright (c) Microsoft Corporation

Module Name:
- cmdline.h

Abstract:
- This file contains the internal structures and definitions used by command line input and editing.

Author:
- Therese Stowell (ThereseS) 15-Nov-1991

Revision History:
- Mike Griese (migrie) Jan 2018:
    Refactored the history and alias functionality into their own files.
Notes:
    The input model for the command line editing popups is complex.
    Here is the relevant pseudocode:

    CookedReadWaitRoutine
        if (CookedRead->Popup)
            Status = (*CookedRead->Popup->PopupInputRoutine)();
            if (Status == CONSOLE_STATUS_READ_COMPLETE)
                return STATUS_SUCCESS;
            return Status;

    CookedRead
        if (Command Line Editing Key)
            ProcessCommandLine
        else
            process regular key

    ProcessCommandLine
        if F7
            return CommandLinePopup

    CommandLinePopup
        draw popup
        return ProcessCommandListInput

    ProcessCommandListInput
        while (TRUE)
            GetChar
            if (wait)
                return wait
            switch (char)
                .
                .
                .
--*/

#pragma once

#include "input.h"
#include "screenInfo.hpp"
#include "server.h"

#include "history.h"
#include "alias.h"
#include "readDataCooked.hpp"

#define DEFAULT_NUMBER_OF_COMMANDS 25
#define DEFAULT_NUMBER_OF_BUFFERS 4

typedef NTSTATUS(*PCLE_POPUP_INPUT_ROUTINE) (_In_ COOKED_READ_DATA* CookedReadData,
                                             _Inout_opt_ IWaitRoutine** ppWaiter,
                                             _In_ bool WaitRoutine);

typedef struct _CLE_POPUP
{
    LIST_ENTRY ListLink;    // pointer to next popup
    SMALL_RECT Region;  // region popup occupies
    TextAttribute Attributes;    // text attributes
    PCHAR_INFO OldContents; // contains data under popup
    SHORT BottomIndex;  // number of command displayed on last line of popup
    SHORT CurrentCommand;
    WCHAR NumberBuffer[6];
    SHORT NumberRead;
    PCLE_POPUP_INPUT_ROUTINE PopupInputRoutine; // routine to call when input is received
    COORD OldScreenSize;
} CLE_POPUP, *PCLE_POPUP;


#define CLE_NO_POPUPS(CommandHistory) (&(CommandHistory)->PopupList == (CommandHistory)->PopupList.Blink)

class CommandLine
{
public:
    ~CommandLine();

    static CommandLine& Instance();

    bool IsEditLineEmpty() const;
    void Hide(const bool fUpdateFields);
    void Show();

protected:
    CommandLine();

    // delete these because we don't want to accidentally get copies of the singleton
    CommandLine(CommandLine const&) = delete;
    CommandLine& operator=(CommandLine const&) = delete;
};

[[nodiscard]]
NTSTATUS ProcessCommandLine(_In_ COOKED_READ_DATA* pCookedReadData,
                            _In_ WCHAR wch,
                            const DWORD dwKeyState);

void DeleteCommandLine(_Inout_ COOKED_READ_DATA* pCookedReadData, const bool fUpdateFields);

void RedrawCommandLine(_Inout_ COOKED_READ_DATA* CookedReadData);

[[nodiscard]]
NTSTATUS EndPopup(SCREEN_INFORMATION& screenInfo, _In_ CommandHistory* CommandHistory);

// Values for WriteChars(), WriteCharsLegacy() dwFlags
#define WC_DESTRUCTIVE_BACKSPACE 0x01
#define WC_KEEP_CURSOR_VISIBLE   0x02
#define WC_ECHO                  0x04

// This is no longer necessary. The buffer will always be Unicode. We don't need to perform special work to check if we're in a raster font
// and convert the entire buffer to match (and all insertions).
//#define WC_FALSIFY_UNICODE       0x08

#define WC_LIMIT_BACKSPACE       0x10
#define WC_NONDESTRUCTIVE_TAB    0x20
//#define WC_NEWLINE_SAVE_X        0x40  -  This has been replaced with an output mode flag instead as it's line discipline behavior that may not necessarily be coupled with VT.
#define WC_DELAY_EOL_WRAP        0x80

// Word delimiters
bool IsWordDelim(const WCHAR wch);
bool IsWordDelim(const std::vector<wchar_t>& charData);

[[nodiscard]]
HRESULT DoSrvSetConsoleTitleW(_In_reads_or_z_(cchBuffer) const wchar_t* const pwsBuffer,
                              const size_t cchBuffer);

#define COMMAND_NUM_TO_INDEX(NUM, CMDHIST) (SHORT)(((NUM+(CMDHIST)->FirstCommand)%((CMDHIST)->MaximumNumberOfCommands)))
#define COMMAND_INDEX_TO_NUM(INDEX, CMDHIST) (SHORT)(((INDEX+((CMDHIST)->MaximumNumberOfCommands)-(CMDHIST)->FirstCommand)%((CMDHIST)->MaximumNumberOfCommands)))

bool IsValidStringBuffer(_In_ bool Unicode, _In_reads_bytes_(Size) PVOID Buffer, _In_ ULONG Size, _In_ ULONG Count, ...);
