/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"
#include "CommandNumberPopup.hpp"

#include "stream.h"
#include "_stream.h"
#include "cmdline.h"
#include "resource.h"

#include "..\interactivity\inc\ServiceLocator.hpp"

// 5 digit number for command history
static constexpr size_t COMMAND_NUMBER_LENGTH = 5;

static constexpr size_t COMMAND_NUMBER_PROMPT_LENGTH = 22;

CommandNumberPopup::CommandNumberPopup(SCREEN_INFORMATION& screenInfo) :
    Popup(screenInfo, { COMMAND_NUMBER_PROMPT_LENGTH + COMMAND_NUMBER_LENGTH, 1 })
{
    _userInput.reserve(COMMAND_NUMBER_LENGTH);
}

// Routine Description:
// - handles numerical user input
// Arguments:
// - cookedReadData - read data to operate on
// - wch - digit to handle
void CommandNumberPopup::_handleNumber(COOKED_READ_DATA& cookedReadData, const wchar_t wch) noexcept
{
    if (_userInput.size() < COMMAND_NUMBER_LENGTH)
    {
        size_t CharsToWrite = sizeof(wchar_t);
        const TextAttribute realAttributes = cookedReadData.ScreenInfo().GetAttributes();
        cookedReadData.ScreenInfo().SetAttributes(_attributes);
        size_t NumSpaces;
        FAIL_FAST_IF_NTSTATUS_FAILED(WriteCharsLegacy(cookedReadData.ScreenInfo(),
                                                      _userInput.data(),
                                                      _userInput.data() + _userInput.size(),
                                                      &wch,
                                                      &CharsToWrite,
                                                      &NumSpaces,
                                                      cookedReadData.OriginalCursorPosition().X,
                                                      WC_DESTRUCTIVE_BACKSPACE | WC_KEEP_CURSOR_VISIBLE | WC_ECHO,
                                                      nullptr));
        cookedReadData.ScreenInfo().SetAttributes(realAttributes);
        try
        {
            _push(wch);
        }
        CATCH_LOG();
    }
}

// Routine Description:
// - handles backspace user input. removes a digit from the user input
// Arguments:
// - cookedReadData - read data to operate on
void CommandNumberPopup::_handleBackspace(COOKED_READ_DATA& cookedReadData) noexcept
{
    if (_userInput.size() > 0)
    {
        size_t CharsToWrite = sizeof(WCHAR);
        const wchar_t backspace = UNICODE_BACKSPACE;
        const TextAttribute realAttributes = cookedReadData.ScreenInfo().GetAttributes();
        cookedReadData.ScreenInfo().SetAttributes(_attributes);
        size_t NumSpaces;
        FAIL_FAST_IF_NTSTATUS_FAILED(WriteCharsLegacy(cookedReadData.ScreenInfo(),
                                                      _userInput.data(),
                                                      _userInput.data() + _userInput.size(),
                                                      &backspace,
                                                      &CharsToWrite,
                                                      &NumSpaces,
                                                      cookedReadData.OriginalCursorPosition().X,
                                                      WC_DESTRUCTIVE_BACKSPACE | WC_KEEP_CURSOR_VISIBLE | WC_ECHO,
                                                      nullptr));
        cookedReadData.ScreenInfo().SetAttributes(realAttributes);
        _pop();
    }
}

// Routine Description:
// - handles escape user input. cancels the popup
// Arguments:
// - cookedReadData - read data to operate on
void CommandNumberPopup::_handleEscape(COOKED_READ_DATA& cookedReadData) noexcept
{
    CommandLine::Instance().EndCurrentPopup();

    // Note that cookedReadData's OriginalCursorPosition is the position before ANY text was entered on the edit line.
    // We want to use the position before the cursor was moved for this popup handler specifically, which may be *anywhere* in the edit line
    // and will be synchronized with the pointers in the cookedReadData structure (BufPtr, etc.)
    LOG_IF_FAILED(cookedReadData.ScreenInfo().SetCursorPosition(cookedReadData.BeforeDialogCursorPosition(), TRUE));
}

// Routine Description:
// - handles return user input. sets the prompt to the history item indicated
// Arguments:
// - cookedReadData - read data to operate on
short CommandNumberPopup::_handleReturn(COOKED_READ_DATA& cookedReadData) noexcept
{
    const short CommandNumber = gsl::narrow<short>(std::min(static_cast<size_t>(_parse()),
                                                            cookedReadData.History().GetNumberOfCommands() - 1));

    SetCurrentCommandLine(cookedReadData, CommandNumber);
    return CommandNumber;
}

// Routine Description:
// - This routine handles the command number selection popup.
// Return Value:
// - CONSOLE_STATUS_WAIT - we ran out of input, so a wait block was created
// - CONSOLE_STATUS_READ_COMPLETE - user hit return
[[nodiscard]]
NTSTATUS CommandNumberPopup::Process(COOKED_READ_DATA& cookedReadData) noexcept
{
    NTSTATUS Status = STATUS_SUCCESS;
    WCHAR Char = UNICODE_NULL;
    bool PopupKeys = false;

    for(;;)
    {
        Status = GetChar(cookedReadData.GetInputBuffer(),
                        &Char,
                        true,
                        nullptr,
                        &PopupKeys,
                        nullptr);
        if (!NT_SUCCESS(Status))
        {
            if (Status != CONSOLE_STATUS_WAIT)
            {
                cookedReadData._BytesRead = 0;
            }
            return Status;
        }

        bool endPopup = false;
        short parsedCommandNumber = 0;
        if (Char >= L'0' && Char <= L'9')
        {
            _handleNumber(cookedReadData, Char);
        }
        else if (Char == UNICODE_BACKSPACE)
        {
            _handleBackspace(cookedReadData);
        }
        else if (Char == (WCHAR)VK_ESCAPE)
        {
            _handleEscape(cookedReadData);
            endPopup = true;
        }
        else if (Char == UNICODE_CARRIAGERETURN)
        {
            parsedCommandNumber = _handleReturn(cookedReadData);
            endPopup = true;
        }
        if (endPopup)
        {
            // CommandNumberPopup can be a 2nd popup, make sure the calling popup goes away as well
            CommandLine::Instance().EndAllPopups();

            // ending a popup causes it to restore the text behind the popup. this may include the prompt
            // line, but we want to have effected a change to it. so after the popups are gone we redraw it
            // just in case
            SetCurrentCommandLine(cookedReadData, parsedCommandNumber);
            break;
        }
    }
    return CONSOLE_STATUS_WAIT_NO_BLOCK;
}

void CommandNumberPopup::_DrawContent()
{
    _DrawPrompt(ID_CONSOLE_MSGCMDLINEF9);
}

// Routine Description:
// - adds single digit number to the popup's number buffer
// Arguments:
// - wch - char of the number to add. must be in the range [L'0', L'9']
// Note: will throw if wch is out of range
void CommandNumberPopup::_push(const wchar_t wch)
{
    THROW_HR_IF(E_INVALIDARG, wch < L'0' || wch > L'9');
    if (_userInput.size() < COMMAND_NUMBER_LENGTH)
    {
        _userInput += wch;
    }
}

// Routine Description:
// - removes the last number added to the number buffer
void CommandNumberPopup::_pop() noexcept
{
    if (!_userInput.empty())
    {
        _userInput.pop_back();
    }
}

// Routine Description:
// - get numerical value for the data stored in the number buffer
// Return Value:
// - parsed integer representing the string value found in the number buffer
int CommandNumberPopup::_parse() const noexcept
{
    try
    {
        return std::stoi(_userInput);
    }
    catch (...)
    {
        return 0;
    }
}
