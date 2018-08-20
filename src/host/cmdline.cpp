/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/
#include "precomp.h"

#include "cmdline.h"
#include "popup.h"

#include "_output.h"
#include "output.h"
#include "stream.h"
#include "_stream.h"
#include "dbcs.h"
#include "handle.h"
#include "misc.h"
#include "../types/inc/convert.hpp"
#include "srvinit.h"

#include "ApiRoutines.h"

#include "..\interactivity\inc\ServiceLocator.hpp"

#pragma hdrstop

[[nodiscard]]
HRESULT CommandNumberPopup(COOKED_READ_DATA& cookedReadData);

// Routine Description:
// - This routine validates a string buffer and returns the pointers of where the strings start within the buffer.
// Arguments:
// - Unicode - Supplies a boolean that is TRUE if the buffer contains Unicode strings, FALSE otherwise.
// - Buffer - Supplies the buffer to be validated.
// - Size - Supplies the size, in bytes, of the buffer to be validated.
// - Count - Supplies the expected number of strings in the buffer.
// ... - Supplies a pair of arguments per expected string. The first one is the expected size, in bytes, of the string
//       and the second one receives a pointer to where the string starts.
// Return Value:
// - TRUE if the buffer is valid, FALSE otherwise.
bool IsValidStringBuffer(_In_ bool Unicode, _In_reads_bytes_(Size) PVOID Buffer, _In_ ULONG Size, _In_ ULONG Count, ...)
{
    va_list Marker;
    va_start(Marker, Count);

    while (Count > 0)
    {
        ULONG const StringSize = va_arg(Marker, ULONG);
        PVOID* StringStart = va_arg(Marker, PVOID *);

        // Make sure the string fits in the supplied buffer and that it is properly aligned.
        if (StringSize > Size)
        {
            break;
        }

        if ((Unicode != false) && ((StringSize % sizeof(WCHAR)) != 0))
        {
            break;
        }

        *StringStart = Buffer;

        // Go to the next string.
        Buffer = RtlOffsetToPointer(Buffer, StringSize);
        Size -= StringSize;
        Count -= 1;
    }

    va_end(Marker);

    return Count == 0;
}

// Routine Description:
// - Detects Word delimiters
bool IsWordDelim(const wchar_t wch)
{
    // the space character is always a word delimiter. Do not add it to the WordDelimiters global because
    // that contains the user configurable word delimiters only.
    if (wch == UNICODE_SPACE)
    {
        return true;
    }
    const auto& delimiters = ServiceLocator::LocateGlobals().WordDelimiters;
    return std::find(delimiters.begin(), delimiters.end(), wch) != delimiters.end();
}

bool IsWordDelim(const std::wstring_view charData)
{
    if (charData.size() != 1)
    {
        return false;
    }
    return IsWordDelim(charData.front());
}

CommandLine::CommandLine()
{

}

CommandLine::~CommandLine()
{

}

CommandLine& CommandLine::Instance()
{
    static CommandLine c;
    return c;
}

bool CommandLine::IsEditLineEmpty() const
{
    const CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();

    if (!gci.HasPendingCookedRead())
    {
        // If the cooked read data pointer is null, there is no edit line data and therefore it's empty.
        return true;
    }
    else if (0 == gci.CookedReadData().VisibleCharCount())
    {
        // If we had a valid pointer, but there are no visible characters for the edit line, then it's empty.
        // Someone started editing and back spaced the whole line out so it exists, but has no data.
        return true;
    }
    else
    {
        return false;
    }
}

void CommandLine::Hide(const bool fUpdateFields)
{
    CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    if (!IsEditLineEmpty())
    {
        DeleteCommandLine(gci.CookedReadData(), fUpdateFields);
    }
}

void CommandLine::Show()
{
    CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    if (!IsEditLineEmpty())
    {
        RedrawCommandLine(gci.CookedReadData());
    }
}

void DeleteCommandLine(COOKED_READ_DATA& cookedReadData, const bool fUpdateFields)
{
    size_t CharsToWrite = cookedReadData.VisibleCharCount();
    COORD coordOriginalCursor = cookedReadData.OriginalCursorPosition();
    const COORD coordBufferSize = cookedReadData.ScreenInfo().GetScreenBufferSize();

    // catch the case where the current command has scrolled off the top of the screen.
    if (coordOriginalCursor.Y < 0)
    {
        CharsToWrite += coordBufferSize.X * coordOriginalCursor.Y;
        CharsToWrite += cookedReadData.OriginalCursorPosition().X;   // account for prompt
        cookedReadData.OriginalCursorPosition().X = 0;
        cookedReadData.OriginalCursorPosition().Y = 0;
        coordOriginalCursor.X = 0;
        coordOriginalCursor.Y = 0;
    }

    if (!CheckBisectStringW(cookedReadData._BackupLimit,
                            CharsToWrite,
                            coordBufferSize.X - cookedReadData.OriginalCursorPosition().X))
    {
        CharsToWrite++;
    }

    try
    {
        FillOutputW(cookedReadData.ScreenInfo(),
                    UNICODE_SPACE,
                    coordOriginalCursor,
                    CharsToWrite);
    }
    CATCH_LOG();

    if (fUpdateFields)
    {
        cookedReadData._BufPtr = cookedReadData._BackupLimit;
        cookedReadData._BytesRead = 0;
        cookedReadData._CurrentPosition = 0;
        cookedReadData.VisibleCharCount() = 0;
    }

    LOG_IF_FAILED(cookedReadData.ScreenInfo().SetCursorPosition(cookedReadData.OriginalCursorPosition(), true));
}

void RedrawCommandLine(COOKED_READ_DATA& cookedReadData)
{
    if (cookedReadData.IsEchoInput())
    {
        // Draw the command line
        cookedReadData.OriginalCursorPosition() = cookedReadData.ScreenInfo().GetTextBuffer().GetCursor().GetPosition();

        SHORT ScrollY = 0;
#pragma prefast(suppress:28931, "Status is not unused. It's used in debug assertions.")
        NTSTATUS Status = WriteCharsLegacy(cookedReadData.ScreenInfo(),
                                           cookedReadData._BackupLimit,
                                           cookedReadData._BackupLimit,
                                           cookedReadData._BackupLimit,
                                           &cookedReadData._BytesRead,
                                           &cookedReadData.VisibleCharCount(),
                                           cookedReadData.OriginalCursorPosition().X,
                                           WC_DESTRUCTIVE_BACKSPACE | WC_KEEP_CURSOR_VISIBLE | WC_ECHO,
                                           &ScrollY);
        FAIL_FAST_IF_NTSTATUS_FAILED(Status);

        cookedReadData.OriginalCursorPosition().Y += ScrollY;

        // Move the cursor back to the right position
        COORD CursorPosition = cookedReadData.OriginalCursorPosition();
        CursorPosition.X += (SHORT)RetrieveTotalNumberOfSpaces(cookedReadData.OriginalCursorPosition().X,
                                                               cookedReadData._BackupLimit,
                                                               cookedReadData._CurrentPosition);
        if (CheckBisectStringW(cookedReadData._BackupLimit,
                               cookedReadData._CurrentPosition,
                               cookedReadData.ScreenInfo().GetScreenBufferSize().X - cookedReadData.OriginalCursorPosition().X))
        {
            CursorPosition.X++;
        }
        Status = AdjustCursorPosition(cookedReadData.ScreenInfo(), CursorPosition, TRUE, nullptr);
        FAIL_FAST_IF_NTSTATUS_FAILED(Status);
    }
}

