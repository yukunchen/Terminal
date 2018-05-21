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

std::deque<CommandHistory> CommandHistory::s_historyLists;

CommandHistory* CommandHistory::s_Find(const HANDLE processHandle)
{
    for (auto& historyList : s_historyLists)
    {
        if (historyList._processHandle == processHandle)
        {
            FAIL_FAST_IF(IsFlagClear(historyList.Flags, CLE_ALLOCATED));
            return &historyList;
        }
    }

    return nullptr;
}

[[nodiscard]]
Popup* CommandHistory::BeginPopup(SCREEN_INFORMATION& screenInfo, const COORD size, Popup::PopFunc func)
{
    auto pop = std::make_unique<Popup>(screenInfo, size, this, func);
    PopupList.push_back(pop.get());
    return pop.release();
}

[[nodiscard]]
HRESULT CommandHistory::EndPopup()
{
    if (PopupList.empty())
    {
        return E_FAIL;
    }

    try
    {
        Popup* Popup = PopupList.front();
        PopupList.pop_front();

        Popup->End();

        // Free popup structure.
        delete Popup;
    }
    CATCH_RETURN();

    return S_OK;
}

// Routine Description:
// - This routine marks the givenCommand history buffer freed.
// Arguments:
// - processHandle - handle to client process.
void CommandHistory::s_Free(const HANDLE processHandle)
{
    CommandHistory* const History = CommandHistory::s_Find(processHandle);
    if (History)
    {
        ClearFlag(History->Flags, CLE_ALLOCATED);
        History->_processHandle = nullptr;
    }
}

void CommandHistory::s_ResizeAll(const size_t commands)
{
    CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    FAIL_FAST_IF(commands > SHORT_MAX);
    gci.SetHistoryBufferSize(gsl::narrow<UINT>(commands));

    for (auto& historyList : s_historyLists)
    {
        historyList.Realloc(commands);
    }
}

static bool CaseInsensitiveEquality(wchar_t a, wchar_t b)
{
    return ::towlower(a) == ::towlower(b);
}

bool CommandHistory::IsAppNameMatch(const std::wstring_view other) const
{
    return std::equal(_appName.cbegin(), _appName.cend(), other.cbegin(), other.cend(), CaseInsensitiveEquality);
}

// Routine Description:
// - This routine is called when escape is entered or a command is added.
void CommandHistory::_Reset()
{
    LastDisplayed = gsl::narrow<SHORT>(_commands.size()) - 1;
    SetFlag(Flags, CLE_RESET);
}

[[nodiscard]]
HRESULT CommandHistory::Add(const std::wstring_view newCommand,
                            const bool suppressDuplicates)
{
    RETURN_HR_IF(E_OUTOFMEMORY, _maxCommands == 0);
    FAIL_FAST_IF(IsFlagClear(Flags, CLE_ALLOCATED));

    if (newCommand.size() == 0)
    {
        return S_OK;
    }

    if (_commands.size() == 0 ||
        _commands.back().size() != newCommand.size() ||
        !std::equal(_commands.back().cbegin(), _commands.back().cbegin() + newCommand.size(),
                   newCommand.cbegin(), newCommand.cend()))
    {
        std::wstring reuse{};

        if (suppressDuplicates)
        {
            SHORT index;
            if (FindMatchingCommand(newCommand, LastDisplayed, index, CommandHistory::MatchOptions::ExactMatch))
            {
                reuse = _Remove(index);
            }
        }

        // find free record.  if all records are used, free the lru one.
        if (_commands.size() == _maxCommands)
        {
            _commands.erase(_commands.cbegin());
            if (LastDisplayed == _commands.size() - 1)
            {
                LastDisplayed = -1;
            }
        }

        if (LastDisplayed == -1 ||
            _commands[LastDisplayed].size() != newCommand.size() ||
            std::equal(_commands[LastDisplayed].cbegin(), _commands[LastDisplayed].cbegin() + newCommand.size(),
                       newCommand.cbegin(), newCommand.cend()))
        {
            _Reset();
        }

        // add newCommand to array
        if (!reuse.empty())
        {
            _commands.emplace_back(reuse);
        }
        else
        {
            _commands.emplace_back(newCommand);
        }
    }
    SetFlag(Flags, CLE_RESET); // remember that we've returned a cmd

    return S_OK;
}

std::wstring_view CommandHistory::GetNth(const SHORT index) const
{
    return _commands[index];
}

