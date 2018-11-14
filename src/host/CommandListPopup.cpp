/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"
#include "CommandListPopup.hpp"

#include "stream.h"
#include "_stream.h"
#include "cmdline.h"
#include "misc.h"
#include "_output.h"
#include "dbcs.h"
#include "../types/inc/GlyphWidth.hpp"

#include "..\interactivity\inc\ServiceLocator.hpp"

static constexpr size_t COMMAND_NUMBER_SIZE = 8;   // size of command number buffer

CommandListPopup::CommandListPopup(SCREEN_INFORMATION& screenInfo, const CommandHistory& history) :
    Popup(screenInfo, { 40, 10 }),
    _history{ history },
    _currentCommand{ history.LastDisplayed }
{
    if (_currentCommand < (SHORT)(_history.GetNumberOfCommands() - Height()))
    {
        _bottomIndex = std::max(_currentCommand, gsl::narrow<SHORT>(Height() - 1i16));
    }
    else
    {
        _bottomIndex = (SHORT)(_history.GetNumberOfCommands() - 1);
    }
}

NTSTATUS CommandListPopup::_handlePopupKeys(COOKED_READ_DATA& cookedReadData, const wchar_t wch)
{
    short Index = 0;
    switch (wch)
    {
    case VK_F9:
    {
        const HRESULT hr = CommandLine::Instance().StartCommandNumberPopup(cookedReadData);
        if (S_FALSE == hr)
        {
            // If we couldn't make the popup, break and go around to read another input character.
            break;
        }
        else
        {
            return hr;
        }
    }
    case VK_ESCAPE:
        CommandLine::Instance().EndCurrentPopup();
        return CONSOLE_STATUS_WAIT_NO_BLOCK;
    case VK_UP:
        _update(-1);
        break;
    case VK_DOWN:
        _update(1);
        break;
    case VK_END:
        // Move waaay forward, UpdateCommandListPopup() can handle it.
        _update((SHORT)(cookedReadData.History().GetNumberOfCommands()));
        break;
    case VK_HOME:
        // Move waaay back, UpdateCommandListPopup() can handle it.
        _update(-(SHORT)(cookedReadData.History().GetNumberOfCommands()));
        break;
    case VK_PRIOR:
        _update(-(SHORT)Height());
        break;
    case VK_NEXT:
        _update((SHORT)Height());
        break;
    case VK_LEFT:
    case VK_RIGHT:
        Index = _currentCommand;
        CommandLine::Instance().EndCurrentPopup();
        SetCurrentCommandLine(cookedReadData, (SHORT)Index);
        return CONSOLE_STATUS_WAIT_NO_BLOCK;
    default:
        break;
    }
    return STATUS_SUCCESS;
}

void CommandListPopup::_handleReturn(COOKED_READ_DATA& cookedReadData)
{
    short Index = 0;
    CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    NTSTATUS Status = STATUS_SUCCESS;
    DWORD LineCount = 1;
    Index = _currentCommand;
    CommandLine::Instance().EndCurrentPopup();
    SetCurrentCommandLine(cookedReadData, (SHORT)Index);
    cookedReadData.ProcessInput(UNICODE_CARRIAGERETURN, 0, Status);
    // complete read
    if (cookedReadData.IsEchoInput())
    {
        // check for alias
        cookedReadData.ProcessAliases(LineCount);
    }

    Status = STATUS_SUCCESS;
    size_t NumBytes;
    if (cookedReadData._BytesRead > cookedReadData._UserBufferSize || LineCount > 1)
    {
        if (LineCount > 1)
        {
            PWSTR Tmp;
            for (Tmp = cookedReadData._BackupLimit; *Tmp != UNICODE_LINEFEED; Tmp++)
            {
                FAIL_FAST_IF(!(Tmp < (cookedReadData._BackupLimit + cookedReadData._BytesRead)));
            }
            NumBytes = (Tmp - cookedReadData._BackupLimit + 1) * sizeof(*Tmp);
        }
        else
        {
            NumBytes = cookedReadData._UserBufferSize;
        }

        // Copy what we can fit into the user buffer
        memmove(cookedReadData._UserBuffer, cookedReadData._BackupLimit, NumBytes);

        // Store all of the remaining as pending until the next read operation.
        INPUT_READ_HANDLE_DATA* const pInputReadHandleData = cookedReadData.GetInputReadHandleData();
        const std::wstring_view pending{ cookedReadData._BackupLimit + (NumBytes / sizeof(wchar_t)),
                                            (cookedReadData._BytesRead - NumBytes) / sizeof(wchar_t) };
        if (LineCount > 1)
        {
            pInputReadHandleData->SaveMultilinePendingInput(pending);
        }
        else
        {
            pInputReadHandleData->SavePendingInput(pending);
        }
    }
    else
    {
        NumBytes = cookedReadData._BytesRead;
        memmove(cookedReadData._UserBuffer, cookedReadData._BackupLimit, NumBytes);
    }

    if (!cookedReadData.IsUnicode())
    {
        try
        {
            auto buffer = std::make_unique<char[]>(NumBytes);
            THROW_IF_NULL_ALLOC(buffer.get());

            NumBytes = ConvertToOem(gci.CP,
                                    cookedReadData._UserBuffer,
                                    gsl::narrow<UINT>(NumBytes / sizeof(WCHAR)),
                                    buffer.get(),
                                    gsl::narrow<UINT>(NumBytes));
            memmove(cookedReadData._UserBuffer, buffer.get(), NumBytes);
        }
        CATCH_LOG();
    }

    *(cookedReadData.pdwNumBytes) = NumBytes;
}