// Routine Description:
// - This routine copies the commandline specified by Index into the cooked read buffer
void SetCurrentCommandLine(COOKED_READ_DATA& cookedReadData, _In_ SHORT Index) // index, not command number
{
    DeleteCommandLine(cookedReadData, TRUE);
    FAIL_FAST_IF_FAILED(cookedReadData.History().RetrieveNth(Index,
                                                             cookedReadData.SpanWholeBuffer(),
                                                             cookedReadData._BytesRead));
    FAIL_FAST_IF_FALSE(cookedReadData._BackupLimit == cookedReadData._BufPtr);
    if (cookedReadData.IsEchoInput())
    {
        SHORT ScrollY = 0;
        FAIL_FAST_IF_NTSTATUS_FAILED(WriteCharsLegacy(cookedReadData.ScreenInfo(),
                                                      cookedReadData._BackupLimit,
                                                      cookedReadData._BufPtr,
                                                      cookedReadData._BufPtr,
                                                      &cookedReadData._BytesRead,
                                                      &cookedReadData.VisibleCharCount(),
                                                      cookedReadData.OriginalCursorPosition().X,
                                                      WC_DESTRUCTIVE_BACKSPACE | WC_KEEP_CURSOR_VISIBLE | WC_ECHO,
                                                      &ScrollY));
        cookedReadData.OriginalCursorPosition().Y += ScrollY;
    }

    size_t const CharsToWrite = cookedReadData._BytesRead / sizeof(WCHAR);
    cookedReadData._CurrentPosition = CharsToWrite;
    cookedReadData._BufPtr = cookedReadData._BackupLimit + CharsToWrite;
}

// Routine Description:
// - This routine handles the command list popup.  It returns when we're out of input or the user has selected a command line.
// Return Value:
// - CONSOLE_STATUS_WAIT - we ran out of input, so a wait block was created
// - CONSOLE_STATUS_READ_COMPLETE - user hit return
[[nodiscard]]
NTSTATUS ProcessCommandListInput(COOKED_READ_DATA& cookedReadData)
{
    const CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    CommandHistory& commandHistory = cookedReadData.History();
    Popup* const Popup = commandHistory.PopupList.front();
    NTSTATUS Status = STATUS_SUCCESS;
    INPUT_READ_HANDLE_DATA* const pInputReadHandleData = cookedReadData.GetInputReadHandleData();
    InputBuffer* const pInputBuffer = cookedReadData.GetInputBuffer();

    for (;;)
    {
        WCHAR Char;
        bool PopupKeys = false;

        Status = GetChar(pInputBuffer,
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

        SHORT Index;
        if (PopupKeys)
        {
            switch (Char)
            {
            case VK_F9:
            {
                const HRESULT hr = CommandNumberPopup(cookedReadData);
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
                cookedReadData.EndCurrentPopup();
                return CONSOLE_STATUS_WAIT_NO_BLOCK;
            case VK_UP:
                Popup->Update(-1);
                break;
            case VK_DOWN:
                Popup->Update(1);
                break;
            case VK_END:
                // Move waaay forward, UpdateCommandListPopup() can handle it.
                Popup->Update((SHORT)(commandHistory.GetNumberOfCommands()));
                break;
            case VK_HOME:
                // Move waaay back, UpdateCommandListPopup() can handle it.
                Popup->Update(-(SHORT)(commandHistory.GetNumberOfCommands()));
                break;
            case VK_PRIOR:
                Popup->Update(-(SHORT)Popup->Height());
                break;
            case VK_NEXT:
                Popup->Update((SHORT)Popup->Height());
                break;
            case VK_LEFT:
            case VK_RIGHT:
                Index = Popup->CurrentCommand;
                cookedReadData.EndCurrentPopup();
                SetCurrentCommandLine(cookedReadData, (SHORT)Index);
                return CONSOLE_STATUS_WAIT_NO_BLOCK;
            default:
                break;
            }
        }
        else if (Char == UNICODE_CARRIAGERETURN)
        {
            DWORD LineCount = 1;
            Index = Popup->CurrentCommand;
            cookedReadData.EndCurrentPopup();
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
                        FAIL_FAST_IF_FALSE(Tmp < (cookedReadData._BackupLimit + cookedReadData._BytesRead));
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
                PCHAR TransBuffer;

                // If ansi, translate string.
                TransBuffer = (PCHAR) new(std::nothrow) BYTE[NumBytes];
                if (TransBuffer == nullptr)
                {
                    return STATUS_NO_MEMORY;
                }

                NumBytes = ConvertToOem(gci.CP,
                                        cookedReadData._UserBuffer,
                                        gsl::narrow<UINT>(NumBytes / sizeof(WCHAR)),
                                        TransBuffer,
                                        gsl::narrow<UINT>(NumBytes));
                memmove(cookedReadData._UserBuffer, TransBuffer, NumBytes);
                delete[] TransBuffer;
            }

            *(cookedReadData.pdwNumBytes) = NumBytes;

            return CONSOLE_STATUS_READ_COMPLETE;
        }
        else
        {
            if (cookedReadData.History().FindMatchingCommand({ &Char, 1 },
                                                               Popup->CurrentCommand,
                                                               Index,
                                                               CommandHistory::MatchOptions::JustLooking))
            {
                Popup->Update((SHORT)(Index - Popup->CurrentCommand), true);
            }
        }
    }
}

// Routine Description:
// - This routine handles the delete from cursor to char char popup.  It returns when we're out of input or the user has entered a char.
// Return Value:
// - CONSOLE_STATUS_WAIT - we ran out of input, so a wait block was created
// - CONSOLE_STATUS_READ_COMPLETE - user hit return
[[nodiscard]]
NTSTATUS ProcessCopyFromCharInput(COOKED_READ_DATA& cookedReadData)
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
                cookedReadData.EndCurrentPopup();
                return CONSOLE_STATUS_WAIT_NO_BLOCK;
            }
        }

        cookedReadData.EndCurrentPopup();

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

// Routine Description:
// - This routine handles the delete char popup.  It returns when we're out of input or the user has entered a char.
// Return Value:
// - CONSOLE_STATUS_WAIT - we ran out of input, so a wait block was created
// - CONSOLE_STATUS_READ_COMPLETE - user hit return
[[nodiscard]]
NTSTATUS ProcessCopyToCharInput(COOKED_READ_DATA& cookedReadData)
{
    NTSTATUS Status = STATUS_SUCCESS;
    InputBuffer* const pInputBuffer = cookedReadData.GetInputBuffer();
    for (;;)
    {
        WCHAR Char;
        bool PopupKeys = false;
        Status = GetChar(pInputBuffer,
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

        if (PopupKeys)
        {
            switch (Char)
            {
            case VK_ESCAPE:
                cookedReadData.EndCurrentPopup();
                return CONSOLE_STATUS_WAIT_NO_BLOCK;
            }
        }

        cookedReadData.EndCurrentPopup();

        // copy up to specified char
        const auto LastCommand = cookedReadData.History().GetLastCommand();
        if (!LastCommand.empty())
        {
            size_t i;

            // find specified char in last command
            for (i = cookedReadData._CurrentPosition + 1; i < LastCommand.size(); i++)
            {
                if (LastCommand[i] == Char)
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

                    Status = WriteCharsLegacy(cookedReadData.ScreenInfo(),
                                              cookedReadData._BackupLimit,
                                              cookedReadData._BufPtr,
                                              cookedReadData._BufPtr,
                                              &j,
                                              &NumSpaces,
                                              cookedReadData.OriginalCursorPosition().X,
                                              WC_DESTRUCTIVE_BACKSPACE | WC_KEEP_CURSOR_VISIBLE | WC_ECHO,
                                              &ScrollY);
                    FAIL_FAST_IF_NTSTATUS_FAILED(Status);
                    cookedReadData.OriginalCursorPosition().Y += ScrollY;
                    cookedReadData.VisibleCharCount() += NumSpaces;
                }

                cookedReadData._BufPtr += j / sizeof(WCHAR);
            }
        }

        return CONSOLE_STATUS_WAIT_NO_BLOCK;
    }
}