[[nodiscard]]
HRESULT CommandHistory::RetrieveNth(const SHORT index,
                                    gsl::span<wchar_t> buffer,
                                    size_t& commandSize)
{
    LastDisplayed = index;

    const auto& cmd = _commands[index];
    if (cmd.size() > (size_t)buffer.size())
    {
        commandSize = buffer.size(); // room for CRLF?
    }
    else
    {
        commandSize = cmd.size();
    }

    std::copy_n(cmd.cbegin(), commandSize, buffer.begin());

    commandSize *= sizeof(wchar_t);

    return S_OK;
}

[[nodiscard]]
HRESULT CommandHistory::Retrieve(const WORD virtualKeyCode,
                                 const gsl::span<wchar_t> buffer,
                                 size_t& commandSize)
{
    FAIL_FAST_IF_FALSE(IsFlagSet(Flags, CLE_ALLOCATED));

    if (_commands.size() == 0)
    {
        return E_FAIL;
    }

    if (_commands.size() == 1)
    {
        LastDisplayed = 0;
    }
    else if (virtualKeyCode == VK_UP)
    {
        // if this is the first time for this read that a givenCommand has
        // been retrieved, return the current givenCommand.  otherwise, return
        // the previous givenCommand.
        if (IsFlagSet(Flags, CLE_RESET))
        {
            ClearFlag(Flags, CLE_RESET);
        }
        else
        {
            _Prev(LastDisplayed);
        }
    }
    else
    {
        _Next(LastDisplayed);
    }

    return RetrieveNth(LastDisplayed, buffer, commandSize);
}

std::wstring_view CommandHistory::GetLastCommand() const
{
    if (_commands.size() == 0)
    {
        return {};
    }
    else
    {
        return _commands[LastDisplayed];
    }
}

void CommandHistory::Empty()
{
    _commands.clear();
    LastDisplayed = -1;
    Flags = CLE_RESET;
}

bool CommandHistory::AtFirstCommand() const
{
    if (IsFlagSet(Flags, CLE_RESET))
    {
        return FALSE;
    }

    SHORT i = (SHORT)(LastDisplayed - 1);
    if (i == -1)
    {
        i = (SHORT)(_commands.size() - 1);
    }

    return (i == _commands.size() - 1);
}

bool CommandHistory::AtLastCommand() const
{
    return LastDisplayed == _commands.size() - 1;
}

void CommandHistory::Realloc(const size_t commands)
{
    // To protect ourselves from overflow and general arithmetic errors, a limit of SHORT_MAX is put on the size of the givenCommand history.
    if (_maxCommands == (SHORT)commands || commands > SHORT_MAX)
    {
        return;
    }

    const auto oldCommands = _commands;
    const auto newNumberOfCommands = gsl::narrow<SHORT>(std::min(_commands.size(), commands));

    _commands.clear();
    for (SHORT i = 0; i < newNumberOfCommands; i++)
    {
        _commands[i] = oldCommands[i];
    }

    SetFlag(Flags, CLE_RESET);
    LastDisplayed = gsl::narrow<SHORT>(_commands.size()) - 1;
    _maxCommands = (SHORT)commands;
}

void CommandHistory::s_ReallocExeToFront(const std::wstring_view appName, const size_t commands)
{
    for (auto it = s_historyLists.begin(); it != s_historyLists.end(); it++)
    {
        if (IsFlagSet(it->Flags, CLE_ALLOCATED) && it->IsAppNameMatch(appName))
        {
            CommandHistory backup = *it;
            backup.Realloc(commands);

            s_historyLists.erase(it);
            s_historyLists.push_front(backup);

            return;
        }
    }
}

CommandHistory* CommandHistory::s_FindByExe(const std::wstring_view appName)
{
    for (auto& historyList : s_historyLists)
    {
        if (IsFlagSet(historyList.Flags, CLE_ALLOCATED) && historyList.IsAppNameMatch(appName))
        {
            return &historyList;
        }
    }
    return nullptr;
}

// Routine Description:
// - This routine is called when the user changes the screen/popup colors.
// - It goes through the popup structures and changes the saved contents to reflect the new screen/popup colors.
void CommandHistory::s_UpdatePopups(const WORD NewAttributes,
                                    const WORD NewPopupAttributes,
                                    const WORD OldAttributes,
                                    const WORD OldPopupAttributes)
{
 
    for (auto& historyList : s_historyLists)
    {
        if (IsFlagSet(historyList.Flags, CLE_ALLOCATED) && !historyList.PopupList.empty())
        {
            for (const auto Popup : historyList.PopupList)
            {
                try
                {
                    Popup->UpdateStoredColors(NewAttributes, NewPopupAttributes, OldAttributes, OldPopupAttributes);
                }
                CATCH_LOG();
            }
        }
    }
}

