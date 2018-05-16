/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/
#include "precomp.h"

#include "history.h"

#include "_output.h"
#include "output.h"
#include "stream.h"
#include "_stream.h"
#include "dbcs.h"
#include "handle.h"
#include "misc.h"
#include "../types/inc/convert.hpp"
#include "srvinit.h"
#include "resource.h"

#include "ApiRoutines.h"

#include "..\interactivity\inc\ServiceLocator.hpp"

#pragma hdrstop


// Routine Description:
// - This routine marks the command history buffer freed.
// Arguments:
// - Console - pointer to console.
// - ProcessHandle - handle to client process.
// Return Value:
// - <none>
PCOMMAND_HISTORY FindCommandHistory(const HANDLE hProcess)
{
    CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    PLIST_ENTRY const ListHead = &gci.CommandHistoryList;
    PLIST_ENTRY ListNext = ListHead->Flink;
    while (ListNext != ListHead)
    {
        PCOMMAND_HISTORY const History = CONTAINING_RECORD(ListNext, COMMAND_HISTORY, ListLink);
        ListNext = ListNext->Flink;
        if (History->ProcessHandle == hProcess)
        {
            FAIL_FAST_IF_FALSE(IsFlagSet(History->Flags, CLE_ALLOCATED));
            return History;
        }
    }

    return nullptr;
}

// Routine Description:
// - This routine marks the command history buffer freed.
// Arguments:
// - hProcess - handle to client process.
// Return Value:
// - <none>
void FreeCommandHistory(const HANDLE hProcess)
{
    PCOMMAND_HISTORY const History = FindCommandHistory(hProcess);
    if (History)
    {
        History->Flags &= ~CLE_ALLOCATED;
        History->ProcessHandle = nullptr;
    }
}

void FreeCommandHistoryBuffers()
{
    CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    PLIST_ENTRY const ListHead = &gci.CommandHistoryList;
    PLIST_ENTRY ListNext = ListHead->Flink;
    while (ListNext != ListHead)
    {
        PCOMMAND_HISTORY History = CONTAINING_RECORD(ListNext, COMMAND_HISTORY, ListLink);
        ListNext = ListNext->Flink;

        if (History)
        {

            RemoveEntryList(&History->ListLink);

            for (SHORT i = 0; i < History->NumberOfCommands; i++)
            {
                delete[] History->Commands[i];
                History->Commands[i] = nullptr;
            }

            delete[] History;
            History = nullptr;
        }
    }
}

void ResizeCommandHistoryBuffers(const UINT cCommands)
{
    CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    FAIL_FAST_IF_FALSE(cCommands <= SHORT_MAX);
    gci.SetHistoryBufferSize(cCommands);

    PLIST_ENTRY const ListHead = &gci.CommandHistoryList;
    PLIST_ENTRY ListNext = ListHead->Flink;
    while (ListNext != ListHead)
    {
        PCOMMAND_HISTORY const History = CONTAINING_RECORD(ListNext, COMMAND_HISTORY, ListLink);
        ListNext = ListNext->Flink;

        PCOMMAND_HISTORY const NewHistory = ReallocCommandHistory(History, cCommands);
        COOKED_READ_DATA* const CookedReadData = gci.lpCookedReadData;
        if (CookedReadData && CookedReadData->_CommandHistory == History)
        {
            CookedReadData->_CommandHistory = NewHistory;
        }
    }
}

static bool CaseInsensitiveEquality(wchar_t a, wchar_t b)
{
    return ::towlower(a) == ::towlower(b);
}

bool _COMMAND_HISTORY::IsAppNameMatch(const std::wstring_view other) const
{
    return std::equal(_appName.cbegin(), _appName.cend(), other.cbegin(), other.cend(), CaseInsensitiveEquality);
}

// Routine Description:
// - This routine is called when escape is entered or a command is added.
void ResetCommandHistory(_In_opt_ PCOMMAND_HISTORY CommandHistory)
{
    if (CommandHistory == nullptr)
    {
        return;
    }
    CommandHistory->LastDisplayed = CommandHistory->LastAdded;
    CommandHistory->Flags |= CLE_RESET;
}