void CommandListPopup::_cycleSelectionToMatchingCommands(COOKED_READ_DATA& cookedReadData, const wchar_t wch)
{
    short Index = 0;
    if (cookedReadData.History().FindMatchingCommand({ &wch, 1 },
                                                     _currentCommand,
                                                     Index,
                                                     CommandHistory::MatchOptions::JustLooking))
    {
        _update((SHORT)(Index - _currentCommand), true);
    }
}

// Routine Description:
// - This routine handles the command list popup.  It returns when we're out of input or the user has selected a command line.
// Return Value:
// - CONSOLE_STATUS_WAIT - we ran out of input, so a wait block was created
// - CONSOLE_STATUS_READ_COMPLETE - user hit return
[[nodiscard]]
NTSTATUS CommandListPopup::Process(COOKED_READ_DATA& cookedReadData) noexcept
{
    NTSTATUS Status = STATUS_SUCCESS;

    for (;;)
    {
        WCHAR wch = UNICODE_NULL;
        bool popupKeys = false;

        Status = _getUserInput(cookedReadData, popupKeys, wch);
        if (!NT_SUCCESS(Status))
        {
            return Status;
        }

        if (popupKeys)
        {
            Status = _handlePopupKeys(cookedReadData, wch);
            if (Status != STATUS_SUCCESS)
            {
                return Status;
            }
        }
        else if (wch == UNICODE_CARRIAGERETURN)
        {
            _handleReturn(cookedReadData);
            return CONSOLE_STATUS_READ_COMPLETE;
        }
        else
        {
            // cycle through commands that start with the letter of the key pressed
            _cycleSelectionToMatchingCommands(cookedReadData, wch);
        }
    }
}

void CommandListPopup::_DrawContent()
{
    _drawList();
}

// Routine Description:
// - Draws a list of commands for the user to choose from
void CommandListPopup::_drawList()
{
    // draw empty popup
    COORD WriteCoord;
    WriteCoord.X = _region.Left + 1i16;
    WriteCoord.Y = _region.Top + 1i16;
    size_t lStringLength = Width();
    for (SHORT i = 0; i < Height(); ++i)
    {
        const OutputCellIterator spaces(UNICODE_SPACE, _attributes, lStringLength);
        const auto result = _screenInfo.Write(spaces, WriteCoord);
        lStringLength = result.GetCellDistance(spaces);
        WriteCoord.Y += 1i16;
    }

    auto& api = ServiceLocator::LocateGlobals().api;

    WriteCoord.Y = _region.Top + 1i16;
    SHORT i = std::max(gsl::narrow<SHORT>(_bottomIndex - Height() + 1), 0i16);
    for (; i <= _bottomIndex; i++)
    {
        CHAR CommandNumber[COMMAND_NUMBER_SIZE];
        // Write command number to screen.
        if (0 != _itoa_s(i, CommandNumber, ARRAYSIZE(CommandNumber), 10))
        {
            return;
        }

        PCHAR CommandNumberPtr = CommandNumber;

        size_t CommandNumberLength;
        if (FAILED(StringCchLengthA(CommandNumberPtr, ARRAYSIZE(CommandNumber), &CommandNumberLength)))
        {
            return;
        }
        __assume_bound(CommandNumberLength);

        if (CommandNumberLength + 1 >= ARRAYSIZE(CommandNumber))
        {
            return;
        }

        CommandNumber[CommandNumberLength] = ':';
        CommandNumber[CommandNumberLength + 1] = ' ';
        CommandNumberLength += 2;
        if (CommandNumberLength > static_cast<ULONG>(Width()))
        {
            CommandNumberLength = static_cast<ULONG>(Width());
        }

        WriteCoord.X = _region.Left + 1i16;

        LOG_IF_FAILED(api.WriteConsoleOutputCharacterAImpl(_screenInfo,
                                                           { CommandNumberPtr, CommandNumberLength },
                                                           WriteCoord,
                                                           CommandNumberLength));

        // write command to screen
        auto command = _history.GetNth(i);
        lStringLength = command.size();
        {
            size_t lTmpStringLength = lStringLength;
            LONG lPopupLength = static_cast<LONG>(Width() - CommandNumberLength);
            PCWCHAR lpStr = command.data();
            while (lTmpStringLength--)
            {
                if (IsGlyphFullWidth(*lpStr++))
                {
                    lPopupLength -= 2;
                }
                else
                {
                    lPopupLength--;
                }

                if (lPopupLength <= 0)
                {
                    lStringLength -= lTmpStringLength;
                    if (lPopupLength < 0)
                    {
                        lStringLength--;
                    }

                    break;
                }
            }
        }

        WriteCoord.X = gsl::narrow<SHORT>(WriteCoord.X + CommandNumberLength);
        size_t used;
        LOG_IF_FAILED(api.WriteConsoleOutputCharacterWImpl(_screenInfo,
                                                           { command.data(), lStringLength },
                                                           WriteCoord,
                                                           used));

        // write attributes to screen
        if (i == _currentCommand)
        {
            WriteCoord.X = _region.Left + 1i16;
            // inverted attributes
            lStringLength = Width();
            TextAttribute inverted = _attributes;
            inverted.Invert();

            const OutputCellIterator it(inverted, lStringLength);
            const auto done = _screenInfo.Write(it, WriteCoord);

            lStringLength = done.GetCellDistance(it);
        }

        WriteCoord.Y += 1;
    }
}

