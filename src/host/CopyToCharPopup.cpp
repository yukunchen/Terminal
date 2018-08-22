/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"
#include "CopyToCharPopup.hpp"

#include "stream.h"
#include "_stream.h"
#include "resource.h"

static constexpr size_t COPY_TO_CHAR_PROMPT_LENGTH = 26;

CopyToCharPopup::CopyToCharPopup(SCREEN_INFORMATION& screenInfo) :
    Popup(screenInfo, { COPY_TO_CHAR_PROMPT_LENGTH + 2, 1 })
{
}

void CopyToCharPopup::_copyToChar(COOKED_READ_DATA& cookedReadData, const std::wstring_view LastCommand, const wchar_t wch)
{
    // uncomment the line below when starting work on modernizing this function
    //auto findResult = std::find(std::next(LastCommand.cbegin(), cookedReadData._CurrentPosition + 1), LastCommand.cend(), wch);
    size_t i;

    // find specified char in last command
    for (i = cookedReadData._CurrentPosition + 1; i < LastCommand.size(); i++)
    {
        if (LastCommand[i] == wch)
        {
            break;
        }
    }

    // If we found it, copy up to it.
    if (i < LastCommand.size() &&
        (LastCommand.size() > cookedReadData._CurrentPosition))
    {
        size_t j = i - cookedReadData._CurrentPosition;
        FAIL_FAST_IF_FALSE(j > 0);
        const auto bufferSpan = cookedReadData.SpanAtPointer();
        std::copy_n(LastCommand.cbegin() + cookedReadData._CurrentPosition,
                    j,
                    bufferSpan.begin());
        cookedReadData._CurrentPosition += j;
        j *= sizeof(WCHAR);
        cookedReadData._BytesRead = std::max(cookedReadData._BytesRead,
                                                cookedReadData._CurrentPosition * sizeof(WCHAR));
        if (cookedReadData.IsEchoInput())
        {
            size_t NumSpaces;
            SHORT ScrollY = 0;

            FAIL_FAST_IF_NTSTATUS_FAILED(WriteCharsLegacy(cookedReadData.ScreenInfo(),
                                                          cookedReadData._BackupLimit,
                                                          cookedReadData._BufPtr,
                                                          cookedReadData._BufPtr,
                                                          &j,
                                                          &NumSpaces,
                                                          cookedReadData.OriginalCursorPosition().X,
                                                          WC_DESTRUCTIVE_BACKSPACE | WC_KEEP_CURSOR_VISIBLE | WC_ECHO,
                                                          &ScrollY));
            cookedReadData.OriginalCursorPosition().Y += ScrollY;
            cookedReadData.VisibleCharCount() += NumSpaces;
        }

        cookedReadData._BufPtr += j / sizeof(WCHAR);
    }
}

// Routine Description:
// - This routine handles the delete char popup.  It returns when we're out of input or the user has entered a char.
// Return Value:
// - CONSOLE_STATUS_WAIT - we ran out of input, so a wait block was created
// - CONSOLE_STATUS_READ_COMPLETE - user hit return
[[nodiscard]]
NTSTATUS CopyToCharPopup::Process(COOKED_READ_DATA& cookedReadData) noexcept
{
    NTSTATUS Status = STATUS_SUCCESS;
    WCHAR Char = UNICODE_NULL;
    bool PopupKeys = false;
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
    else if (PopupKeys && Char == VK_ESCAPE)
    {
        CommandLine::Instance().EndCurrentPopup();
        return CONSOLE_STATUS_WAIT_NO_BLOCK;
    }

    // copy up to specified char
    const auto LastCommand = cookedReadData.History().GetLastCommand();
    if (!LastCommand.empty())
    {
        _copyToChar(cookedReadData, LastCommand, Char);
    }

    CommandLine::Instance().EndCurrentPopup();
    return CONSOLE_STATUS_WAIT_NO_BLOCK;
}

void CopyToCharPopup::_DrawContent()
{
    _DrawPrompt(ID_CONSOLE_MSGCMDLINEF2);
}
