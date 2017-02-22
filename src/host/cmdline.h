/*++
Copyright (c) Microsoft Corporation

Module Name:
- cmdline.h

Abstract:
- This file contains the internal structures and definitions used by command line input and editing.

Author:
- Therese Stowell (ThereseS) 15-Nov-1991

Revision History:

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

// Disable warning about 0 length MSFT compiler struct extension.
#pragma warning(disable:4200)
typedef struct _COMMAND
{
    USHORT CommandLength;
    WCHAR Command[0]; // TODO: refactor
} COMMAND, *PCOMMAND;

// COMMAND_HISTORY Flags
#define CLE_ALLOCATED 0x00000001
#define CLE_RESET     0x00000002

#pragma warning(disable:4200)
typedef struct _COMMAND_HISTORY
{
    LIST_ENTRY ListLink;
    DWORD Flags;
    PWCHAR AppName;
    SHORT NumberOfCommands;
    SHORT LastAdded;
    SHORT LastDisplayed;
    SHORT FirstCommand; // circular buffer
    SHORT MaximumNumberOfCommands;
    HANDLE ProcessHandle;
    LIST_ENTRY PopupList;   // pointer to top-level popup
    PCOMMAND Commands[0]; // TODO: refactor
} COMMAND_HISTORY, *PCOMMAND_HISTORY;

#define DEFAULT_NUMBER_OF_COMMANDS 25
#define DEFAULT_NUMBER_OF_BUFFERS 4

class COOKED_READ_DATA
{
public:
    InputBuffer* pInputBuffer;
    PSCREEN_INFORMATION pScreenInfo;
    ConsoleHandleData* pTempHandle;
    ULONG UserBufferSize;   // doubled size in ansi case
    PWCHAR UserBuffer;
    ULONG BufferSize;
    ULONG BytesRead;
    ULONG CurrentPosition;  // char position, not byte position
    PWCHAR BufPtr;
    PWCHAR BackupLimit;
    COORD OriginalCursorPosition;
    DWORD NumberOfVisibleChars;
    PCOMMAND_HISTORY CommandHistory;
    BOOLEAN Echo;
    BOOLEAN Processed;
    BOOLEAN Line;
    BOOLEAN InsertMode;
    ConsoleProcessHandle* pProcessData;
    INPUT_READ_HANDLE_DATA* pInputReadHandleData;
    PWCHAR ExeName;
    USHORT ExeNameLength;
    ULONG CtrlWakeupMask;
    ULONG ControlKeyState;
    COORD BeforeDialogCursorPosition; // Currently only used for F9 (ProcessCommandNumberInput) since it's the only pop-up to move the cursor when it starts.
};
typedef COOKED_READ_DATA *PCOOKED_READ_DATA;

typedef NTSTATUS(*PCLE_POPUP_INPUT_ROUTINE) (_In_ PCOOKED_READ_DATA CookedReadData, _In_ PCONSOLE_API_MSG WaitReplyMessage, _In_ BOOLEAN WaitRoutine);

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


#define CLE_NO_POPUPS(COMMAND_HISTORY) (&(COMMAND_HISTORY)->PopupList == (COMMAND_HISTORY)->PopupList.Blink)

//
// aliases are grouped per console, per exe.
//

typedef struct _ALIAS
{
    LIST_ENTRY ListLink;
    USHORT SourceLength;
    USHORT TargetLength;
    _Field_size_bytes_opt_(SourceLength) PWCHAR Source;
    _Field_size_bytes_(TargetLength) PWCHAR Target;
} ALIAS, *PALIAS;

typedef struct _EXE_ALIAS_LIST
{
    LIST_ENTRY ListLink;
    USHORT ExeLength;
    _Field_size_bytes_opt_(ExeLength) PWCHAR ExeName;
    LIST_ENTRY AliasList;
} EXE_ALIAS_LIST, *PEXE_ALIAS_LIST;

class CommandLine
{
public:
    ~CommandLine();

    static CommandLine& Instance();

    bool IsEditLineEmpty() const;
    void Hide(_In_ bool const fUpdateFields);
    void Show();

protected:
    CommandLine();

    // delete these because we don't want to accidentally get copies of the singleton
    CommandLine(CommandLine const&) = delete;
    CommandLine& operator=(CommandLine const&) = delete;
};

NTSTATUS ProcessCommandLine(_In_ PCOOKED_READ_DATA pCookedReadData,
                            _In_ WCHAR wch,
                            _In_ const DWORD dwKeyState,
                            _In_opt_ PCONSOLE_API_MSG pWaitReplyMessage,
                            _In_ const BOOLEAN fWaitRoutine);

void DeleteCommandLine(_Inout_ PCOOKED_READ_DATA pCookedReadData, _In_ const BOOL fUpdateFields);

void RedrawCommandLine(_Inout_ PCOOKED_READ_DATA CookedReadData);

PCOMMAND_HISTORY FindCommandHistory(_In_ const HANDLE hProcess);

bool IsCommandLinePopupKey(_In_ PKEY_EVENT_RECORD const pKeyEvent);

bool IsCommandLineEditingKey(_In_ PKEY_EVENT_RECORD const pKeyEvent);

void CleanUpPopups(_In_ PCOOKED_READ_DATA const CookedReadData);

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

// InitExtendedEditKey
// If lpwstr is nullptr, the default value will be used.
void InitExtendedEditKeys(_In_opt_ ExtKeyDefBuf const * const pKeyDefBuf);

// IsPauseKey
// returns TRUE if pKeyEvent is pause.
// The default key is Ctrl-S if extended edit keys are not specified.
bool IsPauseKey(_In_ PKEY_EVENT_RECORD const pKeyEvent);

// Word delimiters
#define IS_WORD_DELIM(wch)  ((wch) == L' ' || (gaWordDelimChars[0] && IsWordDelim(wch)))
bool IsWordDelim(_In_ WCHAR const wch);

HRESULT DoSrvSetConsoleTitleW(_In_reads_or_z_(cchBuffer) const wchar_t* const pwsBuffer,
                              _In_ size_t const cchBuffer);

NTSTATUS MatchAndCopyAlias(_In_reads_bytes_(cbSource) PWCHAR pwchSource,
                           _In_ USHORT cbSource,
                           _Out_writes_bytes_(*pcbTarget) PWCHAR pwchTarget,
                           _Inout_ PUSHORT pcbTarget,
                           _In_reads_bytes_(cbExe) PWCHAR pwchExe,
                           _In_ USHORT cbExe,
                           _Out_ PDWORD pcLines);

NTSTATUS AddCommand(_In_ PCOMMAND_HISTORY pCmdHistory,
                    _In_reads_bytes_(cbCommand) PCWCHAR pwchCommand,
                    _In_ const USHORT cbCommand,
                    _In_ const BOOL fHistoryNoDup);
PCOMMAND_HISTORY AllocateCommandHistory(_In_reads_bytes_(cbAppName) PCWSTR pwszAppName, _In_ const DWORD cbAppName, _In_ HANDLE hProcess);
void FreeAliasBuffers();
void FreeCommandHistory(_In_ HANDLE const hProcess);
void FreeCommandHistoryBuffers();
void ResizeCommandHistoryBuffers(_In_ UINT const cCommands);

int MyStringCompareW(_In_reads_bytes_(cbLength) const WCHAR *Str1,
                     _In_reads_bytes_(cbLength) const WCHAR *Str2,
                     _In_ const USHORT cbLength,
                     _In_ const BOOLEAN fCaseInsensitive);