[[nodiscard]]
NTSTATUS AddCommand(_In_ PCOMMAND_HISTORY pCmdHistory,
                    _In_reads_bytes_(cbCommand) PCWCHAR pwchCommand,
                    const size_t cbCommand,
                    const bool fHistoryNoDup)
{
    if (pCmdHistory == nullptr || pCmdHistory->MaximumNumberOfCommands == 0)
    {
        return STATUS_NO_MEMORY;
    }

    FAIL_FAST_IF_FALSE(IsFlagSet(pCmdHistory->Flags, CLE_ALLOCATED));

    if (cbCommand == 0)
    {
        return STATUS_SUCCESS;
    }

    if (pCmdHistory->NumberOfCommands == 0 ||
        pCmdHistory->Commands[pCmdHistory->LastAdded]->CommandLength != cbCommand ||
        memcmp(pCmdHistory->Commands[pCmdHistory->LastAdded]->Command, pwchCommand, cbCommand))
    {

        PCOMMAND pCmdReuse = nullptr;

        if (fHistoryNoDup)
        {
            SHORT i;
            i = FindMatchingCommand(pCmdHistory, pwchCommand, cbCommand, pCmdHistory->LastDisplayed, FMCFL_EXACT_MATCH);
            if (i != -1)
            {
                pCmdReuse = RemoveCommand(pCmdHistory, i);
            }
        }

        // find free record.  if all records are used, free the lru one.
        if (pCmdHistory->NumberOfCommands < pCmdHistory->MaximumNumberOfCommands)
        {
            pCmdHistory->LastAdded += 1;
            pCmdHistory->NumberOfCommands++;
        }
        else
        {
            COMMAND_IND_INC(pCmdHistory->LastAdded, pCmdHistory);
            COMMAND_IND_INC(pCmdHistory->FirstCommand, pCmdHistory);
            delete[] pCmdHistory->Commands[pCmdHistory->LastAdded];
            if (pCmdHistory->LastDisplayed == pCmdHistory->LastAdded)
            {
                pCmdHistory->LastDisplayed = -1;
            }
        }

        // TODO: Fix Commands history accesses. See: http://osgvsowi/614402
        if (pCmdHistory->LastDisplayed == -1 ||
            pCmdHistory->Commands[pCmdHistory->LastDisplayed]->CommandLength != cbCommand ||
            memcmp(pCmdHistory->Commands[pCmdHistory->LastDisplayed]->Command, pwchCommand, cbCommand))
        {
            ResetCommandHistory(pCmdHistory);
        }

        // add command to array
        PCOMMAND* const ppCmd = &pCmdHistory->Commands[pCmdHistory->LastAdded];
        if (pCmdReuse)
        {
            *ppCmd = pCmdReuse;
        }
        else
        {
            *ppCmd = (PCOMMAND) new(std::nothrow) BYTE[cbCommand + sizeof(COMMAND)];
            if (*ppCmd == nullptr)
            {
                COMMAND_IND_PREV(pCmdHistory->LastAdded, pCmdHistory);
                pCmdHistory->NumberOfCommands -= 1;
                return STATUS_NO_MEMORY;
            }
            (*ppCmd)->CommandLength = cbCommand;
            memmove((*ppCmd)->Command, pwchCommand, cbCommand);
        }
    }
    pCmdHistory->Flags |= CLE_RESET; // remember that we've returned a cmd
    return STATUS_SUCCESS;
}

[[nodiscard]]
NTSTATUS RetrieveNthCommand(_In_ PCOMMAND_HISTORY CommandHistory,
                            _In_ SHORT Index, // index, not command number
                            _In_reads_bytes_(BufferSize) PWCHAR Buffer,
                            _In_ size_t BufferSize,
                            _Out_ size_t* const CommandSize)
{
    FAIL_FAST_IF_FALSE(Index < CommandHistory->NumberOfCommands);
    CommandHistory->LastDisplayed = Index;
    PCOMMAND const CommandRecord = CommandHistory->Commands[Index];
    if (CommandRecord->CommandLength > (USHORT) BufferSize)
    {
        *CommandSize = (USHORT)BufferSize; // room for CRLF?
    }
    else
    {
        *CommandSize = CommandRecord->CommandLength;
    }

    memmove(Buffer, CommandRecord->Command, *CommandSize);
    return STATUS_SUCCESS;
}

[[nodiscard]]
NTSTATUS RetrieveCommand(_In_ PCOMMAND_HISTORY CommandHistory,
                         _In_ WORD VirtualKeyCode,
                         _In_reads_bytes_(BufferSize) PWCHAR Buffer,
                         _In_ size_t BufferSize,
                         _Out_ size_t* const CommandSize)
{
    if (CommandHistory == nullptr)
    {
        return STATUS_UNSUCCESSFUL;
    }

    FAIL_FAST_IF_FALSE(IsFlagSet(CommandHistory->Flags, CLE_ALLOCATED));

    if (CommandHistory->NumberOfCommands == 0)
    {
        return STATUS_UNSUCCESSFUL;
    }

    if (CommandHistory->NumberOfCommands == 1)
    {
        CommandHistory->LastDisplayed = 0;
    }
    else if (VirtualKeyCode == VK_UP)
    {
        // if this is the first time for this read that a command has
        // been retrieved, return the current command.  otherwise, return
        // the previous command.
        if (CommandHistory->Flags & CLE_RESET)
        {
            CommandHistory->Flags &= ~CLE_RESET;
        }
        else
        {
            COMMAND_IND_PREV(CommandHistory->LastDisplayed, CommandHistory);
        }
    }
    else
    {
        COMMAND_IND_NEXT(CommandHistory->LastDisplayed, CommandHistory);
    }

    return RetrieveNthCommand(CommandHistory, CommandHistory->LastDisplayed, Buffer, BufferSize, CommandSize);
}