// Routine Description:
// - This routine handles the command number selection popup.
// Return Value:
// - CONSOLE_STATUS_WAIT - we ran out of input, so a wait block was created
// - CONSOLE_STATUS_READ_COMPLETE - user hit return
[[nodiscard]]
NTSTATUS ProcessCommandNumberInput(COOKED_READ_DATA& cookedReadData)
{
    CommandHistory& commandHistory = cookedReadData.History();
    Popup* const Popup = commandHistory.PopupList.front();
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

        if (Char >= L'0' && Char <= L'9')
        {
            if (Popup->CommandNumberInput().size() < COMMAND_NUMBER_LENGTH)
            {
                size_t CharsToWrite = sizeof(WCHAR);
                const TextAttribute realAttributes = cookedReadData.ScreenInfo().GetAttributes();
                cookedReadData.ScreenInfo().SetAttributes(Popup->Attributes);
                size_t NumSpaces;
                const auto numberBuffer = Popup->CommandNumberInput();
                Status = WriteCharsLegacy(cookedReadData.ScreenInfo(),
                                          numberBuffer.data(),
                                          numberBuffer.data() + numberBuffer.size(),
                                          &Char,
                                          &CharsToWrite,
                                          &NumSpaces,
                                          cookedReadData.OriginalCursorPosition().X,
                                          WC_DESTRUCTIVE_BACKSPACE | WC_KEEP_CURSOR_VISIBLE | WC_ECHO,
                                          nullptr);
                FAIL_FAST_IF_NTSTATUS_FAILED(Status);
                cookedReadData.ScreenInfo().SetAttributes(realAttributes);
                try
                {
                    Popup->AddNumberToNumberBuffer(Char);
                }
                catch (...)
                {
                    return NTSTATUS_FROM_HRESULT(wil::ResultFromCaughtException());
                }
            }
        }
        else if (Char == UNICODE_BACKSPACE)
        {
            if (Popup->CommandNumberInput().size() > 0)
            {
                size_t CharsToWrite = sizeof(WCHAR);
                const TextAttribute realAttributes = cookedReadData.ScreenInfo().GetAttributes();
                cookedReadData.ScreenInfo().SetAttributes(Popup->Attributes);
                size_t NumSpaces;
                const auto numberBuffer = Popup->CommandNumberInput();
                Status = WriteCharsLegacy(cookedReadData.ScreenInfo(),
                                          numberBuffer.data(),
                                          numberBuffer.data() + numberBuffer.size(),
                                          &Char,
                                          &CharsToWrite,
                                          &NumSpaces,
                                          cookedReadData.OriginalCursorPosition().X,
                                          WC_DESTRUCTIVE_BACKSPACE | WC_KEEP_CURSOR_VISIBLE | WC_ECHO,
                                          nullptr);

                FAIL_FAST_IF_NTSTATUS_FAILED(Status);
                cookedReadData.ScreenInfo().SetAttributes(realAttributes);
                Popup->DeleteLastFromNumberBuffer();
            }
        }
        else if (Char == (WCHAR)VK_ESCAPE)
        {
            cookedReadData.EndCurrentPopup();
            if (!commandHistory.PopupList.empty())
            {
                cookedReadData.EndCurrentPopup();
            }

            // Note that cookedReadData's OriginalCursorPosition is the position before ANY text was entered on the edit line.
            // We want to use the position before the cursor was moved for this popup handler specifically, which may be *anywhere* in the edit line
            // and will be synchronized with the pointers in the cookedReadData structure (BufPtr, etc.)
            LOG_IF_FAILED(cookedReadData.ScreenInfo().SetCursorPosition(cookedReadData.BeforeDialogCursorPosition(), TRUE));
        }
        else if (Char == UNICODE_CARRIAGERETURN)
        {
            const short CommandNumber = gsl::narrow<short>(std::min(static_cast<size_t>(Popup->ParseCommandNumberInput()),
                                                                    cookedReadData.History().GetNumberOfCommands() - 1));

            cookedReadData.EndCurrentPopup();
            if (!commandHistory.PopupList.empty())
            {
                cookedReadData.EndCurrentPopup();
            }
            SetCurrentCommandLine(cookedReadData, CommandNumber);
        }
        return CONSOLE_STATUS_WAIT_NO_BLOCK;
    }
}

// Routine Description:
// - This routine handles the command list popup.  It puts up the popup, then calls ProcessCommandListInput to get and process input.
// Return Value:
// - CONSOLE_STATUS_WAIT - we ran out of input, so a wait block was created
// - STATUS_SUCCESS - read was fully completed (user hit return)
[[nodiscard]]
NTSTATUS CommandListPopup(COOKED_READ_DATA& cookedReadData)
{
    if (cookedReadData.HasHistory() &&
        cookedReadData.History().GetNumberOfCommands())
    {
        COORD PopupSize;
        PopupSize.X = 40;
        PopupSize.Y = 10;

        CommandHistory& commandHistory = cookedReadData.History();
        try
        {
            const auto Popup = commandHistory.BeginPopup(cookedReadData.ScreenInfo(), PopupSize, Popup::PopFunc::CommandList);

            SHORT const CurrentCommand = commandHistory.LastDisplayed;
            if (CurrentCommand < (SHORT)(commandHistory.GetNumberOfCommands() - Popup->Height()))
            {
                Popup->BottomIndex = std::max(CurrentCommand, gsl::narrow<SHORT>(Popup->Height() - 1i16));
            }
            else
            {
                Popup->BottomIndex = (SHORT)(commandHistory.GetNumberOfCommands() - 1);
            }
            Popup->CurrentCommand = CurrentCommand;
            Popup->Draw();

            return ProcessCommandListInput(cookedReadData);
        }
        CATCH_RETURN();
    }
    else
    {
        return S_FALSE;
    }
}

// Routine Description:
// - This routine handles the "delete up to this char" popup.  It puts up the popup, then calls ProcessCopyFromCharInput to get and process input.
// Return Value:
// - CONSOLE_STATUS_WAIT - we ran out of input, so a wait block was created
// - STATUS_SUCCESS - read was fully completed (user hit return)
[[nodiscard]]
NTSTATUS CopyFromCharPopup(COOKED_READ_DATA& cookedReadData)
{
    // Delete the current command from cursor position to the
    // letter specified by the user. The user is prompted via
    // popup to enter a character.
    if (cookedReadData.HasHistory())
    {
        COORD PopupSize;

        PopupSize.X = COPY_FROM_CHAR_PROMPT_LENGTH + 2;
        PopupSize.Y = 1;

        try
        {
            CommandHistory& commandHistory = cookedReadData.History();
            const auto Popup = commandHistory.BeginPopup(cookedReadData.ScreenInfo(), PopupSize, Popup::PopFunc::CopyFromChar);
            Popup->Draw();

            return ProcessCopyFromCharInput(cookedReadData);
        }
        CATCH_RETURN();
    }
    else
    {
        return S_FALSE;
    }
}