// Routine Description:
// - For popup lists, will adjust the position of the highlighted item and
//   possibly scroll the list if necessary.
// Arguments:
// - originalDelta - The number of lines to move up or down
// - wrap - Down past the bottom or up past the top should wrap the command list
void CommandListPopup::_update(const SHORT originalDelta, const bool wrap)
{
    SHORT delta = originalDelta;
    if (delta == 0)
    {
        return;
    }
    SHORT const Size = Height();

    SHORT CurCmdNum = _currentCommand;
    SHORT NewCmdNum = CurCmdNum + delta;

    if (wrap)
    {
        // Modulo the number of commands to "circle" around if we went off the end.
        NewCmdNum %= _history.GetNumberOfCommands();
    }
    else
    {
        if (NewCmdNum >= gsl::narrow<SHORT>(_history.GetNumberOfCommands()))
        {
            NewCmdNum = gsl::narrow<SHORT>(_history.GetNumberOfCommands()) - 1i16;
        }
        else if (NewCmdNum < 0)
        {
            NewCmdNum = 0;
        }
    }
    delta = NewCmdNum - CurCmdNum;

    bool Scroll = false;
    // determine amount to scroll, if any
    if (NewCmdNum <= _bottomIndex - Size)
    {
        _bottomIndex += delta;
        if (_bottomIndex < Size - 1i16)
        {
            _bottomIndex = Size - 1i16;
        }
        Scroll = true;
    }
    else if (NewCmdNum > _bottomIndex)
    {
        _bottomIndex += delta;
        if (_bottomIndex >= gsl::narrow<SHORT>(_history.GetNumberOfCommands()))
        {
            _bottomIndex = gsl::narrow<SHORT>(_history.GetNumberOfCommands()) - 1i16;
        }
        Scroll = true;
    }

    // write commands to popup
    if (Scroll)
    {
        _currentCommand = NewCmdNum;
        _drawList();
    }
    else
    {
        _updateHighlight(_currentCommand, NewCmdNum);
        _currentCommand = NewCmdNum;
    }
}

// Routine Description:
// - Adjusts the highligted line in a list of commands
// Arguments:
// - OldCurrentCommand - The previous command highlighted
// - NewCurrentCommand - The new command to be highlighted.
void CommandListPopup::_updateHighlight(const SHORT OldCurrentCommand, const SHORT NewCurrentCommand)
{
    SHORT TopIndex;
    if (_bottomIndex < Height())
    {
        TopIndex = 0;
    }
    else
    {
        TopIndex = _bottomIndex - Height() + 1i16;
    }
    COORD WriteCoord;
    WriteCoord.X = _region.Left + 1i16;
    size_t lStringLength = Width();

    WriteCoord.Y = _region.Top + 1i16 + OldCurrentCommand - TopIndex;

    const OutputCellIterator it(_attributes, lStringLength);
    const auto done = _screenInfo.Write(it, WriteCoord);
    lStringLength = done.GetCellDistance(it);

    // highlight new command
    WriteCoord.Y = _region.Top + 1i16 + NewCurrentCommand - TopIndex;

    // inverted attributes
    TextAttribute inverted = _attributes;
    inverted.Invert();
    const OutputCellIterator itAttr(inverted, lStringLength);
    const auto doneAttr = _screenInfo.Write(itAttr, WriteCoord);
    lStringLength = done.GetCellDistance(itAttr);
}