PCOMMAND GetLastCommand(_In_ PCOMMAND_HISTORY CommandHistory)
{
    if (CommandHistory->NumberOfCommands == 0)
    {
        return nullptr;
    }
    else
    {
        return CommandHistory->Commands[CommandHistory->LastDisplayed];
    }
}

void EmptyCommandHistory(_In_opt_ PCOMMAND_HISTORY CommandHistory)
{
    if (CommandHistory == nullptr)
    {
        return;
    }

    for (SHORT i = 0; i < CommandHistory->NumberOfCommands; i++)
    {
        delete[] CommandHistory->Commands[i];
    }

    CommandHistory->NumberOfCommands = 0;
    CommandHistory->LastAdded = -1;
    CommandHistory->LastDisplayed = -1;
    CommandHistory->FirstCommand = 0;
    CommandHistory->Flags = CLE_RESET;
}

bool AtFirstCommand(_In_ PCOMMAND_HISTORY CommandHistory)
{
    if (CommandHistory == nullptr)
    {
        return FALSE;
    }

    if (CommandHistory->Flags & CLE_RESET)
    {
        return FALSE;
    }

    SHORT i = (SHORT)(CommandHistory->LastDisplayed - 1);
    if (i == -1)
    {
        i = (SHORT)(CommandHistory->NumberOfCommands - 1);
    }

    return (i == CommandHistory->LastAdded);
}

bool AtLastCommand(_In_ PCOMMAND_HISTORY CommandHistory)
{
    if (CommandHistory == nullptr)
    {
        return false;
    }
    else
    {
        return (CommandHistory->LastDisplayed == CommandHistory->LastAdded);
    }
}

PCOMMAND_HISTORY ReallocCommandHistory(_In_opt_ PCOMMAND_HISTORY CurrentCommandHistory, const size_t NumCommands)
{
    CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    // To protect ourselves from overflow and general arithmetic errors, a limit of SHORT_MAX is put on the size of the command history.
    if (CurrentCommandHistory == nullptr || CurrentCommandHistory->MaximumNumberOfCommands == (SHORT)NumCommands || NumCommands > SHORT_MAX)
    {
        return CurrentCommandHistory;
    }

    PCOMMAND_HISTORY const History = (PCOMMAND_HISTORY) new(std::nothrow) BYTE[sizeof(COMMAND_HISTORY) + NumCommands * sizeof(PCOMMAND)];
    if (History == nullptr)
    {
        return CurrentCommandHistory;
    }

    *History = *CurrentCommandHistory;
    History->Flags |= CLE_RESET;
    History->NumberOfCommands = std::min(History->NumberOfCommands, gsl::narrow<SHORT>(NumCommands));
    History->LastAdded = History->NumberOfCommands - 1;
    History->LastDisplayed = History->NumberOfCommands - 1;
    History->FirstCommand = 0;
    History->MaximumNumberOfCommands = (SHORT)NumCommands;
    int i;
    for (i = 0; i < History->NumberOfCommands; i++)
    {
        History->Commands[i] = CurrentCommandHistory->Commands[COMMAND_NUM_TO_INDEX(i, CurrentCommandHistory)];
    }
    for (; i < CurrentCommandHistory->NumberOfCommands; i++)
    {
#pragma prefast(suppress:6001, "Confused by 0 length array being used. This is fine until 0-size array is refactored.")
        delete[](CurrentCommandHistory->Commands[COMMAND_NUM_TO_INDEX(i, CurrentCommandHistory)]);
    }

    RemoveEntryList(&CurrentCommandHistory->ListLink);
    InitializeListHead(&History->PopupList);
    InsertHeadList(&gci.CommandHistoryList, &History->ListLink);

    delete[] CurrentCommandHistory;
    return History;
}

COMMAND_HISTORY* FindExeCommandHistory(const std::wstring_view appName)
{
    CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    PLIST_ENTRY const ListHead = &gci.CommandHistoryList;
    PLIST_ENTRY ListNext = ListHead->Flink;
    while (ListNext != ListHead)
    {
        PCOMMAND_HISTORY const History = CONTAINING_RECORD(ListNext, COMMAND_HISTORY, ListLink);
        ListNext = ListNext->Flink;

        if (IsFlagSet(History->Flags, CLE_ALLOCATED) && History->IsAppNameMatch(appName))
        {
            return History;
        }
    }
    return nullptr;
}