// Routine Description:
// - This routine handles the "copy up to this char" popup.  It puts up the popup, then calls ProcessCopyToCharInput to get and process input.
// Return Value:
// - CONSOLE_STATUS_WAIT - we ran out of input, so a wait block was created
// - STATUS_SUCCESS - read was fully completed (user hit return)
// - S_FALSE - if we couldn't make a popup because we had no commands
[[nodiscard]]
NTSTATUS CopyToCharPopup(COOKED_READ_DATA& cookedReadData)
{
    // copy the previous command to the current command, up to but
    // not including the character specified by the user.  the user
    // is prompted via popup to enter a character.
    if (cookedReadData.HasHistory())
    {
        COORD PopupSize;
        PopupSize.X = COPY_TO_CHAR_PROMPT_LENGTH + 2;
        PopupSize.Y = 1;

        try
        {
            CommandHistory& commandHistory = cookedReadData.History();
            const auto Popup = commandHistory.BeginPopup(cookedReadData.ScreenInfo(), PopupSize, Popup::PopFunc::CopyToChar);
            Popup->Draw();

            return ProcessCopyToCharInput(cookedReadData);
        }
        CATCH_RETURN();
    }
    else
    {
        return S_FALSE;
    }
}

// Routine Description:
// - This routine handles the "enter command number" popup.  It puts up the popup, then calls ProcessCommandNumberInput to get and process input.
// Return Value:
// - CONSOLE_STATUS_WAIT - we ran out of input, so a wait block was created
// - STATUS_SUCCESS - read was fully completed (user hit return)
// - S_FALSE - if we couldn't make a popup because we had no commands or it wouldn't fit.
[[nodiscard]]
HRESULT CommandNumberPopup(COOKED_READ_DATA& cookedReadData)
{
    if (cookedReadData.HasHistory() &&
        cookedReadData.History().GetNumberOfCommands() &&
        cookedReadData.ScreenInfo().GetScreenBufferSize().X >= MINIMUM_COMMAND_PROMPT_SIZE + 2)
    {
        COORD PopupSize;
        // 2 is for border
        PopupSize.X = COMMAND_NUMBER_PROMPT_LENGTH + COMMAND_NUMBER_LENGTH;
        PopupSize.Y = 1;

        try
        {
            CommandHistory& commandHistory = cookedReadData.History();
            const auto Popup = commandHistory.BeginPopup(cookedReadData.ScreenInfo(), PopupSize, Popup::PopFunc::CommandNumber);
            Popup->Draw();

            // Save the original cursor position in case the user cancels out of the dialog
            cookedReadData.BeforeDialogCursorPosition() = cookedReadData.ScreenInfo().GetTextBuffer().GetCursor().GetPosition();

            // Move the cursor into the dialog so the user can type multiple characters for the command number
            const COORD CursorPosition = Popup->GetCursorPosition();
            LOG_IF_FAILED(cookedReadData.ScreenInfo().SetCursorPosition(CursorPosition, TRUE));

            // Transfer control to the handler routine
            return ProcessCommandNumberInput(cookedReadData);
        }
        CATCH_RETURN();
    }
    else
    {
        return S_FALSE;
    }
}

// Routine Description:
// - Process virtual key code and updates the prompt line with the next history element in the direction
// specified by wch
// Arguments:
// - cookedReadData - The cooked read data to operate on
// - searchDirection - Direction in history to search
// Note:
// - May throw exceptions
void ProcessHistoryCycling(COOKED_READ_DATA& cookedReadData, const CommandHistory::SearchDirection searchDirection)
{
    if (!cookedReadData.HasHistory())
    {
        return;
    }

    // for doskey compatibility, buffer isn't circular
    if ((searchDirection == CommandHistory::SearchDirection::Previous &&
         (!cookedReadData.HasHistory() || !cookedReadData.History().AtFirstCommand())) ||
        (searchDirection == CommandHistory::SearchDirection::Next &&
         (!cookedReadData.HasHistory() || !cookedReadData.History().AtLastCommand())))
    {
        DeleteCommandLine(cookedReadData, true);
        THROW_IF_FAILED(cookedReadData.History().Retrieve(searchDirection,
                                                          cookedReadData.SpanWholeBuffer(),
                                                          cookedReadData._BytesRead));
        FAIL_FAST_IF_FALSE(cookedReadData._BackupLimit == cookedReadData._BufPtr);
        if (cookedReadData.IsEchoInput())
        {
            short ScrollY = 0;
            FAIL_FAST_IF_NTSTATUS_FAILED(WriteCharsLegacy(cookedReadData.ScreenInfo(),
                                                          cookedReadData._BackupLimit,
                                                          cookedReadData._BufPtr,
                                                          cookedReadData._BufPtr,
                                                          &cookedReadData._BytesRead,
                                                          &cookedReadData.VisibleCharCount(),
                                                          cookedReadData.OriginalCursorPosition().X,
                                                          WC_DESTRUCTIVE_BACKSPACE | WC_KEEP_CURSOR_VISIBLE | WC_ECHO,
                                                          &ScrollY));
            cookedReadData.OriginalCursorPosition().Y += ScrollY;
        }
        const size_t CharsToWrite = cookedReadData._BytesRead / sizeof(WCHAR);
        cookedReadData._CurrentPosition = CharsToWrite;
        cookedReadData._BufPtr = cookedReadData._BackupLimit + CharsToWrite;
    }
}

// Routine Description:
// - Sets the text on the prompt to the oldest run command in the cookedReadData's history
// Arguments:
// - cookedReadData - The cooked read data to operate on
// Note:
// - May throw exceptions
void SetPromptToOldestCommand(COOKED_READ_DATA& cookedReadData)
{
    if (cookedReadData.HasHistory() && cookedReadData.History().GetNumberOfCommands())
    {
        DeleteCommandLine(cookedReadData, true);
        const short commandNumber = 0;
        THROW_IF_FAILED(cookedReadData.History().RetrieveNth(commandNumber,
                                                             cookedReadData.SpanWholeBuffer(),
                                                             cookedReadData._BytesRead));
        FAIL_FAST_IF_FALSE(cookedReadData._BackupLimit == cookedReadData._BufPtr);
        if (cookedReadData.IsEchoInput())
        {
            short ScrollY = 0;
            FAIL_FAST_IF_NTSTATUS_FAILED(WriteCharsLegacy(cookedReadData.ScreenInfo(),
                                                          cookedReadData._BackupLimit,
                                                          cookedReadData._BufPtr,
                                                          cookedReadData._BufPtr,
                                                          &cookedReadData._BytesRead,
                                                          &cookedReadData.VisibleCharCount(),
                                                          cookedReadData.OriginalCursorPosition().X,
                                                          WC_DESTRUCTIVE_BACKSPACE | WC_KEEP_CURSOR_VISIBLE | WC_ECHO,
                                                          &ScrollY));
            cookedReadData.OriginalCursorPosition().Y += ScrollY;
        }
        size_t CharsToWrite = cookedReadData._BytesRead / sizeof(WCHAR);
        cookedReadData._CurrentPosition = CharsToWrite;
        cookedReadData._BufPtr = cookedReadData._BackupLimit + CharsToWrite;
    }
}

