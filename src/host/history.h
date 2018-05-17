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

// CommandHistory Flags
#define CLE_ALLOCATED 0x00000001
#define CLE_RESET     0x00000002

#pragma warning(disable:4200)
class CommandHistory
{
public:
    static CommandHistory* s_Allocate(const std::wstring_view appName, const HANDLE processHandle);
    static CommandHistory* s_Find(const HANDLE processHandle);
    static CommandHistory* s_FindByExe(const std::wstring_view appName);
    static CommandHistory* s_Realloc(CommandHistory* const current,
                                     const size_t commands);
    static void s_Free(const HANDLE processHandle);
    static void s_ResizeAll(const size_t commands);

    enum class MatchOptions
    {
        None = 0x0,
        ExactMatch = 0x1,
        JustLooking = 0x2
    };
    bool FindMatchingCommand(const std::wstring_view command,
                             const SHORT startingIndex,
                             SHORT& indexFound,
                             const MatchOptions options);
    bool IsAppNameMatch(const std::wstring_view other) const;

    [[nodiscard]]
    HRESULT Add(const std::wstring_view command,
                const bool suppressDuplicates);

    [[nodiscard]]
    HRESULT Retrieve(const WORD virtualKeyCode,
                     const gsl::span<wchar_t> buffer,
                     size_t& commandSize);

    [[nodiscard]]
    HRESULT RetrieveNth(const SHORT index,
                        const gsl::span<wchar_t> buffer,
                        size_t& commandSize);

    void Empty();

    bool AtFirstCommand() const;
    bool AtLastCommand() const;

    PCOMMAND GetLastCommand() const;

    SHORT IndexToNum(const SHORT index) const;
    SHORT NumToIndex(const SHORT index) const;

private:
    void _Reset();
    PCOMMAND _Remove(const SHORT iDel);

    // _Next and _Prev go to the next and prev command
    // _Inc  and _Dec go to the next and prev slots
    // Don't get the two confused - it matters when the cmd history is not full!
    void _Prev(SHORT& ind) const;
    void _Next(SHORT& ind) const;
    void _Dec(SHORT& ind) const;
    void _Inc(SHORT& ind) const;

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
};

DEFINE_ENUM_FLAG_OPERATORS(CommandHistory::MatchOptions);