// Routine Description:
// - This routine returns the LRU command history buffer, or the command history buffer that corresponds to the app name.
// Arguments:
// - Console - pointer to console.
// Return Value:
// - Pointer to command history buffer.  if none are available, returns nullptr.
PCOMMAND_HISTORY COMMAND_HISTORY::s_AllocateCommandHistory(const std::wstring_view appName, const HANDLE processHandle)
{
    CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    // Reuse a history buffer.  The buffer must be !CLE_ALLOCATED.
    // If possible, the buffer should have the same app name.
    PLIST_ENTRY const ListHead = &gci.CommandHistoryList;
    PLIST_ENTRY ListNext = ListHead->Blink;
    PCOMMAND_HISTORY BestCandidate = nullptr;
    PCOMMAND_HISTORY History = nullptr;
    bool SameApp = false;
    while (ListNext != ListHead)
    {
        History = CONTAINING_RECORD(ListNext, COMMAND_HISTORY, ListLink);
        ListNext = ListNext->Blink;

        if (IsFlagClear(History->Flags, CLE_ALLOCATED))
        {
            // use LRU history buffer with same app name
            if (History->IsAppNameMatch(appName))
            {
                BestCandidate = History;
                SameApp = true;
                break;
            }

            // second best choice is LRU history buffer
            if (BestCandidate == nullptr)
            {
                BestCandidate = History;
            }
        }
    }

    // if there isn't a free buffer for the app name and the maximum number of
    // command history buffers hasn't been allocated, allocate a new one.
    if (!SameApp && gci.NumCommandHistories < gci.GetNumberOfHistoryBuffers())
    {
        size_t Size, TotalSize;

        if (FAILED(SizeTMult(gci.GetHistoryBufferSize(), sizeof(PCOMMAND), &Size)))
        {
            return nullptr;
        }

        if (FAILED(SizeTAdd(sizeof(COMMAND_HISTORY), Size, &TotalSize)))
        {
            return nullptr;
        }

        History = (PCOMMAND_HISTORY) new(std::nothrow) BYTE[TotalSize];
        if (History == nullptr)
        {
            return nullptr;
        }
        ZeroMemory(History, TotalSize * sizeof(BYTE));

        History->_appName = appName;
        History->Flags = CLE_ALLOCATED;
        History->NumberOfCommands = 0;
        History->LastAdded = -1;
        History->LastDisplayed = -1;
        History->FirstCommand = 0;
        History->MaximumNumberOfCommands = (SHORT)gci.GetHistoryBufferSize();
        InsertHeadList(&gci.CommandHistoryList, &History->ListLink);
        gci.NumCommandHistories += 1;
        History->ProcessHandle = processHandle;
        InitializeListHead(&History->PopupList);
        return History;
    }

    // If the app name doesn't match, copy in the new app name and free the old commands.
    if (BestCandidate)
    {
        History = BestCandidate;
        FAIL_FAST_IF_FALSE(CLE_NO_POPUPS(History));
        if (!SameApp)
        {
            for (SHORT i = 0; i < History->NumberOfCommands; i++)
            {
                delete[] History->Commands[i];
            }

            History->NumberOfCommands = 0;
            History->LastAdded = -1;
            History->LastDisplayed = -1;
            History->FirstCommand = 0;
            History->_appName = appName;
        }

        History->ProcessHandle = processHandle;
        SetFlag(History->Flags, CLE_ALLOCATED);

        // move to the front of the list
        RemoveEntryList(&BestCandidate->ListLink);
        InsertHeadList(&gci.CommandHistoryList, &BestCandidate->ListLink);
    }

    return BestCandidate;
}

PCOMMAND RemoveCommand(_In_ PCOMMAND_HISTORY CommandHistory, _In_ SHORT iDel)
{
    SHORT iFirst = CommandHistory->FirstCommand;
    SHORT iLast = CommandHistory->LastAdded;
    SHORT iDisp = CommandHistory->LastDisplayed;

    if (CommandHistory->NumberOfCommands == 0)
    {
        return nullptr;
    }

    SHORT const nDel = COMMAND_INDEX_TO_NUM(iDel, CommandHistory);
    if ((nDel < COMMAND_INDEX_TO_NUM(iFirst, CommandHistory)) || (nDel > COMMAND_INDEX_TO_NUM(iLast, CommandHistory)))
    {
        return nullptr;
    }

    if (iDisp == iDel)
    {
        CommandHistory->LastDisplayed = -1;
    }

    PCOMMAND* const ppcFirst = &(CommandHistory->Commands[iFirst]);
    PCOMMAND* const ppcDel = &(CommandHistory->Commands[iDel]);
    PCOMMAND const pcmdDel = *ppcDel;

    if (iDel < iLast)
    {
        memmove(ppcDel, ppcDel + 1, (iLast - iDel) * sizeof(PCOMMAND));
        if ((iDisp > iDel) && (iDisp <= iLast))
        {
            COMMAND_IND_DEC(iDisp, CommandHistory);
        }
        COMMAND_IND_DEC(iLast, CommandHistory);
    }
    else if (iFirst <= iDel)
    {
        memmove(ppcFirst + 1, ppcFirst, (iDel - iFirst) * sizeof(PCOMMAND));
        if ((iDisp >= iFirst) && (iDisp < iDel))
        {
            COMMAND_IND_INC(iDisp, CommandHistory);
        }
        COMMAND_IND_INC(iFirst, CommandHistory);
    }

    CommandHistory->FirstCommand = iFirst;
    CommandHistory->LastAdded = iLast;
    CommandHistory->LastDisplayed = iDisp;
    CommandHistory->NumberOfCommands--;
    return pcmdDel;
}