// Routine Description:
// - Sets the text on the prompt the most recently run command in cookedReadData's history
// Arguments:
// - cookedReadData - The cooked read data to operate on
// Note:
// - May throw exceptions
void SetPromptToNewestCommand(COOKED_READ_DATA& cookedReadData)
{
    DeleteCommandLine(cookedReadData, true);
    if (cookedReadData.HasHistory() && cookedReadData.History().GetNumberOfCommands())
    {
        const short commandNumber = (SHORT)(cookedReadData.History().GetNumberOfCommands() - 1);
        THROW_IF_FAILED(cookedReadData.History().RetrieveNth(commandNumber,
                                                             cookedReadData.SpanWholeBuffer(),
                                                             cookedReadData._BytesRead));
        FAIL_FAST_IF_FALSE(cookedReadData._BackupLimit == cookedReadData._BufPtr);
        if (cookedReadData.IsEchoInput())
        {
            short ScrollY = 0;
            FAIL_FAST_IF_NTSTATUS_FAILED(WriteCharsLegacy(cookedReadData.ScreenInfo(),
                                                          cookedReadData._BackupLimit,
                                                          cookedReadData._BufPtr,
                                                          cookedReadData._BufPtr,
                                                          &cookedReadData._BytesRead,
                                                          &cookedReadData.VisibleCharCount(),
                                                          cookedReadData.OriginalCursorPosition().X,
                                                          WC_DESTRUCTIVE_BACKSPACE | WC_KEEP_CURSOR_VISIBLE | WC_ECHO,
                                                          &ScrollY));
            cookedReadData.OriginalCursorPosition().Y += ScrollY;
        }
        size_t CharsToWrite = cookedReadData._BytesRead / sizeof(WCHAR);
        cookedReadData._CurrentPosition = CharsToWrite;
        cookedReadData._BufPtr = cookedReadData._BackupLimit + CharsToWrite;
    }
}

// Routine Description:
// - Deletes all prompt text to the right of the cursor
// Arguments:
// - cookedReadData - The cooked read data to operate on
void DeletePromptAfterCursor(COOKED_READ_DATA& cookedReadData) noexcept
{
    DeleteCommandLine(cookedReadData, false);
    cookedReadData._BytesRead = cookedReadData._CurrentPosition * sizeof(WCHAR);
    if (cookedReadData.IsEchoInput())
    {
        FAIL_FAST_IF_NTSTATUS_FAILED(WriteCharsLegacy(cookedReadData.ScreenInfo(),
                                                      cookedReadData._BackupLimit,
                                                      cookedReadData._BackupLimit,
                                                      cookedReadData._BackupLimit,
                                                      &cookedReadData._BytesRead,
                                                      &cookedReadData.VisibleCharCount(),
                                                      cookedReadData.OriginalCursorPosition().X,
                                                      WC_DESTRUCTIVE_BACKSPACE | WC_KEEP_CURSOR_VISIBLE | WC_ECHO,
                                                      nullptr));
    }
}

// Routine Description:
// - Deletes all user input on the prompt to the left of the cursor
// Arguments:
// - cookedReadData - The cooked read data to operate on
// Return Value:
// - The new cursor position
COORD DeletePromptBeforeCursor(COOKED_READ_DATA& cookedReadData) noexcept
{
    DeleteCommandLine(cookedReadData, false);
    cookedReadData._BytesRead -= cookedReadData._CurrentPosition * sizeof(WCHAR);
    cookedReadData._CurrentPosition = 0;
    memmove(cookedReadData._BackupLimit, cookedReadData._BufPtr, cookedReadData._BytesRead);
    cookedReadData._BufPtr = cookedReadData._BackupLimit;
    if (cookedReadData.IsEchoInput())
    {
        FAIL_FAST_IF_NTSTATUS_FAILED(WriteCharsLegacy(cookedReadData.ScreenInfo(),
                                                      cookedReadData._BackupLimit,
                                                      cookedReadData._BackupLimit,
                                                      cookedReadData._BackupLimit,
                                                      &cookedReadData._BytesRead,
                                                      &cookedReadData.VisibleCharCount(),
                                                      cookedReadData.OriginalCursorPosition().X,
                                                      WC_DESTRUCTIVE_BACKSPACE | WC_KEEP_CURSOR_VISIBLE | WC_ECHO,
                                                      nullptr));
    }
    return cookedReadData.OriginalCursorPosition();
}

// Routine Description:
// - Moves the cursor to the end of the prompt text
// Arguments:
// - cookedReadData - The cooked read data to operate on
// Return Value:
// - The new cursor position
COORD MoveCursorToEndOfPrompt(COOKED_READ_DATA& cookedReadData) noexcept
{
    cookedReadData._CurrentPosition = cookedReadData._BytesRead / sizeof(WCHAR);
    cookedReadData._BufPtr = cookedReadData._BackupLimit + cookedReadData._CurrentPosition;
    COORD cursorPosition{ 0, 0 };
    cursorPosition.X = (SHORT)(cookedReadData.OriginalCursorPosition().X + cookedReadData.VisibleCharCount());
    cursorPosition.Y = cookedReadData.OriginalCursorPosition().Y;

    const SHORT sScreenBufferSizeX = cookedReadData.ScreenInfo().GetScreenBufferSize().X;
    if (CheckBisectProcessW(cookedReadData.ScreenInfo(),
                            cookedReadData._BackupLimit,
                            cookedReadData._CurrentPosition,
                            sScreenBufferSizeX - cookedReadData.OriginalCursorPosition().X,
                            cookedReadData.OriginalCursorPosition().X,
                            true))
    {
        cursorPosition.X++;
    }
    return cursorPosition;
}

// Routine Description:
// - Moves the cursor to the start of the user input on the prompt
// Arguments:
// - cookedReadData - The cooked read data to operate on
// Return Value:
// - The new cursor position
COORD MoveCursorToStartOfPrompt(COOKED_READ_DATA& cookedReadData) noexcept
{
    cookedReadData._CurrentPosition = 0;
    cookedReadData._BufPtr = cookedReadData._BackupLimit;
    return cookedReadData.OriginalCursorPosition();
}

// Routine Description:
// - Moves the cursor left by a word
// Arguments:
// - cookedReadData - The cooked read data to operate on
// Return Value:
// - New cursor position
COORD MoveCursorLeftByWord(COOKED_READ_DATA& cookedReadData) noexcept
{
    PWCHAR LastWord;
    COORD cursorPosition = cookedReadData.ScreenInfo().GetTextBuffer().GetCursor().GetPosition();
    if (cookedReadData._BufPtr != cookedReadData._BackupLimit)
    {
        // A bit better word skipping.
        LastWord = cookedReadData._BufPtr - 1;
        if (LastWord != cookedReadData._BackupLimit)
        {
            if (*LastWord == L' ')
            {
                // Skip spaces, until the non-space character is found.
                while (--LastWord != cookedReadData._BackupLimit)
                {
                    FAIL_FAST_IF_FALSE(LastWord > cookedReadData._BackupLimit);
                    if (*LastWord != L' ')
                    {
                        break;
                    }
                }
            }
            if (LastWord != cookedReadData._BackupLimit)
            {
                if (IsWordDelim(*LastWord))
                {
                    // Skip WORD_DELIMs until space or non WORD_DELIM is found.
                    while (--LastWord != cookedReadData._BackupLimit)
                    {
                        FAIL_FAST_IF_FALSE(LastWord > cookedReadData._BackupLimit);
                        if (*LastWord == L' ' || !IsWordDelim(*LastWord))
                        {
                            break;
                        }
                    }
                }
                else
                {
                    // Skip the regular words
                    while (--LastWord != cookedReadData._BackupLimit)
                    {
                        FAIL_FAST_IF_FALSE(LastWord > cookedReadData._BackupLimit);
                        if (IsWordDelim(*LastWord))
                        {
                            break;
                        }
                    }
                }
            }
            FAIL_FAST_IF_FALSE(LastWord >= cookedReadData._BackupLimit);
            if (LastWord != cookedReadData._BackupLimit)
            {

                // LastWord is currently pointing to the last character
                // of the previous word, unless it backed up to the beginning
                // of the buffer.
                // Let's increment LastWord so that it points to the expeced
                // insertion point.
                ++LastWord;
            }
            cookedReadData._BufPtr = LastWord;
        }
        cookedReadData._CurrentPosition = (ULONG)(cookedReadData._BufPtr - cookedReadData._BackupLimit);
        cursorPosition = cookedReadData.OriginalCursorPosition();
        cursorPosition.X = (SHORT)(cursorPosition.X +
                                    RetrieveTotalNumberOfSpaces(cookedReadData.OriginalCursorPosition().X,
                                                                cookedReadData._BackupLimit,
                                                                cookedReadData._CurrentPosition));
        const SHORT sScreenBufferSizeX = cookedReadData.ScreenInfo().GetScreenBufferSize().X;
        if (CheckBisectStringW(cookedReadData._BackupLimit,
                                cookedReadData._CurrentPosition + 1,
                                sScreenBufferSizeX - cookedReadData.OriginalCursorPosition().X))
        {
            cursorPosition.X++;
        }
    }
    return cursorPosition;
}

