/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"
#include "CopyFromCharPopup.hpp"

#include "stream.h"
#include "_stream.h"
#include "resource.h"

static constexpr size_t COPY_FROM_CHAR_PROMPT_LENGTH = 28;

CopyFromCharPopup::CopyFromCharPopup(SCREEN_INFORMATION& screenInfo) :
    Popup(screenInfo, { COPY_FROM_CHAR_PROMPT_LENGTH + 2, 1 })
{
}

// Routine Description:
// - This routine handles the delete from cursor to char char popup.  It returns when we're out of input or the user has entered a char.
// Return Value:
// - CONSOLE_STATUS_WAIT - we ran out of input, so a wait block was created
// - CONSOLE_STATUS_READ_COMPLETE - user hit return
[[nodiscard]]
NTSTATUS CopyFromCharPopup::Process(COOKED_READ_DATA& cookedReadData) noexcept
{
    NTSTATUS Status = STATUS_SUCCESS;
    InputBuffer* const pInputBuffer = cookedReadData.GetInputBuffer();
    for (;;)
    {
        WCHAR Char;
        bool PopupKeys = false;
        Status = GetChar(pInputBuffer,
                         &Char,
                         TRUE,
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

        if (PopupKeys)
        {
            switch (Char)
            {
            case VK_ESCAPE:
                CommandLine::Instance().EndCurrentPopup();
                return CONSOLE_STATUS_WAIT_NO_BLOCK;
            }
        }

        CommandLine::Instance().EndCurrentPopup();

        size_t i;  // char index (not byte)
        // delete from cursor up to specified char
        for (i = cookedReadData._CurrentPosition + 1; i < (int)(cookedReadData._BytesRead / sizeof(WCHAR)); i++)
        {
            if (cookedReadData._BackupLimit[i] == Char)
            {
                break;
            }
        }

        if (i != (int)(cookedReadData._BytesRead / sizeof(WCHAR) + 1))
        {
            COORD CursorPosition;

            // save cursor position
            CursorPosition = cookedReadData.ScreenInfo().GetTextBuffer().GetCursor().GetPosition();

            // Delete commandline.
            DeleteCommandLine(cookedReadData, FALSE);

            // Delete chars.
            memmove(&cookedReadData._BackupLimit[cookedReadData._CurrentPosition],
                    &cookedReadData._BackupLimit[i],
                    cookedReadData._BytesRead - (i * sizeof(WCHAR)));
            cookedReadData._BytesRead -= (i - cookedReadData._CurrentPosition) * sizeof(WCHAR);

            // Write commandline.
            if (cookedReadData.IsEchoInput())
            {
                Status = WriteCharsLegacy(cookedReadData.ScreenInfo(),
                                          cookedReadData._BackupLimit,
                                          cookedReadData._BackupLimit,
                                          cookedReadData._BackupLimit,
                                          &cookedReadData._BytesRead,
                                          &cookedReadData.VisibleCharCount(),
                                          cookedReadData.OriginalCursorPosition().X,
                                          WC_DESTRUCTIVE_BACKSPACE | WC_KEEP_CURSOR_VISIBLE | WC_ECHO,
                                          nullptr);
                FAIL_FAST_IF_NTSTATUS_FAILED(Status);
            }

            // restore cursor position
            Status = cookedReadData.ScreenInfo().SetCursorPosition(CursorPosition, TRUE);
            FAIL_FAST_IF_NTSTATUS_FAILED(Status);
        }

        return CONSOLE_STATUS_WAIT_NO_BLOCK;
    }
}

void CopyFromCharPopup::_DrawContent()
{
    _DrawPrompt(ID_CONSOLE_MSGCMDLINEF4);
}