// Routine Description:
// - this routine finds the most recent command that starts with the letters already in the current command.  it returns the array index (no mod needed).
SHORT FindMatchingCommand(_In_ PCOMMAND_HISTORY CommandHistory,
                          _In_reads_bytes_(cbIn) PCWCHAR pwchIn,
                          _In_ size_t cbIn,
                          _In_ SHORT CommandIndex,  // where to start from
                          _In_ DWORD Flags)
{
    if (CommandHistory->NumberOfCommands == 0)
    {
        return -1;
    }

    if (!(Flags & FMCFL_JUST_LOOKING) && (CommandHistory->Flags & CLE_RESET))
    {
        CommandHistory->Flags &= ~CLE_RESET;
    }
    else
    {
        COMMAND_IND_PREV(CommandIndex, CommandHistory);
    }

    if (cbIn == 0)
    {
        return CommandIndex;
    }

    for (SHORT i = 0; i < CommandHistory->NumberOfCommands; i++)
    {
        PCOMMAND pcmdT = CommandHistory->Commands[CommandIndex];

        if ((IsFlagClear(Flags, FMCFL_EXACT_MATCH) && (cbIn <= pcmdT->CommandLength)) || ((USHORT)cbIn == pcmdT->CommandLength))
        {
            if (!wcsncmp(pcmdT->Command, pwchIn, (USHORT)cbIn / sizeof(WCHAR)))
            {
                return CommandIndex;
            }
        }

        COMMAND_IND_PREV(CommandIndex, CommandHistory);
    }

    return -1;
}

// Routine Description:
// - Clears all command history for the given EXE name
// - Will convert input parameters and call the W version of this method
// Arguments:
// - psExeNameBuffer - The client EXE application attached to the host whose history we should clear
// - cchExeNameBufferLength - Length in characters of EXE name buffer
// Return Value:
// - Check HRESULT with SUCCEEDED. Can return memory, safe math, safe string, or locale conversion errors.
HRESULT ApiRoutines::ExpungeConsoleCommandHistoryAImpl(_In_reads_or_z_(cchExeNameBufferLength) const char* const psExeNameBuffer,
                                                       const size_t cchExeNameBufferLength)
{
    const CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    wistd::unique_ptr<wchar_t[]> pwsExeName;
    size_t cchExeName;
    RETURN_IF_FAILED(ConvertToW(gci.CP, psExeNameBuffer, cchExeNameBufferLength, pwsExeName, cchExeName));


    return ExpungeConsoleCommandHistoryWImpl(pwsExeName.get(),
                                             cchExeName);
}

// Routine Description:
// - Clears all command history for the given EXE name
// Arguments:
// - pwsExeNameBuffer - The client EXE application attached to the host whose history we should clear
// - cchExeNameBufferLength - Length in characters of EXE name buffer
// Return Value:
// - Check HRESULT with SUCCEEDED. Can return memory, safe math, safe string, or locale conversion errors.
HRESULT ApiRoutines::ExpungeConsoleCommandHistoryWImpl(_In_reads_or_z_(cchExeNameBufferLength) const wchar_t* const pwsExeNameBuffer,
                                                       const size_t cchExeNameBufferLength)
{
    LockConsole();
    auto Unlock = wil::ScopeExit([&] { UnlockConsole(); });

    EmptyCommandHistory(FindExeCommandHistory({ pwsExeNameBuffer, cchExeNameBufferLength }));

    return S_OK;
}

// Routine Description:
// - Sets the number of commands that will be stored in history for a given EXE name
// - Will convert input parameters and call the W version of this method
// Arguments:
// - psExeNameBuffer - A client EXE application attached to the host
// - cchExeNameBufferLength - Length in characters of EXE name buffer
// - NumberOfCommands - Specifies the maximum length of the associated history buffer
// Return Value:
// - Check HRESULT with SUCCEEDED. Can return memory, safe math, safe string, or locale conversion errors.
HRESULT ApiRoutines::SetConsoleNumberOfCommandsAImpl(_In_reads_or_z_(cchExeNameBufferLength) const char* const psExeNameBuffer,
                                                     const size_t cchExeNameBufferLength,
                                                     const size_t NumberOfCommands)
{
    const CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    wistd::unique_ptr<wchar_t[]> pwsExeName;
    size_t cchExeName;
    RETURN_IF_FAILED(ConvertToW(gci.CP, psExeNameBuffer, cchExeNameBufferLength, pwsExeName, cchExeName));

    return SetConsoleNumberOfCommandsWImpl(pwsExeName.get(),
                                           cchExeName,
                                           NumberOfCommands);
}