// Routine Description:
// - Moves cursor left by a glyph
// Arguments:
// - cookedReadData - The cooked read data to operate on
// Return Value:
// - New cursor position
COORD MoveCursorLeft(COOKED_READ_DATA& cookedReadData)
{
    COORD cursorPosition = cookedReadData.ScreenInfo().GetTextBuffer().GetCursor().GetPosition();
    if (cookedReadData._BufPtr != cookedReadData._BackupLimit)
    {
        cookedReadData._BufPtr--;
        cookedReadData._CurrentPosition--;
        cursorPosition.X = cookedReadData.ScreenInfo().GetTextBuffer().GetCursor().GetPosition().X;
        cursorPosition.Y = cookedReadData.ScreenInfo().GetTextBuffer().GetCursor().GetPosition().Y;
        cursorPosition.X = (SHORT)(cursorPosition.X -
                                    RetrieveNumberOfSpaces(cookedReadData.OriginalCursorPosition().X,
                                                            cookedReadData._BackupLimit,
                                                            cookedReadData._CurrentPosition));
        const SHORT sScreenBufferSizeX = cookedReadData.ScreenInfo().GetScreenBufferSize().X;
        if (CheckBisectProcessW(cookedReadData.ScreenInfo(),
                                cookedReadData._BackupLimit,
                                cookedReadData._CurrentPosition + 2,
                                sScreenBufferSizeX - cookedReadData.OriginalCursorPosition().X,
                                cookedReadData.OriginalCursorPosition().X,
                                true))
        {
            if ((cursorPosition.X == -2) || (cursorPosition.X == -1))
            {
                cursorPosition.X--;
            }
        }
    }
    return cursorPosition;
}

// Routine Description:
// - Moves the cursor to the right by a word
// Arguments:
// - cookedReadData - The cooked read data to operate on
// Return Value:
// - The new cursor position
COORD MoveCursorRightByWord(COOKED_READ_DATA& cookedReadData) noexcept
{
    COORD cursorPosition = cookedReadData.ScreenInfo().GetTextBuffer().GetCursor().GetPosition();
    if (cookedReadData._CurrentPosition < (cookedReadData._BytesRead / sizeof(WCHAR)))
    {
        PWCHAR NextWord = cookedReadData._BufPtr;

        // A bit better word skipping.
        PWCHAR BufLast = cookedReadData._BackupLimit + cookedReadData._BytesRead / sizeof(WCHAR);

        FAIL_FAST_IF_FALSE(NextWord < BufLast);
        if (*NextWord == L' ')
        {
            // If the current character is space, skip to the next non-space character.
            while (NextWord < BufLast)
            {
                if (*NextWord != L' ')
                {
                    break;
                }
                ++NextWord;
            }
        }
        else
        {
            // Skip the body part.
            bool fStartFromDelim = IsWordDelim(*NextWord);

            while (++NextWord < BufLast)
            {
                if (fStartFromDelim != IsWordDelim(*NextWord))
                {
                    break;
                }
            }

            // Skip the space block.
            if (NextWord < BufLast && *NextWord == L' ')
            {
                while (++NextWord < BufLast)
                {
                    if (*NextWord != L' ')
                    {
                        break;
                    }
                }
            }
        }

        cookedReadData._BufPtr = NextWord;
        cookedReadData._CurrentPosition = (ULONG)(cookedReadData._BufPtr - cookedReadData._BackupLimit);
        cursorPosition = cookedReadData.OriginalCursorPosition();
        cursorPosition.X = (SHORT)(cursorPosition.X +
                                   RetrieveTotalNumberOfSpaces(cookedReadData.OriginalCursorPosition().X,
                                                               cookedReadData._BackupLimit,
                                                               cookedReadData._CurrentPosition));
        const SHORT sScreenBufferSizeX = cookedReadData.ScreenInfo().GetScreenBufferSize().X;
        if (CheckBisectStringW(cookedReadData._BackupLimit,
                                cookedReadData._CurrentPosition + 1,
                                sScreenBufferSizeX - cookedReadData.OriginalCursorPosition().X))
        {
            cursorPosition.X++;
        }
    }
    return cursorPosition;
}

// Routine Description:
// - Moves the cursor to the right by a glyph
// Arguments:
// - cookedReadData - The cooked read data to operate on
// Return Value:
// - The new cursor position
COORD MoveCursorRight(COOKED_READ_DATA& cookedReadData) noexcept
{
    COORD cursorPosition = cookedReadData.ScreenInfo().GetTextBuffer().GetCursor().GetPosition();
    const SHORT sScreenBufferSizeX = cookedReadData.ScreenInfo().GetScreenBufferSize().X;
    // If not at the end of the line, move cursor position right.
    if (cookedReadData._CurrentPosition < (cookedReadData._BytesRead / sizeof(WCHAR)))
    {
        cursorPosition = cookedReadData.ScreenInfo().GetTextBuffer().GetCursor().GetPosition();
        cursorPosition.X = (SHORT)(cursorPosition.X +
                                    RetrieveNumberOfSpaces(cookedReadData.OriginalCursorPosition().X,
                                                            cookedReadData._BackupLimit,
                                                            cookedReadData._CurrentPosition));
        if (CheckBisectProcessW(cookedReadData.ScreenInfo(),
                                cookedReadData._BackupLimit,
                                cookedReadData._CurrentPosition + 2,
                                sScreenBufferSizeX - cookedReadData.OriginalCursorPosition().X,
                                cookedReadData.OriginalCursorPosition().X,
                                true))
        {
            if (cursorPosition.X == (sScreenBufferSizeX - 1))
                cursorPosition.X++;
        }

        cookedReadData._BufPtr++;
        cookedReadData._CurrentPosition++;
    }
    // if at the end of the line, copy a character from the same position in the last command
    else if (cookedReadData.HasHistory())
    {
        size_t NumSpaces;
        const auto LastCommand = cookedReadData.History().GetLastCommand();
        if (!LastCommand.empty() && LastCommand.size() > cookedReadData._CurrentPosition)
        {
            *cookedReadData._BufPtr = LastCommand[cookedReadData._CurrentPosition];
            cookedReadData._BytesRead += sizeof(WCHAR);
            cookedReadData._CurrentPosition++;
            if (cookedReadData.IsEchoInput())
            {
                short ScrollY = 0;
                size_t CharsToWrite = sizeof(WCHAR);
                FAIL_FAST_IF_NTSTATUS_FAILED(WriteCharsLegacy(cookedReadData.ScreenInfo(),
                                                              cookedReadData._BackupLimit,
                                                              cookedReadData._BufPtr,
                                                              cookedReadData._BufPtr,
                                                              &CharsToWrite,
                                                              &NumSpaces,
                                                              cookedReadData.OriginalCursorPosition().X,
                                                              WC_DESTRUCTIVE_BACKSPACE | WC_KEEP_CURSOR_VISIBLE | WC_ECHO,
                                                              &ScrollY));
                cookedReadData.OriginalCursorPosition().Y += ScrollY;
                cookedReadData.VisibleCharCount() += NumSpaces;
            }
            cookedReadData._BufPtr += 1;
        }
    }
    return cursorPosition;
}