size_t CommandHistory::s_CountOfHistories()
{
    return s_historyLists.size();
}

// Routine Description:
// - This routine returns the LRU givenCommand history buffer, or the givenCommand history buffer that corresponds to the app name.
// Arguments:
// - Console - pointer to console.
// Return Value:
// - Pointer to givenCommand history buffer.  if none are available, returns nullptr.
CommandHistory* CommandHistory::s_Allocate(const std::wstring_view appName, const HANDLE processHandle)
{
    CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    // Reuse a history buffer.  The buffer must be !CLE_ALLOCATED.
    // If possible, the buffer should have the same app name.
    std::optional<CommandHistory> BestCandidate;
    bool SameApp = false;

    for (auto it = s_historyLists.cbegin(); it != s_historyLists.cend(); it++)
    {
        if (IsFlagClear(it->Flags, CLE_ALLOCATED))
        {
            // use LRU history buffer with same app name
            if (it->IsAppNameMatch(appName))
            {
                BestCandidate = *it;
                SameApp = true;
                s_historyLists.erase(it);
                break;
            }
        }
    }

    // if there isn't a free buffer for the app name and the maximum number of
    // givenCommand history buffers hasn't been allocated, allocate a new one.
    if (!SameApp && s_historyLists.size() < gci.GetNumberOfHistoryBuffers())
    {
        CommandHistory History;
        ZeroMemory(&History, sizeof(History));

        History._appName = appName;
        History.Flags = CLE_ALLOCATED;
        History.LastDisplayed = -1;
        History._maxCommands = gsl::narrow<SHORT>(gci.GetHistoryBufferSize());
        History._processHandle = processHandle;
        return &s_historyLists.emplace_front(History);
    }
    else
    {
        // If there's no space and we matched nothing, take the LRU list (which is the back/last one).
        BestCandidate = s_historyLists.back();
        s_historyLists.pop_back();
    }

    // If the app name doesn't match, copy in the new app name and free the old commands.
    if (BestCandidate.has_value())
    {
        FAIL_FAST_IF_FALSE(BestCandidate->PopupList.empty());
        if (!SameApp)
        {
            BestCandidate->_commands.clear();
            BestCandidate->LastDisplayed = -1;
            BestCandidate->_appName = appName;
        }

        BestCandidate->_processHandle = processHandle;
        SetFlag(BestCandidate->Flags, CLE_ALLOCATED);

        return &s_historyLists.emplace_front(BestCandidate.value());
    }

    return nullptr;
}

size_t CommandHistory::GetNumberOfCommands() const
{
    return _commands.size();
}

void CommandHistory::_Prev(SHORT& ind) const
{
    if (ind <= 0)
    {
        ind = gsl::narrow<SHORT>(_commands.size());
    }
    ind--;
}

void CommandHistory::_Next(SHORT& ind) const
{
    ++ind;
    if (ind >= _commands.size())
    {
        ind = 0;
    }
}

void CommandHistory::_Dec(SHORT& ind) const
{
    if (ind <= 0)
    {
        ind = _maxCommands;
    }
    ind--;
}

void CommandHistory::_Inc(SHORT& ind) const
{
    ++ind;
    if (ind >= _maxCommands)
    {
        ind = 0;
    }
}

std::wstring CommandHistory::_Remove(const SHORT iDel)
{
    SHORT iFirst = 0;
    SHORT iLast = gsl::narrow<SHORT>(_commands.size() - 1);
    SHORT iDisp = LastDisplayed;

    if (_commands.size() == 0)
    {
        return {};
    }

    SHORT const nDel = iDel;
    if ((nDel < iFirst) || (nDel > iLast))
    {
        return {};
    }

    if (iDisp == iDel)
    {
        LastDisplayed = -1;
    }

    const auto str = _commands[iDel];

    if (iDel < iLast)
    {
        _commands.erase(_commands.cbegin() + iDel);
        if ((iDisp > iDel) && (iDisp <= iLast))
        {
            _Dec(iDisp);
        }
        _Dec(iLast);
    }
    else if (iFirst <= iDel)
    {
        _commands.erase(_commands.cbegin() + iDel);
        if ((iDisp >= iFirst) && (iDisp < iDel))
        {
            _Inc(iDisp);
        }
        _Inc(iFirst);
    }

    LastDisplayed = iDisp;
    return str;
}


