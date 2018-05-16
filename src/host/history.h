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
    size_t CommandLength;
    WCHAR Command[0]; // TODO: refactor
} COMMAND, *PCOMMAND;

// COMMAND_HISTORY Flags
#define CLE_ALLOCATED 0x00000001
#define CLE_RESET     0x00000002

#pragma warning(disable:4200)
typedef struct _COMMAND_HISTORY
{
    bool IsAppNameMatch(const std::wstring_view other) const;
    static _COMMAND_HISTORY* s_AllocateCommandHistory(const std::wstring_view appName, const HANDLE processHandle);


    enum class MatchOptions
    {
        None = 0x0,
        ExactMatch = 0x1,
        JustLooking = 0x2
    };
    bool FindMatchingCommand(const std::wstring_view command,
                             const size_t startingIndex,
                             size_t& indexFound,
                             const MatchOptions options);

private:
    std::wstring _appName;
public:

    LIST_ENTRY ListLink;
    DWORD Flags;
    SHORT NumberOfCommands;
    SHORT LastAdded;
    SHORT LastDisplayed;
    SHORT FirstCommand; // circular buffer
    SHORT MaximumNumberOfCommands;
    HANDLE ProcessHandle;
    LIST_ENTRY PopupList;   // pointer to top-level popup
    PCOMMAND Commands[0]; // TODO: refactor
} COMMAND_HISTORY, *PCOMMAND_HISTORY;

DEFINE_ENUM_FLAG_OPERATORS(_COMMAND_HISTORY::MatchOptions);

[[nodiscard]]
NTSTATUS AddCommand(_In_ PCOMMAND_HISTORY pCmdHistory,
                    _In_reads_bytes_(cbCommand) PCWCHAR pwchCommand,
                    const size_t cbCommand,
                    const bool fHistoryNoDup);

void FreeCommandHistory(const HANDLE hProcess);
void FreeCommandHistoryBuffers();
void ResizeCommandHistoryBuffers(const UINT cCommands);
void EmptyCommandHistory(_In_opt_ PCOMMAND_HISTORY CommandHistory);
PCOMMAND_HISTORY ReallocCommandHistory(_In_opt_ PCOMMAND_HISTORY CurrentCommandHistory, const size_t NumCommands);
COMMAND_HISTORY* FindExeCommandHistory(const std::wstring_view appName);
bool AtFirstCommand(_In_ PCOMMAND_HISTORY CommandHistory);
bool AtLastCommand(_In_ PCOMMAND_HISTORY CommandHistory);
void EmptyCommandHistory(_In_opt_ PCOMMAND_HISTORY CommandHistory);
PCOMMAND GetLastCommand(_In_ PCOMMAND_HISTORY CommandHistory);
PCOMMAND RemoveCommand(_In_ PCOMMAND_HISTORY CommandHistory, _In_ SHORT iDel);

[[nodiscard]]
NTSTATUS RetrieveNthCommand(_In_ PCOMMAND_HISTORY CommandHistory,
                            _In_ SHORT Index,
                            _In_reads_bytes_(BufferSize)
                            PWCHAR Buffer,
                            _In_ size_t BufferSize, _Out_ size_t* const CommandSize);

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