// Routine Description:
// - Place a ctrl-z in the current command line
// Arguments:
// - cookedReadData - The cooked read data to operate on
void InsertCtrlZ(COOKED_READ_DATA& cookedReadData) noexcept
{
    size_t NumSpaces = 0;

    *cookedReadData._BufPtr = (WCHAR)0x1a;  // ctrl-z
    cookedReadData._BytesRead += sizeof(WCHAR);
    cookedReadData._CurrentPosition++;
    if (cookedReadData.IsEchoInput())
    {
        short ScrollY = 0;
        size_t CharsToWrite = sizeof(WCHAR);
        FAIL_FAST_IF_NTSTATUS_FAILED(WriteCharsLegacy(cookedReadData.ScreenInfo(),
                                                     cookedReadData._BackupLimit,
                                                     cookedReadData._BufPtr,
                                                     cookedReadData._BufPtr,
                                                     &CharsToWrite,
                                                     &NumSpaces,
                                                     cookedReadData.OriginalCursorPosition().X,
                                                     WC_DESTRUCTIVE_BACKSPACE | WC_KEEP_CURSOR_VISIBLE | WC_ECHO,
                                                     &ScrollY));
        cookedReadData.OriginalCursorPosition().Y += ScrollY;
        cookedReadData.VisibleCharCount() += NumSpaces;
    }
    cookedReadData._BufPtr += 1;
}

// Routine Description:
// - Empties the command history for cookedReadData
// Arguments:
// - cookedReadData - The cooked read data to operate on
void DeleteCommandHistory(COOKED_READ_DATA& cookedReadData) noexcept
{
    if (cookedReadData.HasHistory())
    {
        cookedReadData.History().Empty();
        cookedReadData.History().Flags |= CLE_ALLOCATED;
    }
}

// Routine Description:
// - Copy the remainder of the previous command to the current command.
// Arguments:
// - cookedReadData - The cooked read data to operate on
void FillPromptWithPreviousCommandFragment(COOKED_READ_DATA& cookedReadData) noexcept
{
    if (cookedReadData.HasHistory())
    {
        size_t NumSpaces, cchCount;

        const auto LastCommand = cookedReadData.History().GetLastCommand();
        if (!LastCommand.empty() && LastCommand.size() > cookedReadData._CurrentPosition)
        {
            cchCount = LastCommand.size() - cookedReadData._CurrentPosition;
            const auto bufferSpan = cookedReadData.SpanAtPointer();
            std::copy_n(LastCommand.cbegin() + cookedReadData._CurrentPosition, cchCount, bufferSpan.begin());
            cookedReadData._CurrentPosition += cchCount;
            cchCount *= sizeof(WCHAR);
            cookedReadData._BytesRead = std::max(LastCommand.size() * sizeof(wchar_t), cookedReadData._BytesRead);
            if (cookedReadData.IsEchoInput())
            {
                short ScrollY = 0;
                FAIL_FAST_IF_NTSTATUS_FAILED(WriteCharsLegacy(cookedReadData.ScreenInfo(),
                                                              cookedReadData._BackupLimit,
                                                              cookedReadData._BufPtr,
                                                              cookedReadData._BufPtr,
                                                              &cchCount,
                                                              &NumSpaces,
                                                              cookedReadData.OriginalCursorPosition().X,
                                                              WC_DESTRUCTIVE_BACKSPACE | WC_KEEP_CURSOR_VISIBLE | WC_ECHO,
                                                              &ScrollY));
                cookedReadData.OriginalCursorPosition().Y += ScrollY;
                cookedReadData.VisibleCharCount() += NumSpaces;
            }
            cookedReadData._BufPtr += cchCount / sizeof(WCHAR);
        }
    }
}

// Routine Description:
// - Cycles through the stored commands that start with the characters in the current command.
// Arguments:
// - cookedReadData - The cooked read data to operate on
// Return Value:
// - The new cursor position
COORD CycleMatchingCommandHistoryToPrompt(COOKED_READ_DATA& cookedReadData)
{
    COORD cursorPosition = cookedReadData.ScreenInfo().GetTextBuffer().GetCursor().GetPosition();
    if (cookedReadData.HasHistory())
    {
        SHORT index;
        if (cookedReadData.History().FindMatchingCommand({ cookedReadData._BackupLimit, cookedReadData._CurrentPosition },
                                                            cookedReadData.History().LastDisplayed,
                                                            index,
                                                            CommandHistory::MatchOptions::None))
        {
            SHORT CurrentPos;

            // save cursor position
            CurrentPos = (SHORT)cookedReadData._CurrentPosition;

            DeleteCommandLine(cookedReadData, true);
            THROW_IF_FAILED(cookedReadData.History().RetrieveNth((SHORT)index,
                                                                 cookedReadData.SpanWholeBuffer(),
                                                                 cookedReadData._BytesRead));
            FAIL_FAST_IF_FALSE(cookedReadData._BackupLimit == cookedReadData._BufPtr);
            if (cookedReadData.IsEchoInput())
            {
                short ScrollY = 0;
                FAIL_FAST_IF_NTSTATUS_FAILED(WriteCharsLegacy(cookedReadData.ScreenInfo(),
                                                              cookedReadData._BackupLimit,
                                                              cookedReadData._BufPtr,
                                                              cookedReadData._BufPtr,
                                                              &cookedReadData._BytesRead,
                                                              &cookedReadData.VisibleCharCount(),
                                                              cookedReadData.OriginalCursorPosition().X,
                                                              WC_DESTRUCTIVE_BACKSPACE | WC_KEEP_CURSOR_VISIBLE | WC_ECHO,
                                                              &ScrollY));
                cookedReadData.OriginalCursorPosition().Y += ScrollY;
                cursorPosition.Y += ScrollY;
            }

            // restore cursor position
            cookedReadData._BufPtr = cookedReadData._BackupLimit + CurrentPos;
            cookedReadData._CurrentPosition = CurrentPos;
            FAIL_FAST_IF_NTSTATUS_FAILED(cookedReadData.ScreenInfo().SetCursorPosition(cursorPosition, true));
        }
    }
    return cursorPosition;
}