// Routine Description:
// - Sets the number of commands that will be stored in history for a given EXE name
// Arguments:
// - pwsExeNameBuffer - A client EXE application attached to the host
// - cchExeNameBufferLength - Length in characters of EXE name buffer
// - NumberOfCommands - Specifies the maximum length of the associated history buffer
// Return Value:
// - Check HRESULT with SUCCEEDED. Can return memory, safe math, safe string, or locale conversion errors.
HRESULT ApiRoutines::SetConsoleNumberOfCommandsWImpl(_In_reads_or_z_(cchExeNameBufferLength) const wchar_t* const pwsExeNameBuffer,
                                                     const size_t cchExeNameBufferLength,
                                                     const size_t NumberOfCommands)
{
    LockConsole();
    auto Unlock = wil::ScopeExit([&] { UnlockConsole(); });

    ReallocCommandHistory(FindExeCommandHistory({ pwsExeNameBuffer, cchExeNameBufferLength }), NumberOfCommands);

    return S_OK;
}

// Routine Description:
// - Retrieves the amount of space needed to retrieve all command history for a given EXE name
// - Works for both Unicode and Multibyte text.
// - This method configuration is called for both A/W routines to allow us an efficient way of asking the system
//   the lengths of how long each conversion would be without actually performing the full allocations/conversions.
// Arguments:
// - pwsExeNameBuffer - The client EXE application attached to the host whose set we should check
// - cchExeNameBufferLength - Length in characters of EXE name buffer
// - fCountInUnicode - True for W version (UCS-2 Unicode) calls. False for A version calls (all multibyte formats.)
// - uiCodePage - Set to valid Windows Codepage for A version calls. Ignored for W (but typically just set to 0.)
// - pcchCommandHistoryLength - Pointer to receive the length of buffer that would be required to retrieve all history for the given exe.
// Return Value:
// - Check HRESULT with SUCCEEDED. Can return memory, safe math, safe string, or locale conversion errors.
HRESULT GetConsoleCommandHistoryLengthImplHelper(_In_reads_or_z_(cchExeNameBufferLength) const wchar_t* const pwsExeNameBuffer,
                                                 const size_t cchExeNameBufferLength,
                                                 const bool fCountInUnicode,
                                                 const UINT uiCodePage,
                                                 _Out_ size_t* const pcchCommandHistoryLength)
{
    // Ensure output variables are initialized
    *pcchCommandHistoryLength = 0;

    LockConsole();
    auto Unlock = wil::ScopeExit([&] { UnlockConsole(); });

    PCOMMAND_HISTORY const pCommandHistory = FindExeCommandHistory({ pwsExeNameBuffer, cchExeNameBufferLength });
    if (nullptr != pCommandHistory)
    {
        size_t cchNeeded = 0;

        // Every command history item is made of a string length followed by 1 null character.
        size_t const cchNull = 1;

        for (SHORT i = 0; i < pCommandHistory->NumberOfCommands; i++)
        {
            // Commands store lengths in bytes.
            size_t cchCommand = pCommandHistory->Commands[i]->CommandLength / sizeof(wchar_t);

            // This is the proposed length of the whole string.
            size_t cchProposed;
            RETURN_IF_FAILED(SizeTAdd(cchCommand, cchNull, &cchProposed));

            // If we're counting how much multibyte space will be needed, trial convert the command string before we add.
            if (!fCountInUnicode)
            {
                RETURN_IF_FAILED(GetALengthFromW(uiCodePage, pCommandHistory->Commands[i]->Command, cchCommand, &cchCommand));
            }

            // Accumulate the result
            RETURN_IF_FAILED(SizeTAdd(cchNeeded, cchProposed, &cchNeeded));
        }

        *pcchCommandHistoryLength = cchNeeded;
    }

    return S_OK;
}

// Routine Description:
// - Retrieves the amount of space needed to retrieve all command history for a given EXE name
// - Converts input text from A to W then makes the call to the W implementation.
// Arguments:
// - psExeNameBuffer - The client EXE application attached to the host whose set we should check
// - cchExeNameBufferLength - Length in characters of EXE name buffer
// - pcchCommandHistoryLength - Pointer to receive the length of buffer that would be required to retrieve all history for the given exe.
// Return Value:
// - Check HRESULT with SUCCEEDED. Can return memory, safe math, safe string, or locale conversion errors.
HRESULT ApiRoutines::GetConsoleCommandHistoryLengthAImpl(_In_reads_or_z_(cchExeNameBufferLength) const char* const psExeNameBuffer,
                                                         const size_t cchExeNameBufferLength,
                                                         _Out_ size_t* const pcchCommandHistoryLength)
{
    const CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    UINT const uiCodePage = gci.CP;

    // Ensure output variables are initialized
    *pcchCommandHistoryLength = 0;

    LockConsole();
    auto Unlock = wil::ScopeExit([&] { UnlockConsole(); });

    wistd::unique_ptr<wchar_t[]> pwsExeName;
    size_t cchExeName;
    RETURN_IF_FAILED(ConvertToW(uiCodePage, psExeNameBuffer, cchExeNameBufferLength, pwsExeName, cchExeName));

    return GetConsoleCommandHistoryLengthImplHelper(pwsExeName.get(), cchExeName, false, uiCodePage, pcchCommandHistoryLength);
}

