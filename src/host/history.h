/*++
Copyright (c) Microsoft Corporation

Module Name:
- history.h

Abstract:
- Encapsulates the cmdline functions and structures specifically related to
        command history functionality.
--*/

#pragma once

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

[[nodiscard]]
NTSTATUS AddCommand(_In_ PCOMMAND_HISTORY pCmdHistory,
                    _In_reads_bytes_(cbCommand) PCWCHAR pwchCommand,
                    const USHORT cbCommand,
                    const BOOL fHistoryNoDup);
PCOMMAND_HISTORY AllocateCommandHistory(_In_reads_bytes_(cbAppName) PCWSTR pwszAppName, const DWORD cbAppName, _In_ HANDLE hProcess);
void FreeCommandHistory(_In_ HANDLE const hProcess);
void FreeCommandHistoryBuffers();
void ResizeCommandHistoryBuffers(_In_ UINT const cCommands);
void EmptyCommandHistory(_In_opt_ PCOMMAND_HISTORY CommandHistory);
PCOMMAND_HISTORY ReallocCommandHistory(_In_opt_ PCOMMAND_HISTORY CurrentCommandHistory, _In_ DWORD const NumCommands);
PCOMMAND_HISTORY FindExeCommandHistory(_In_reads_(AppNameLength) PVOID AppName, _In_ DWORD AppNameLength, _In_ BOOLEAN const UnicodeExe);
BOOL AtFirstCommand(_In_ PCOMMAND_HISTORY CommandHistory);
BOOL AtLastCommand(_In_ PCOMMAND_HISTORY CommandHistory);
void EmptyCommandHistory(_In_opt_ PCOMMAND_HISTORY CommandHistory);
PCOMMAND GetLastCommand(_In_ PCOMMAND_HISTORY CommandHistory);
PCOMMAND RemoveCommand(_In_ PCOMMAND_HISTORY CommandHistory, _In_ SHORT iDel);
SHORT FindMatchingCommand(_In_ PCOMMAND_HISTORY CommandHistory,
                          _In_reads_bytes_(cbIn) PCWCHAR pwchIn,
                          _In_ ULONG cbIn,
                          _In_ SHORT CommandIndex,
                          _In_ DWORD Flags);
[[nodiscard]]
NTSTATUS RetrieveNthCommand(_In_ PCOMMAND_HISTORY CommandHistory,
                            _In_ SHORT Index,
                            _In_reads_bytes_(BufferSize)
                            PWCHAR Buffer,
                            _In_ ULONG BufferSize, _Out_ PULONG CommandSize);

// COMMAND_IND_NEXT and COMMAND_IND_PREV go to the next and prev command
// COMMAND_IND_INC  and COMMAND_IND_DEC  go to the next and prev slots
//
// Don't get the two confused - it matters when the cmd history is not full!
#define COMMAND_IND_PREV(IND, CMDHIST)               \
{                                                    \
    if (IND <= 0) {                                  \
        IND = (CMDHIST)->NumberOfCommands;           \
    }                                                \
    IND--;                                           \
}

#define COMMAND_IND_NEXT(IND, CMDHIST)               \
{                                                    \
    ++IND;                                           \
    if (IND >= (CMDHIST)->NumberOfCommands) {        \
        IND = 0;                                     \
    }                                                \
}

#define COMMAND_IND_DEC(IND, CMDHIST)                \
{                                                    \
    if (IND <= 0) {                                  \
        IND = (CMDHIST)->MaximumNumberOfCommands;    \
    }                                                \
    IND--;                                           \
}

#define COMMAND_IND_INC(IND, CMDHIST)                \
{                                                    \
    ++IND;                                           \
    if (IND >= (CMDHIST)->MaximumNumberOfCommands) { \
        IND = 0;                                     \
    }                                                \
}