// Routine Description:
// - Deletes a glyph from the right side of the cursor
// Arguments:
// - cookedReadData - The cooked read data to operate on
// Return Value:
// - The new cursor position
COORD DeleteFromRightOfCursor(COOKED_READ_DATA& cookedReadData) noexcept
{
    // save cursor position
    COORD cursorPosition = cookedReadData.ScreenInfo().GetTextBuffer().GetCursor().GetPosition();

    if (!cookedReadData.AtEol())
    {
        // Delete commandline.
#pragma prefast(suppress:__WARNING_BUFFER_OVERFLOW, "Not sure why prefast is getting confused here")
        DeleteCommandLine(cookedReadData, false);

        // Delete char.
        cookedReadData._BytesRead -= sizeof(WCHAR);
        memmove(cookedReadData._BufPtr,
                cookedReadData._BufPtr + 1,
                cookedReadData._BytesRead - (cookedReadData._CurrentPosition * sizeof(WCHAR)));

        {
            PWCHAR buf = (PWCHAR)((PBYTE)cookedReadData._BackupLimit + cookedReadData._BytesRead);
            *buf = (WCHAR)' ';
        }

        // Write commandline.
        if (cookedReadData.IsEchoInput())
        {
            FAIL_FAST_IF_NTSTATUS_FAILED(WriteCharsLegacy(cookedReadData.ScreenInfo(),
                                                          cookedReadData._BackupLimit,
                                                          cookedReadData._BackupLimit,
                                                          cookedReadData._BackupLimit,
                                                          &cookedReadData._BytesRead,
                                                          &cookedReadData.VisibleCharCount(),
                                                          cookedReadData.OriginalCursorPosition().X,
                                                          WC_DESTRUCTIVE_BACKSPACE | WC_KEEP_CURSOR_VISIBLE | WC_ECHO,
                                                          nullptr));
        }

        // restore cursor position
        const SHORT sScreenBufferSizeX = cookedReadData.ScreenInfo().GetScreenBufferSize().X;
        if (CheckBisectProcessW(cookedReadData.ScreenInfo(),
                                cookedReadData._BackupLimit,
                                cookedReadData._CurrentPosition + 1,
                                sScreenBufferSizeX - cookedReadData.OriginalCursorPosition().X,
                                cookedReadData.OriginalCursorPosition().X,
                                true))
        {
            cursorPosition.X++;
        }
    }
    return cursorPosition;
}



// TODO: [MSFT:4586207] Clean up this mess -- needs helpers. http://osgvsowi/4586207
// Routine Description:
// - This routine process command line editing keys.
// Return Value:
// - CONSOLE_STATUS_WAIT - CommandListPopup ran out of input
// - CONSOLE_STATUS_READ_COMPLETE - user hit <enter> in CommandListPopup
// - STATUS_SUCCESS - everything's cool
[[nodiscard]]
NTSTATUS ProcessCommandLine(COOKED_READ_DATA& cookedReadData,
                            _In_ WCHAR wch,
                            const DWORD dwKeyState)
{
    const CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    COORD cursorPosition = cookedReadData.ScreenInfo().GetTextBuffer().GetCursor().GetPosition();
    NTSTATUS Status;

    const bool altPressed = IsAnyFlagSet(dwKeyState, LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED);
    const bool ctrlPressed = IsAnyFlagSet(dwKeyState, LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED);
    bool UpdateCursorPosition = false;
    switch (wch)
    {
    case VK_ESCAPE:
        DeleteCommandLine(cookedReadData, true);
        break;
    case VK_DOWN:
        try
        {
            ProcessHistoryCycling(cookedReadData, CommandHistory::SearchDirection::Next);
            Status = STATUS_SUCCESS;
        }
        catch (...)
        {
            Status = wil::ResultFromCaughtException();
        }
        break;
    case VK_UP:
    case VK_F5:
        try
        {
            ProcessHistoryCycling(cookedReadData, CommandHistory::SearchDirection::Previous);
            Status = STATUS_SUCCESS;
        }
        catch (...)
        {
            Status = wil::ResultFromCaughtException();
        }
        break;
    case VK_PRIOR:
        try
        {
            SetPromptToOldestCommand(cookedReadData);
            Status = STATUS_SUCCESS;
        }
        catch (...)
        {
            Status = wil::ResultFromCaughtException();
        }
        break;
    case VK_NEXT:
        try
        {
            SetPromptToNewestCommand(cookedReadData);
            Status = STATUS_SUCCESS;
        }
        catch (...)
        {
            Status = wil::ResultFromCaughtException();
        }
        break;
    case VK_END:
        if (ctrlPressed)
        {
            DeletePromptAfterCursor(cookedReadData);
        }
        else
        {
            cursorPosition = MoveCursorToEndOfPrompt(cookedReadData);
            UpdateCursorPosition = true;
        }
        break;
    case VK_HOME:
        if (ctrlPressed)
        {
            cursorPosition = DeletePromptBeforeCursor(cookedReadData);
            UpdateCursorPosition = true;
        }
        else
        {
            cursorPosition = MoveCursorToStartOfPrompt(cookedReadData);
            UpdateCursorPosition = true;
        }
        break;
    case VK_LEFT:
        if (ctrlPressed)
        {
            cursorPosition = MoveCursorLeftByWord(cookedReadData);
            UpdateCursorPosition = true;
        }
        else
        {
            cursorPosition = MoveCursorLeft(cookedReadData);
            UpdateCursorPosition = true;
        }
        break;
    case VK_F1:
    {
        // we don't need to check for end of buffer here because we've
        // already done it.
        cursorPosition = MoveCursorRight(cookedReadData);
        UpdateCursorPosition = true;
        break;
    }
    case VK_RIGHT:
        // we don't need to check for end of buffer here because we've
        // already done it.
        if (ctrlPressed)
        {
            cursorPosition = MoveCursorRightByWord(cookedReadData);
            UpdateCursorPosition = true;
        }
        else
        {
            cursorPosition = MoveCursorRight(cookedReadData);
            UpdateCursorPosition = true;
        }
        break;
    case VK_F2:
    {
        Status = CopyToCharPopup(cookedReadData);
        if (S_FALSE == Status)
        {
            // We couldn't make the popup, so loop around and read the next character.
            break;
        }
        else
        {
            return Status;
        }
    }
    case VK_F3:
        FillPromptWithPreviousCommandFragment(cookedReadData);
        break;
    case VK_F4:
    {
        Status = CopyFromCharPopup(cookedReadData);
        if (S_FALSE == Status)
        {
            // We couldn't display a popup. Go around a loop behind.
            break;
        }
        else
        {
            return Status;
        }
    }
    case VK_F6:
    {
        InsertCtrlZ(cookedReadData);
        break;
    }
    case VK_F7:
        if (!ctrlPressed && !altPressed)
        {
            Status = CommandListPopup(cookedReadData);
        }
        else if (altPressed)
        {
            DeleteCommandHistory(cookedReadData);
        }
        break;

    case VK_F8:
        try
        {
            cursorPosition = CycleMatchingCommandHistoryToPrompt(cookedReadData);
            UpdateCursorPosition = true;
        }
        catch (...)
        {
            Status = wil::ResultFromCaughtException();
        }
        break;
    case VK_F9:
    {
        Status = CommandNumberPopup(cookedReadData);
        if (S_FALSE == Status)
        {
            // If we couldn't make the popup, break and go around to read another input character.
            break;
        }
        else
        {
            return Status;
        }
    }
    case VK_F10:
        // Alt+F10 clears the aliases for specifically cmd.exe.
        if (altPressed)
        {
            Alias::s_ClearCmdExeAliases();
        }
        break;
    case VK_INSERT:
        cookedReadData.SetInsertMode(!cookedReadData.IsInsertMode());
        cookedReadData.ScreenInfo().SetCursorDBMode(cookedReadData.IsInsertMode() != gci.GetInsertMode());
        break;
    case VK_DELETE:
        cursorPosition = DeleteFromRightOfCursor(cookedReadData);
        UpdateCursorPosition = true;
        break;
    default:
        FAIL_FAST_HR(E_NOTIMPL);
        break;
    }

    if (UpdateCursorPosition && cookedReadData.IsEchoInput())
    {
        Status = AdjustCursorPosition(cookedReadData.ScreenInfo(), cursorPosition, true, nullptr);
        FAIL_FAST_IF_NTSTATUS_FAILED(Status);
    }

    return STATUS_SUCCESS;
}