// Routine Description:
// - Retrieves the amount of space needed to retrieve all command history for a given EXE name
// Arguments:
// - pwsExeNameBuffer - The client EXE application attached to the host whose set we should check
// - cchExeNameBufferLength - Length in characters of EXE name buffer
// - pcchCommandHistoryLength - Pointer to receive the length of buffer that would be required to retrieve all history for the given exe.
// Return Value:
// - Check HRESULT with SUCCEEDED. Can return memory, safe math, safe string, or locale conversion errors.
HRESULT ApiRoutines::GetConsoleCommandHistoryLengthWImpl(_In_reads_or_z_(cchExeNameBufferLength) const wchar_t* const pwsExeNameBuffer,
                                                         const size_t cchExeNameBufferLength,
                                                         _Out_ size_t* const pcchCommandHistoryLength)
{
    LockConsole();
    auto Unlock = wil::ScopeExit([&] { UnlockConsole(); });

    return GetConsoleCommandHistoryLengthImplHelper(pwsExeNameBuffer, cchExeNameBufferLength, true, 0, pcchCommandHistoryLength);
}

// Routine Description:
// - Retrieves a the full command history for a given EXE name known to the console.
// - It is permitted to call this function without having a target buffer. Use the result to allocate
//   the appropriate amount of space and call again.
// - This behavior exists to allow the A version of the function to help allocate the right temp buffer for conversion of
//   the output/result data.
// Arguments:
// - pwsExeNameBuffer - The client EXE application attached to the host whose set we should check
// - cchExeNameBufferLength - Length in characters of EXE name buffer
// - pwsCommandHistoryBuffer - The target buffer for data we are attempting to retrieve. Optionally nullptr to retrieve needed space.
// - cchCommandHistoryBufferLength - Length in characters of target buffer. Set to 0 when buffer is nullptr.
// - pcchCommandHistoryBufferWrittenOrNeeded - Pointer to space that will specify how many characters were written (if buffer is valid)
//                                             or how many characters would have been consumed.
// Return Value:
// - Check HRESULT with SUCCEEDED. Can return memory, safe math, safe string, or locale conversion errors.
HRESULT GetConsoleCommandHistoryWImplHelper(_In_reads_or_z_(cchExeNameBufferLength) const wchar_t* const pwsExeNameBuffer,
                                            const size_t cchExeNameBufferLength,
                                            _Out_writes_to_opt_(cchCommandHistoryBufferLength, *pcchCommandHistoryBufferWrittenOrNeeded) _Always_(_Post_z_) wchar_t* const pwsCommandHistoryBuffer,
                                            const size_t cchCommandHistoryBufferLength,
                                            _Out_ size_t* const pcchCommandHistoryBufferWrittenOrNeeded)
{
    // Ensure output variables are initialized
    *pcchCommandHistoryBufferWrittenOrNeeded = 0;
    if (nullptr != pwsCommandHistoryBuffer)
    {
        *pwsCommandHistoryBuffer = L'\0';
    }

    PCOMMAND_HISTORY const CommandHistory = FindExeCommandHistory({ pwsExeNameBuffer, cchExeNameBufferLength });

    if (nullptr != CommandHistory)
    {
        PWCHAR CommandBufferW = pwsCommandHistoryBuffer;

        size_t cchTotalLength = 0;

        size_t const cchNull = 1;

        for (SHORT i = 0; i < CommandHistory->NumberOfCommands; i++)
        {
            // Command stores length in bytes. Add 1 for null terminator.
            size_t const cchCommand = CommandHistory->Commands[i]->CommandLength / sizeof(wchar_t);

            size_t cchNeeded;
            RETURN_IF_FAILED(SizeTAdd(cchCommand, cchNull, &cchNeeded));

            // If we can return the data, attempt to do so until we're done or it overflows.
            // If we cannot return data, we're just going to loop anyway and count how much space we'd need.
            if (nullptr != pwsCommandHistoryBuffer)
            {
                // Calculate what the new total would be after we add what we need.
                size_t cchNewTotal;
                RETURN_IF_FAILED(SizeTAdd(cchTotalLength, cchNeeded, &cchNewTotal));

                RETURN_HR_IF(HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW), cchNewTotal > cchCommandHistoryBufferLength);

                size_t cchRemaining;
                RETURN_IF_FAILED(SizeTSub(cchCommandHistoryBufferLength,
                                          cchTotalLength,
                                          &cchRemaining));

                RETURN_IF_FAILED(StringCchCopyNW(CommandBufferW,
                                                 cchRemaining,
                                                 CommandHistory->Commands[i]->Command,
                                                 cchCommand));

                CommandBufferW += cchNeeded;
            }

            RETURN_IF_FAILED(SizeTAdd(cchTotalLength, cchNeeded, &cchTotalLength));
        }

        *pcchCommandHistoryBufferWrittenOrNeeded = cchTotalLength;
    }

    return S_OK;
}