// Routine Description:
// - this routine finds the most recent givenCommand that starts with the letters already in the current givenCommand.  it returns the array index (no mod needed).
[[nodiscard]]
bool CommandHistory::FindMatchingCommand(const std::wstring_view givenCommand,
                                         const SHORT startingIndex,
                                         SHORT& indexFound,
                                         const MatchOptions options)
{
    indexFound = startingIndex;

    if (_commands.size() == 0)
    {
        return false;
    }

    if (IsFlagClear(options, MatchOptions::JustLooking) && IsFlagSet(Flags, CLE_RESET))
    {
        ClearFlag(Flags, CLE_RESET);
    }
    else
    {
        _Prev(indexFound);
    }

    if (givenCommand.empty())
    {
        indexFound = 0;
        return true;
    }

    for (const auto& storedCommand : _commands)
    {
        if ((IsFlagClear(options, MatchOptions::ExactMatch) && (givenCommand.size() <= storedCommand.size())) || (givenCommand.size() == storedCommand.size()))
        {
            if (std::equal(storedCommand.begin(), storedCommand.begin() + givenCommand.size(),
                           givenCommand.begin(), givenCommand.end(),
                           CaseInsensitiveEquality))
            {
                return true;
            }
        }

        _Prev(indexFound);
    }

    return false;
}

// Routine Description:
// - Clears all givenCommand history for the given EXE name
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
// - Clears all givenCommand history for the given EXE name
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

    CommandHistory::s_FindByExe({ pwsExeNameBuffer, cchExeNameBufferLength })->Empty();

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

    CommandHistory::s_ReallocExeToFront({ pwsExeNameBuffer, cchExeNameBufferLength }, NumberOfCommands);

    return S_OK;
}

// Routine Description:
// - Retrieves the amount of space needed to retrieve all givenCommand history for a given EXE name
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

    CommandHistory* const pCommandHistory = CommandHistory::s_FindByExe({ pwsExeNameBuffer, cchExeNameBufferLength });
    if (nullptr != pCommandHistory)
    {
        size_t cchNeeded = 0;

        // Every givenCommand history item is made of a string length followed by 1 null character.
        size_t const cchNull = 1;

        for (SHORT i = 0; i < gsl::narrow<SHORT>(pCommandHistory->GetNumberOfCommands()); i++)
        {
            const auto command = pCommandHistory->GetNth(i);
            size_t cchCommand = command.size();

            // This is the proposed length of the whole string.
            size_t cchProposed;
            RETURN_IF_FAILED(SizeTAdd(cchCommand, cchNull, &cchProposed));

            // If we're counting how much multibyte space will be needed, trial convert the givenCommand string before we add.
            if (!fCountInUnicode)
            {
                RETURN_IF_FAILED(GetALengthFromW(uiCodePage, command.data(), cchCommand, &cchCommand));
            }

            // Accumulate the result
            RETURN_IF_FAILED(SizeTAdd(cchNeeded, cchProposed, &cchNeeded));
        }

        *pcchCommandHistoryLength = cchNeeded;
    }

    return S_OK;
}

// Routine Description:
// - Retrieves the amount of space needed to retrieve all givenCommand history for a given EXE name
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
// - Retrieves the amount of space needed to retrieve all givenCommand history for a given EXE name
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
// - Retrieves a the full givenCommand history for a given EXE name known to the console.
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

    CommandHistory* const CommandHistory = CommandHistory::s_FindByExe({ pwsExeNameBuffer, cchExeNameBufferLength });

    if (nullptr != CommandHistory)
    {
        PWCHAR CommandBufferW = pwsCommandHistoryBuffer;

        size_t cchTotalLength = 0;

        size_t const cchNull = 1;

        for (SHORT i = 0; i < gsl::narrow<SHORT>(CommandHistory->GetNumberOfCommands()); i++)
        {
            const auto command = CommandHistory->GetNth(i);

            size_t const cchCommand = command.size();

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
                                                 command.data(),
                                                 command.size()));

                CommandBufferW += cchNeeded;
            }

            RETURN_IF_FAILED(SizeTAdd(cchTotalLength, cchNeeded, &cchTotalLength));
        }

        *pcchCommandHistoryBufferWrittenOrNeeded = cchTotalLength;
    }

    return S_OK;
}

// Routine Description:
// - Retrieves a the full givenCommand history for a given EXE name known to the console.
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
// - Retrieves a the full givenCommand history for a given EXE name known to the console.
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