// Routine Description:
// - Retrieves a the full command history for a given EXE name known to the console.
// - Converts inputs from A to W, calls the W version of this method, and then converts the resulting text W to A.
// Arguments:
// - psExeNameBuffer - The client EXE application attached to the host whose set we should check
// - cchExeNameBufferLength - Length in characters of EXE name buffer
// - psCommandHistoryBuffer - The target buffer for data we are attempting to retrieve.
// - cchCommandHistoryBufferLength - Length in characters of target buffer.
// - pcchCommandHistoryBufferWrittenOrNeeded - Pointer to space that will specify how many characters were written
// Return Value:
// - Check HRESULT with SUCCEEDED. Can return memory, safe math, safe string, or locale conversion errors.
HRESULT ApiRoutines::GetConsoleCommandHistoryAImpl(_In_reads_or_z_(cchExeNameBufferLength) const char* const psExeNameBuffer,
                                                   const size_t cchExeNameBufferLength,
                                                   _Out_writes_to_(cchCommandHistoryBufferLength, *pcchCommandHistoryBufferWritten) _Always_(_Post_z_) char* const psCommandHistoryBuffer,
                                                   const size_t cchCommandHistoryBufferLength,
                                                   _Out_ size_t* const pcchCommandHistoryBufferWritten)
{
    const CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    UINT const uiCodePage = gci.CP;

    // Ensure output variables are initialized
    *pcchCommandHistoryBufferWritten = 0;
    *psCommandHistoryBuffer = '\0';

    LockConsole();
    auto Unlock = wil::ScopeExit([&] { UnlockConsole(); });

    // Convert our input parameters to Unicode.
    wistd::unique_ptr<wchar_t[]> pwsExeName;
    size_t cchExeName;
    RETURN_IF_FAILED(ConvertToW(uiCodePage, psExeNameBuffer, cchExeNameBufferLength, pwsExeName, cchExeName));

    // Figure out how big our temporary Unicode buffer must be to retrieve output
    size_t cchCommandBufferNeeded;
    RETURN_IF_FAILED(GetConsoleCommandHistoryWImplHelper(pwsExeName.get(), cchExeName, nullptr, 0, &cchCommandBufferNeeded));

    // If there's nothing to get, then simply return.
    RETURN_HR_IF(S_OK, 0 == cchCommandBufferNeeded);

    // Allocate a unicode buffer of the right size.
    wistd::unique_ptr<wchar_t[]> pwsCommand = wil::make_unique_nothrow<wchar_t[]>(cchCommandBufferNeeded);
    RETURN_IF_NULL_ALLOC(pwsCommand);

    // Call the Unicode version of this method
    size_t cchCommandBufferWritten;
    RETURN_IF_FAILED(GetConsoleCommandHistoryWImplHelper(pwsExeName.get(), cchExeName, pwsCommand.get(), cchCommandBufferNeeded, &cchCommandBufferWritten));

    // Convert result to A
    wistd::unique_ptr<char[]> psConverted;
    size_t cchConverted;
    RETURN_IF_FAILED(ConvertToA(uiCodePage, pwsCommand.get(), cchCommandBufferWritten, psConverted, cchConverted));

    // Copy safely to output buffer
    // - CommandHistory are a series of null terminated strings. We cannot use a SafeString function to copy.
    //   So instead, validate and use raw memory copy.
    RETURN_HR_IF(HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW), cchConverted > cchCommandHistoryBufferLength);
    memcpy_s(psCommandHistoryBuffer, cchCommandHistoryBufferLength, psConverted.get(), cchConverted);

    // And return the size copied.
    *pcchCommandHistoryBufferWritten = cchConverted;

    return S_OK;
}

// Routine Description:
// - Retrieves a the full command history for a given EXE name known to the console.
// - Converts inputs from A to W, calls the W version of this method, and then converts the resulting text W to A.
// Arguments:
// - pwsExeNameBuffer - The client EXE application attached to the host whose set we should check
// - cchExeNameBufferLength - Length in characters of EXE name buffer
// - pwsCommandHistoryBuffer - The target buffer for data we are attempting to retrieve.
// - cchCommandHistoryBufferLength - Length in characters of target buffer.
// - pcchCommandHistoryBufferWrittenOrNeeded - Pointer to space that will specify how many characters were written
// Return Value:
// - Check HRESULT with SUCCEEDED. Can return memory, safe math, safe string, or locale conversion errors.
HRESULT ApiRoutines::GetConsoleCommandHistoryWImpl(_In_reads_or_z_(cchExeNameBufferLength) const wchar_t* const pwsExeNameBuffer,
                                                   const size_t cchExeNameBufferLength,
                                                   _Out_writes_to_(cchCommandHistoryBufferLength, *pcchCommandHistoryBufferWritten) _Always_(_Post_z_) wchar_t* const pwsCommandHistoryBuffer,
                                                   const size_t cchCommandHistoryBufferLength,
                                                   _Out_ size_t* const pcchCommandHistoryBufferWritten)
{
    LockConsole();
    auto Unlock = wil::ScopeExit([&] { UnlockConsole(); });

    return GetConsoleCommandHistoryWImplHelper(pwsExeNameBuffer, cchExeNameBufferLength, pwsCommandHistoryBuffer, cchCommandHistoryBufferLength, pcchCommandHistoryBufferWritten);
}
