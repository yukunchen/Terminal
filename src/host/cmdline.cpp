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
HRESULT CommandNumberPopup(_In_ COOKED_READ_DATA* const CookedReadData);

// Routine Description:
// - This routine validates a string buffer and returns the pointers of where the strings start within the buffer.
// Arguments:
// - Unicode - Supplies a boolean that is TRUE if the buffer contains Unicode strings, FALSE otherwise.
// - Buffer - Supplies the buffer to be validated.
// - size - Supplies the size, in bytes, of the buffer to be validated.
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

bool IsWordDelim(const std::vector<wchar_t>& charData)
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
    const COOKED_READ_DATA* const pTyped = gci.lpCookedReadData;

    if (nullptr == pTyped)
    {
        // If the cooked read data pointer is null, there is no edit line data and therefore it's empty.
        return true;
    }
    else if (0 == pTyped->_NumberOfVisibleChars)
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
    const CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    if (!IsEditLineEmpty())
    {
        COOKED_READ_DATA* CookedReadData = gci.lpCookedReadData;
        DeleteCommandLine(CookedReadData, fUpdateFields);
    }
}

void CommandLine::Show()
{
    const CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    if (!IsEditLineEmpty())
    {
        COOKED_READ_DATA* CookedReadData = gci.lpCookedReadData;
        RedrawCommandLine(CookedReadData);
    }
}

void DeleteCommandLine(_Inout_ COOKED_READ_DATA* const pCookedReadData, const bool fUpdateFields)
{
    size_t CharsToWrite = pCookedReadData->_NumberOfVisibleChars;
    COORD coordOriginalCursor = pCookedReadData->_OriginalCursorPosition;
    const COORD coordBufferSize = pCookedReadData->_screenInfo.GetScreenBufferSize();

    // catch the case where the current command has scrolled off the top of the screen.
    if (coordOriginalCursor.Y < 0)
    {
        CharsToWrite += coordBufferSize.X * coordOriginalCursor.Y;
        CharsToWrite += pCookedReadData->_OriginalCursorPosition.X;   // account for prompt
        pCookedReadData->_OriginalCursorPosition.X = 0;
        pCookedReadData->_OriginalCursorPosition.Y = 0;
        coordOriginalCursor.X = 0;
        coordOriginalCursor.Y = 0;
    }

    if (!CheckBisectStringW(pCookedReadData->_BackupLimit,
                            CharsToWrite,
                            coordBufferSize.X - pCookedReadData->_OriginalCursorPosition.X))
    {
        CharsToWrite++;
    }

    try
    {
        FillOutputW(pCookedReadData->_screenInfo,
                    { UNICODE_SPACE },
                    coordOriginalCursor,
                    CharsToWrite);
    }
    CATCH_LOG();

    if (fUpdateFields)
    {
        pCookedReadData->_BufPtr = pCookedReadData->_BackupLimit;
        pCookedReadData->_BytesRead = 0;
        pCookedReadData->_CurrentPosition = 0;
        pCookedReadData->_NumberOfVisibleChars = 0;
    }

    LOG_IF_FAILED(pCookedReadData->_screenInfo.SetCursorPosition(pCookedReadData->_OriginalCursorPosition, true));
}

void RedrawCommandLine(_Inout_ COOKED_READ_DATA* const pCookedReadData)
{
    if (pCookedReadData->_Echo)
    {
        // Draw the command line
        pCookedReadData->_OriginalCursorPosition = pCookedReadData->_screenInfo.GetTextBuffer().GetCursor().GetPosition();

        SHORT ScrollY = 0;
#pragma prefast(suppress:28931, "Status is not unused. It's used in debug assertions.")
        NTSTATUS Status = WriteCharsLegacy(pCookedReadData->_screenInfo,
                                           pCookedReadData->_BackupLimit,
                                           pCookedReadData->_BackupLimit,
                                           pCookedReadData->_BackupLimit,
                                           &pCookedReadData->_BytesRead,
                                           &pCookedReadData->_NumberOfVisibleChars,
                                           pCookedReadData->_OriginalCursorPosition.X,
                                           WC_DESTRUCTIVE_BACKSPACE | WC_KEEP_CURSOR_VISIBLE | WC_ECHO,
                                           &ScrollY);
        FAIL_FAST_IF_NTSTATUS_FAILED(Status);

        pCookedReadData->_OriginalCursorPosition.Y += ScrollY;

        // Move the cursor back to the right position
        COORD CursorPosition = pCookedReadData->_OriginalCursorPosition;
        CursorPosition.X += (SHORT)RetrieveTotalNumberOfSpaces(pCookedReadData->_OriginalCursorPosition.X,
                                                               pCookedReadData->_BackupLimit,
                                                               pCookedReadData->_CurrentPosition);
        if (CheckBisectStringW(pCookedReadData->_BackupLimit,
                               pCookedReadData->_CurrentPosition,
                               pCookedReadData->_screenInfo.GetScreenBufferSize().X - pCookedReadData->_OriginalCursorPosition.X))
        {
            CursorPosition.X++;
        }
        Status = AdjustCursorPosition(pCookedReadData->_screenInfo, CursorPosition, TRUE, nullptr);
        FAIL_FAST_IF_NTSTATUS_FAILED(Status);
    }
}

// Routine Description:
// - This routine copies the commandline specified by Index into the cooked read buffer
void SetCurrentCommandLine(_In_ COOKED_READ_DATA* const CookedReadData, _In_ SHORT Index) // index, not command number
{
    DeleteCommandLine(CookedReadData, TRUE);
    FAIL_FAST_IF_FAILED(CookedReadData->_CommandHistory->RetrieveNth(Index,
                                                                     CookedReadData->SpanWholeBuffer(),
                                                                     CookedReadData->_BytesRead));
    FAIL_FAST_IF_FALSE(CookedReadData->_BackupLimit == CookedReadData->_BufPtr);
    if (CookedReadData->_Echo)
    {
        SHORT ScrollY = 0;
        FAIL_FAST_IF_NTSTATUS_FAILED(WriteCharsLegacy(CookedReadData->_screenInfo,
                                                      CookedReadData->_BackupLimit,
                                                      CookedReadData->_BufPtr,
                                                      CookedReadData->_BufPtr,
                                                      &CookedReadData->_BytesRead,
                                                      &CookedReadData->_NumberOfVisibleChars,
                                                      CookedReadData->_OriginalCursorPosition.X,
                                                      WC_DESTRUCTIVE_BACKSPACE | WC_KEEP_CURSOR_VISIBLE | WC_ECHO,
                                                      &ScrollY));
        CookedReadData->_OriginalCursorPosition.Y += ScrollY;
    }

    size_t const CharsToWrite = CookedReadData->_BytesRead / sizeof(WCHAR);
    CookedReadData->_CurrentPosition = CharsToWrite;
    CookedReadData->_BufPtr = CookedReadData->_BackupLimit + CharsToWrite;
}

// Routine Description:
// - This routine handles the command list popup.  It returns when we're out of input or the user has selected a command line.
// Return Value:
// - CONSOLE_STATUS_WAIT - we ran out of input, so a wait block was created
// - CONSOLE_STATUS_READ_COMPLETE - user hit return
[[nodiscard]]
NTSTATUS ProcessCommandListInput(COOKED_READ_DATA* const pCookedReadData)
{
    const CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    CommandHistory* const pCommandHistory = pCookedReadData->_CommandHistory;
    Popup* const Popup = pCommandHistory->PopupList.front();
    NTSTATUS Status = STATUS_SUCCESS;
    INPUT_READ_HANDLE_DATA* const pInputReadHandleData = pCookedReadData->GetInputReadHandleData();
    InputBuffer* const pInputBuffer = pCookedReadData->GetInputBuffer();

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
                pCookedReadData->_BytesRead = 0;
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
                const HRESULT hr = CommandNumberPopup(pCookedReadData);
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
                pCookedReadData->EndCurrentPopup();
                return CONSOLE_STATUS_WAIT_NO_BLOCK;
            case VK_UP:
                Popup->Update(-1);
                break;
            case VK_DOWN:
                Popup->Update(1);
                break;
            case VK_END:
                // Move waaay forward, UpdateCommandListPopup() can handle it.
                Popup->Update((SHORT)(pCommandHistory->GetNumberOfCommands()));
                break;
            case VK_HOME:
                // Move waaay back, UpdateCommandListPopup() can handle it.
                Popup->Update(-(SHORT)(pCommandHistory->GetNumberOfCommands()));
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
                pCookedReadData->EndCurrentPopup();
                SetCurrentCommandLine(pCookedReadData, (SHORT)Index);
                return CONSOLE_STATUS_WAIT_NO_BLOCK;
            default:
                break;
            }
        }
        else if (Char == UNICODE_CARRIAGERETURN)
        {
            DWORD LineCount = 1;
            Index = Popup->CurrentCommand;
            pCookedReadData->EndCurrentPopup();
            SetCurrentCommandLine(pCookedReadData, (SHORT)Index);
            pCookedReadData->ProcessInput(UNICODE_CARRIAGERETURN, 0, Status);
            // complete read
            if (pCookedReadData->_Echo)
            {
                // check for alias
                pCookedReadData->ProcessAliases(LineCount);
            }

            Status = STATUS_SUCCESS;
            size_t NumBytes;
            if (pCookedReadData->_BytesRead > pCookedReadData->_UserBufferSize || LineCount > 1)
            {
                if (LineCount > 1)
                {
                    PWSTR Tmp;
                    for (Tmp = pCookedReadData->_BackupLimit; *Tmp != UNICODE_LINEFEED; Tmp++)
                    {
                        FAIL_FAST_IF_FALSE(Tmp < (pCookedReadData->_BackupLimit + pCookedReadData->_BytesRead));
                    }
                    NumBytes = (Tmp - pCookedReadData->_BackupLimit + 1) * sizeof(*Tmp);
                }
                else
                {
                    NumBytes = pCookedReadData->_UserBufferSize;
                }

                // Copy what we can fit into the user buffer
                memmove(pCookedReadData->_UserBuffer, pCookedReadData->_BackupLimit, NumBytes);

                // Store all of the remaining as pending until the next read operation.
                const std::string_view pending{ ((char*)pCookedReadData->_BackupLimit + NumBytes), pCookedReadData->_BytesRead - NumBytes };
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
                NumBytes = pCookedReadData->_BytesRead;
                memmove(pCookedReadData->_UserBuffer, pCookedReadData->_BackupLimit, NumBytes);
            }

            if (!pCookedReadData->_fIsUnicode)
            {
                PCHAR TransBuffer;

                // If ansi, translate string.
                TransBuffer = (PCHAR) new(std::nothrow) BYTE[NumBytes];
                if (TransBuffer == nullptr)
                {
                    return STATUS_NO_MEMORY;
                }

                NumBytes = ConvertToOem(gci.CP,
                                        pCookedReadData->_UserBuffer,
                                        gsl::narrow<UINT>(NumBytes / sizeof(WCHAR)),
                                        TransBuffer,
                                        gsl::narrow<UINT>(NumBytes));
                memmove(pCookedReadData->_UserBuffer, TransBuffer, NumBytes);
                delete[] TransBuffer;
            }

            *(pCookedReadData->pdwNumBytes) = NumBytes;

            return CONSOLE_STATUS_READ_COMPLETE;
        }
        else
        {
            if (pCookedReadData->_CommandHistory->FindMatchingCommand({ &Char, 1 }, Popup->CurrentCommand, Index, CommandHistory::MatchOptions::JustLooking))
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
NTSTATUS ProcessCopyFromCharInput(_In_ COOKED_READ_DATA* const pCookedReadData)
{
    NTSTATUS Status = STATUS_SUCCESS;
    InputBuffer* const pInputBuffer = pCookedReadData->GetInputBuffer();
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
                pCookedReadData->_BytesRead = 0;
            }

            return Status;
        }

        if (PopupKeys)
        {
            switch (Char)
            {
            case VK_ESCAPE:
                pCookedReadData->EndCurrentPopup();
                return CONSOLE_STATUS_WAIT_NO_BLOCK;
            }
        }

        pCookedReadData->EndCurrentPopup();

        size_t i;  // char index (not byte)
        // delete from cursor up to specified char
        for (i = pCookedReadData->_CurrentPosition + 1; i < (int)(pCookedReadData->_BytesRead / sizeof(WCHAR)); i++)
        {
            if (pCookedReadData->_BackupLimit[i] == Char)
            {
                break;
            }
        }

        if (i != (int)(pCookedReadData->_BytesRead / sizeof(WCHAR) + 1))
        {
            COORD CursorPosition;

            // save cursor position
            CursorPosition = pCookedReadData->_screenInfo.GetTextBuffer().GetCursor().GetPosition();

            // Delete commandline.
            DeleteCommandLine(pCookedReadData, FALSE);

            // Delete chars.
            memmove(&pCookedReadData->_BackupLimit[pCookedReadData->_CurrentPosition],
                    &pCookedReadData->_BackupLimit[i],
                    pCookedReadData->_BytesRead - (i * sizeof(WCHAR)));
            pCookedReadData->_BytesRead -= (i - pCookedReadData->_CurrentPosition) * sizeof(WCHAR);

            // Write commandline.
            if (pCookedReadData->_Echo)
            {
                Status = WriteCharsLegacy(pCookedReadData->_screenInfo,
                                          pCookedReadData->_BackupLimit,
                                          pCookedReadData->_BackupLimit,
                                          pCookedReadData->_BackupLimit,
                                          &pCookedReadData->_BytesRead,
                                          &pCookedReadData->_NumberOfVisibleChars,
                                          pCookedReadData->_OriginalCursorPosition.X,
                                          WC_DESTRUCTIVE_BACKSPACE | WC_KEEP_CURSOR_VISIBLE | WC_ECHO,
                                          nullptr);
                FAIL_FAST_IF_NTSTATUS_FAILED(Status);
            }

            // restore cursor position
            Status = pCookedReadData->_screenInfo.SetCursorPosition(CursorPosition, TRUE);
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
NTSTATUS ProcessCopyToCharInput(_In_ COOKED_READ_DATA* const pCookedReadData)
{
    NTSTATUS Status = STATUS_SUCCESS;
    InputBuffer* const pInputBuffer = pCookedReadData->GetInputBuffer();
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
                pCookedReadData->_BytesRead = 0;
            }
            return Status;
        }

        if (PopupKeys)
        {
            switch (Char)
            {
            case VK_ESCAPE:
                pCookedReadData->EndCurrentPopup();
                return CONSOLE_STATUS_WAIT_NO_BLOCK;
            }
        }

        pCookedReadData->EndCurrentPopup();

        // copy up to specified char
        const auto LastCommand = pCookedReadData->_CommandHistory->GetLastCommand();
        if (!LastCommand.empty())
        {
            size_t i;

            // find specified char in last command
            for (i = pCookedReadData->_CurrentPosition + 1; i < LastCommand.size(); i++)
            {
                if (LastCommand[i] == Char)
                {
                    break;
                }
            }

            // If we found it, copy up to it.
            if (i < LastCommand.size() &&
                (LastCommand.size() > pCookedReadData->_CurrentPosition))
            {
                size_t j = i - pCookedReadData->_CurrentPosition;
                FAIL_FAST_IF_FALSE(j > 0);
                const auto bufferSpan = pCookedReadData->SpanAtPointer();
                std::copy_n(LastCommand.cbegin() + pCookedReadData->_CurrentPosition,
                            j,
                            bufferSpan.begin());
                pCookedReadData->_CurrentPosition += j;
                j *= sizeof(WCHAR);
                pCookedReadData->_BytesRead = std::max(pCookedReadData->_BytesRead,
                                                       pCookedReadData->_CurrentPosition * sizeof(WCHAR));
                if (pCookedReadData->_Echo)
                {
                    size_t NumSpaces;
                    SHORT ScrollY = 0;

                    Status = WriteCharsLegacy(pCookedReadData->_screenInfo,
                                              pCookedReadData->_BackupLimit,
                                              pCookedReadData->_BufPtr,
                                              pCookedReadData->_BufPtr,
                                              &j,
                                              &NumSpaces,
                                              pCookedReadData->_OriginalCursorPosition.X,
                                              WC_DESTRUCTIVE_BACKSPACE | WC_KEEP_CURSOR_VISIBLE | WC_ECHO,
                                              &ScrollY);
                    FAIL_FAST_IF_NTSTATUS_FAILED(Status);
                    pCookedReadData->_OriginalCursorPosition.Y += ScrollY;
                    pCookedReadData->_NumberOfVisibleChars += NumSpaces;
                }

                pCookedReadData->_BufPtr += j / sizeof(WCHAR);
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
NTSTATUS ProcessCommandNumberInput(_In_ COOKED_READ_DATA* const pCookedReadData)
{
    CommandHistory* const CommandHistory = pCookedReadData->_CommandHistory;
    Popup* const Popup = CommandHistory->PopupList.front();
    NTSTATUS Status = STATUS_SUCCESS;
    InputBuffer* const pInputBuffer = pCookedReadData->GetInputBuffer();
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
                pCookedReadData->_BytesRead = 0;
            }
            return Status;
        }

        if (Char >= L'0' && Char <= L'9')
        {
            if (Popup->NumberRead < 5)
            {
                size_t CharsToWrite = sizeof(WCHAR);
                const TextAttribute realAttributes = pCookedReadData->_screenInfo.GetAttributes();
                pCookedReadData->_screenInfo.SetAttributes(Popup->Attributes);
                size_t NumSpaces;
                Status = WriteCharsLegacy(pCookedReadData->_screenInfo,
                                          Popup->NumberBuffer,
                                          &Popup->NumberBuffer[Popup->NumberRead],
                                          &Char,
                                          &CharsToWrite,
                                          &NumSpaces,
                                          pCookedReadData->_OriginalCursorPosition.X,
                                          WC_DESTRUCTIVE_BACKSPACE | WC_KEEP_CURSOR_VISIBLE | WC_ECHO,
                                          nullptr);
                FAIL_FAST_IF_NTSTATUS_FAILED(Status);
                pCookedReadData->_screenInfo.SetAttributes(realAttributes);
                Popup->NumberBuffer[Popup->NumberRead] = Char;
                Popup->NumberRead += 1;
            }
        }
        else if (Char == UNICODE_BACKSPACE)
        {
            if (Popup->NumberRead > 0)
            {
                size_t CharsToWrite = sizeof(WCHAR);
                const TextAttribute realAttributes = pCookedReadData->_screenInfo.GetAttributes();
                pCookedReadData->_screenInfo.SetAttributes(Popup->Attributes);
                size_t NumSpaces;
                Status = WriteCharsLegacy(pCookedReadData->_screenInfo,
                                          Popup->NumberBuffer,
                                          &Popup->NumberBuffer[Popup->NumberRead],
                                          &Char,
                                          &CharsToWrite,
                                          &NumSpaces,
                                          pCookedReadData->_OriginalCursorPosition.X,
                                          WC_DESTRUCTIVE_BACKSPACE | WC_KEEP_CURSOR_VISIBLE | WC_ECHO,
                                          nullptr);

                FAIL_FAST_IF_NTSTATUS_FAILED(Status);
                pCookedReadData->_screenInfo.SetAttributes(realAttributes);
                Popup->NumberBuffer[Popup->NumberRead] = (WCHAR)' ';
                Popup->NumberRead -= 1;
            }
        }
        else if (Char == (WCHAR)VK_ESCAPE)
        {
            pCookedReadData->EndCurrentPopup();
            if (!CommandHistory->PopupList.empty())
            {
                pCookedReadData->EndCurrentPopup();
            }

            // Note that CookedReadData's OriginalCursorPosition is the position before ANY text was entered on the edit line.
            // We want to use the position before the cursor was moved for this popup handler specifically, which may be *anywhere* in the edit line
            // and will be synchronized with the pointers in the CookedReadData structure (BufPtr, etc.)
            LOG_IF_FAILED(pCookedReadData->_screenInfo.SetCursorPosition(pCookedReadData->BeforeDialogCursorPosition, TRUE));
        }
        else if (Char == UNICODE_CARRIAGERETURN)
        {
            CHAR NumberBuffer[6];
            int i;

            // This is guaranteed above.
            __analysis_assume(Popup->NumberRead < 6);
            for (i = 0; i < Popup->NumberRead; i++)
            {
                FAIL_FAST_IF_FALSE(i < ARRAYSIZE(NumberBuffer));
                NumberBuffer[i] = (CHAR)Popup->NumberBuffer[i];
            }
            NumberBuffer[i] = 0;

            SHORT CommandNumber = (SHORT)atoi(NumberBuffer);
            if ((WORD)CommandNumber >= (WORD)pCookedReadData->_CommandHistory->GetNumberOfCommands())
            {
                CommandNumber = (SHORT)(pCookedReadData->_CommandHistory->GetNumberOfCommands() - 1);
            }

            pCookedReadData->EndCurrentPopup();
            if (!CommandHistory->PopupList.empty())
            {
                pCookedReadData->EndCurrentPopup();
            }
            SetCurrentCommandLine(pCookedReadData, CommandNumber);
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
NTSTATUS CommandListPopup(_In_ COOKED_READ_DATA* const CookedReadData)
{
    CommandHistory* const CommandHistory = CookedReadData->_CommandHistory;
    if (CommandHistory &&
        CommandHistory->GetNumberOfCommands())
    {
        COORD PopupSize;
        PopupSize.X = 40;
        PopupSize.Y = 10;
        
        try
        {
            const auto Popup = CommandHistory->BeginPopup(CookedReadData->_screenInfo, PopupSize, Popup::PopFunc::CommandList);

            SHORT const CurrentCommand = CommandHistory->LastDisplayed;
            if (CurrentCommand < (SHORT)(CommandHistory->GetNumberOfCommands() - Popup->Height()))
            {
                Popup->BottomIndex = std::max(CurrentCommand, gsl::narrow<SHORT>(Popup->Height() - 1i16));
            }
            else
            {
                Popup->BottomIndex = (SHORT)(CommandHistory->GetNumberOfCommands() - 1);
            }

            Popup->Draw();

            return ProcessCommandListInput(CookedReadData);
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
NTSTATUS CopyFromCharPopup(_In_ COOKED_READ_DATA* CookedReadData)
{
    // Delete the current command from cursor position to the
    // letter specified by the user. The user is prompted via
    // popup to enter a character.
    if (CookedReadData->_CommandHistory)
    {
        COORD PopupSize;

        PopupSize.X = COPY_FROM_CHAR_PROMPT_LENGTH + 2;
        PopupSize.Y = 1;

        try
        {
            CommandHistory* const CommandHistory = CookedReadData->_CommandHistory;
            const auto Popup = CommandHistory->BeginPopup(CookedReadData->_screenInfo, PopupSize, Popup::PopFunc::CopyFromChar);
            Popup->Draw();

            return ProcessCopyFromCharInput(CookedReadData);
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
NTSTATUS CopyToCharPopup(_In_ COOKED_READ_DATA* CookedReadData)
{
    // copy the previous command to the current command, up to but
    // not including the character specified by the user.  the user
    // is prompted via popup to enter a character.
    if (CookedReadData->_CommandHistory)
    {
        COORD PopupSize;
        PopupSize.X = COPY_TO_CHAR_PROMPT_LENGTH + 2;
        PopupSize.Y = 1;

        try
        {
            CommandHistory* const CommandHistory = CookedReadData->_CommandHistory;
            const auto Popup = CommandHistory->BeginPopup(CookedReadData->_screenInfo, PopupSize, Popup::PopFunc::CopyToChar);
            Popup->Draw();

            return ProcessCopyToCharInput(CookedReadData);
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
HRESULT CommandNumberPopup(_In_ COOKED_READ_DATA* const CookedReadData)
{
    if (CookedReadData->_CommandHistory &&
        CookedReadData->_CommandHistory->GetNumberOfCommands() &&
        CookedReadData->_screenInfo.GetScreenBufferSize().X >= MINIMUM_COMMAND_PROMPT_SIZE + 2)
    {
        COORD PopupSize;
        // 2 is for border
        PopupSize.X = COMMAND_NUMBER_PROMPT_LENGTH + COMMAND_NUMBER_LENGTH;
        PopupSize.Y = 1;

        try
        {
            CommandHistory* const CommandHistory = CookedReadData->_CommandHistory;
            const auto Popup = CommandHistory->BeginPopup(CookedReadData->_screenInfo, PopupSize, Popup::PopFunc::CommandNumber);
            Popup->Draw();

            // Save the original cursor position in case the user cancels out of the dialog
            CookedReadData->BeforeDialogCursorPosition = CookedReadData->_screenInfo.GetTextBuffer().GetCursor().GetPosition();

            // Move the cursor into the dialog so the user can type multiple characters for the command number
            const COORD CursorPosition = Popup->GetCursorPosition();
            LOG_IF_FAILED(CookedReadData->_screenInfo.SetCursorPosition(CursorPosition, TRUE));

            // Transfer control to the handler routine
            return ProcessCommandNumberInput(CookedReadData);
        }
        CATCH_RETURN();
    }
    else
    {
        return S_FALSE;
    }
}


// TODO: [MSFT:4586207] Clean up this mess -- needs helpers. http://osgvsowi/4586207
// Routine Description:
// - This routine process command line editing keys.
// Return Value:
// - CONSOLE_STATUS_WAIT - CommandListPopup ran out of input
// - CONSOLE_STATUS_READ_COMPLETE - user hit <enter> in CommandListPopup
// - STATUS_SUCCESS - everything's cool
[[nodiscard]]
NTSTATUS ProcessCommandLine(_In_ COOKED_READ_DATA* pCookedReadData,
                            _In_ WCHAR wch,
                            const DWORD dwKeyState)
{
    const CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    COORD CurrentPosition = { 0 };
    size_t CharsToWrite;
    NTSTATUS Status;
    SHORT ScrollY = 0;
    const SHORT sScreenBufferSizeX = pCookedReadData->_screenInfo.GetScreenBufferSize().X;

    bool UpdateCursorPosition = false;
    if (wch == VK_F7 && (dwKeyState & (RIGHT_CTRL_PRESSED | LEFT_CTRL_PRESSED | RIGHT_ALT_PRESSED | LEFT_ALT_PRESSED)) == 0)
    {
        HRESULT hr = CommandListPopup(pCookedReadData);
        if (S_FALSE != hr)
        {
            return hr;
        }
        // Fall down if we couldn't process the popup.
    }
    else
    {
        switch (wch)
        {
        case VK_ESCAPE:
            DeleteCommandLine(pCookedReadData, true);
            break;
        case VK_UP:
        case VK_DOWN:
        case VK_F5:
            if (wch == VK_F5)
                wch = VK_UP;
            // for doskey compatibility, buffer isn't circular
            if (wch == VK_UP && !pCookedReadData->_CommandHistory->AtFirstCommand() || wch == VK_DOWN && !pCookedReadData->_CommandHistory->AtLastCommand())
            {
                DeleteCommandLine(pCookedReadData, true);
                Status = NTSTATUS_FROM_HRESULT(pCookedReadData->_CommandHistory->Retrieve(wch,
                                                                                          pCookedReadData->SpanWholeBuffer(),
                                                                                          pCookedReadData->_BytesRead));
                FAIL_FAST_IF_FALSE(pCookedReadData->_BackupLimit == pCookedReadData->_BufPtr);
                if (pCookedReadData->_Echo)
                {
                    Status = WriteCharsLegacy(pCookedReadData->_screenInfo,
                                              pCookedReadData->_BackupLimit,
                                              pCookedReadData->_BufPtr,
                                              pCookedReadData->_BufPtr,
                                              &pCookedReadData->_BytesRead,
                                              &pCookedReadData->_NumberOfVisibleChars,
                                              pCookedReadData->_OriginalCursorPosition.X,
                                              WC_DESTRUCTIVE_BACKSPACE | WC_KEEP_CURSOR_VISIBLE | WC_ECHO,
                                              &ScrollY);
                    FAIL_FAST_IF_NTSTATUS_FAILED(Status);
                    pCookedReadData->_OriginalCursorPosition.Y += ScrollY;
                }
                CharsToWrite = pCookedReadData->_BytesRead / sizeof(WCHAR);
                pCookedReadData->_CurrentPosition = CharsToWrite;
                pCookedReadData->_BufPtr = pCookedReadData->_BackupLimit + CharsToWrite;
            }
            break;
        case VK_PRIOR:
        case VK_NEXT:
            if (pCookedReadData->_CommandHistory && pCookedReadData->_CommandHistory->GetNumberOfCommands())
            {
                // display oldest or newest command
                SHORT CommandNumber;
                if (wch == VK_PRIOR)
                {
                    CommandNumber = 0;
                }
                else
                {
                    CommandNumber = (SHORT)(pCookedReadData->_CommandHistory->GetNumberOfCommands() - 1);
                }
                DeleteCommandLine(pCookedReadData, true);
                Status = NTSTATUS_FROM_HRESULT(pCookedReadData->_CommandHistory->RetrieveNth(CommandNumber,
                                                                                             pCookedReadData->SpanWholeBuffer(),
                                                                                             pCookedReadData->_BytesRead));
                FAIL_FAST_IF_FALSE(pCookedReadData->_BackupLimit == pCookedReadData->_BufPtr);
                if (pCookedReadData->_Echo)
                {
                    Status = WriteCharsLegacy(pCookedReadData->_screenInfo,
                                              pCookedReadData->_BackupLimit,
                                              pCookedReadData->_BufPtr,
                                              pCookedReadData->_BufPtr,
                                              &pCookedReadData->_BytesRead,
                                              &pCookedReadData->_NumberOfVisibleChars,
                                              pCookedReadData->_OriginalCursorPosition.X,
                                              WC_DESTRUCTIVE_BACKSPACE | WC_KEEP_CURSOR_VISIBLE | WC_ECHO,
                                              &ScrollY);
                    FAIL_FAST_IF_NTSTATUS_FAILED(Status);
                    pCookedReadData->_OriginalCursorPosition.Y += ScrollY;
                }
                CharsToWrite = pCookedReadData->_BytesRead / sizeof(WCHAR);
                pCookedReadData->_CurrentPosition = CharsToWrite;
                pCookedReadData->_BufPtr = pCookedReadData->_BackupLimit + CharsToWrite;
            }
            break;
        case VK_END:
            if (dwKeyState & (RIGHT_CTRL_PRESSED | LEFT_CTRL_PRESSED))
            {
                DeleteCommandLine(pCookedReadData, false);
                pCookedReadData->_BytesRead = pCookedReadData->_CurrentPosition * sizeof(WCHAR);
                if (pCookedReadData->_Echo)
                {
                    Status = WriteCharsLegacy(pCookedReadData->_screenInfo,
                                              pCookedReadData->_BackupLimit,
                                              pCookedReadData->_BackupLimit,
                                              pCookedReadData->_BackupLimit,
                                              &pCookedReadData->_BytesRead,
                                              &pCookedReadData->_NumberOfVisibleChars,
                                              pCookedReadData->_OriginalCursorPosition.X,
                                              WC_DESTRUCTIVE_BACKSPACE | WC_KEEP_CURSOR_VISIBLE | WC_ECHO,
                                              nullptr);
                    FAIL_FAST_IF_NTSTATUS_FAILED(Status);
                }
            }
            else
            {
                pCookedReadData->_CurrentPosition = pCookedReadData->_BytesRead / sizeof(WCHAR);
                pCookedReadData->_BufPtr = pCookedReadData->_BackupLimit + pCookedReadData->_CurrentPosition;
                CurrentPosition.X = (SHORT)(pCookedReadData->_OriginalCursorPosition.X + pCookedReadData->_NumberOfVisibleChars);
                CurrentPosition.Y = pCookedReadData->_OriginalCursorPosition.Y;
                if (CheckBisectProcessW(pCookedReadData->_screenInfo,
                                        pCookedReadData->_BackupLimit,
                                        pCookedReadData->_CurrentPosition,
                                        sScreenBufferSizeX - pCookedReadData->_OriginalCursorPosition.X,
                                        pCookedReadData->_OriginalCursorPosition.X,
                                        true))
                {
                    CurrentPosition.X++;
                }
                UpdateCursorPosition = true;
            }
            break;
        case VK_HOME:
            if (dwKeyState & (RIGHT_CTRL_PRESSED | LEFT_CTRL_PRESSED))
            {
                DeleteCommandLine(pCookedReadData, false);
                pCookedReadData->_BytesRead -= pCookedReadData->_CurrentPosition * sizeof(WCHAR);
                pCookedReadData->_CurrentPosition = 0;
                memmove(pCookedReadData->_BackupLimit, pCookedReadData->_BufPtr, pCookedReadData->_BytesRead);
                pCookedReadData->_BufPtr = pCookedReadData->_BackupLimit;
                if (pCookedReadData->_Echo)
                {
                    Status = WriteCharsLegacy(pCookedReadData->_screenInfo,
                                              pCookedReadData->_BackupLimit,
                                              pCookedReadData->_BackupLimit,
                                              pCookedReadData->_BackupLimit,
                                              &pCookedReadData->_BytesRead,
                                              &pCookedReadData->_NumberOfVisibleChars,
                                              pCookedReadData->_OriginalCursorPosition.X,
                                              WC_DESTRUCTIVE_BACKSPACE | WC_KEEP_CURSOR_VISIBLE | WC_ECHO,
                                              nullptr);
                    FAIL_FAST_IF_NTSTATUS_FAILED(Status);
                }
                CurrentPosition = pCookedReadData->_OriginalCursorPosition;
                UpdateCursorPosition = true;
            }
            else
            {
                pCookedReadData->_CurrentPosition = 0;
                pCookedReadData->_BufPtr = pCookedReadData->_BackupLimit;
                CurrentPosition = pCookedReadData->_OriginalCursorPosition;
                UpdateCursorPosition = true;
            }
            break;
        case VK_LEFT:
            if (dwKeyState & (RIGHT_CTRL_PRESSED | LEFT_CTRL_PRESSED))
            {
                PWCHAR LastWord;
                if (pCookedReadData->_BufPtr != pCookedReadData->_BackupLimit)
                {
                    // A bit better word skipping.
                    LastWord = pCookedReadData->_BufPtr - 1;
                    if (LastWord != pCookedReadData->_BackupLimit)
                    {
                        if (*LastWord == L' ')
                        {
                            // Skip spaces, until the non-space character is found.
                            while (--LastWord != pCookedReadData->_BackupLimit)
                            {
                                FAIL_FAST_IF_FALSE(LastWord > pCookedReadData->_BackupLimit);
                                if (*LastWord != L' ')
                                {
                                    break;
                                }
                            }
                        }
                        if (LastWord != pCookedReadData->_BackupLimit)
                        {
                            if (IsWordDelim(*LastWord))
                            {
                                // Skip WORD_DELIMs until space or non WORD_DELIM is found.
                                while (--LastWord != pCookedReadData->_BackupLimit)
                                {
                                    FAIL_FAST_IF_FALSE(LastWord > pCookedReadData->_BackupLimit);
                                    if (*LastWord == L' ' || !IsWordDelim(*LastWord))
                                    {
                                        break;
                                    }
                                }
                            }
                            else
                            {
                                // Skip the regular words
                                while (--LastWord != pCookedReadData->_BackupLimit)
                                {
                                    FAIL_FAST_IF_FALSE(LastWord > pCookedReadData->_BackupLimit);
                                    if (IsWordDelim(*LastWord))
                                    {
                                        break;
                                    }
                                }
                            }
                        }
                        FAIL_FAST_IF_FALSE(LastWord >= pCookedReadData->_BackupLimit);
                        if (LastWord != pCookedReadData->_BackupLimit)
                        {
                            /*
                             * LastWord is currently pointing to the last character
                             * of the previous word, unless it backed up to the beginning
                             * of the buffer.
                             * Let's increment LastWord so that it points to the expeced
                             * insertion point.
                             */
                            ++LastWord;
                        }
                        pCookedReadData->_BufPtr = LastWord;
                    }
                    pCookedReadData->_CurrentPosition = (ULONG)(pCookedReadData->_BufPtr - pCookedReadData->_BackupLimit);
                    CurrentPosition = pCookedReadData->_OriginalCursorPosition;
                    CurrentPosition.X = (SHORT)(CurrentPosition.X +
                                                RetrieveTotalNumberOfSpaces(pCookedReadData->_OriginalCursorPosition.X,
                                                                            pCookedReadData->_BackupLimit, pCookedReadData->_CurrentPosition));
                    if (CheckBisectStringW(pCookedReadData->_BackupLimit,
                                           pCookedReadData->_CurrentPosition + 1,
                                           sScreenBufferSizeX - pCookedReadData->_OriginalCursorPosition.X))
                    {
                        CurrentPosition.X++;
                    }

                    UpdateCursorPosition = true;
                }
            }
            else
            {
                if (pCookedReadData->_BufPtr != pCookedReadData->_BackupLimit)
                {
                    pCookedReadData->_BufPtr--;
                    pCookedReadData->_CurrentPosition--;
                    CurrentPosition.X = pCookedReadData->_screenInfo.GetTextBuffer().GetCursor().GetPosition().X;
                    CurrentPosition.Y = pCookedReadData->_screenInfo.GetTextBuffer().GetCursor().GetPosition().Y;
                    CurrentPosition.X = (SHORT)(CurrentPosition.X -
                                                RetrieveNumberOfSpaces(pCookedReadData->_OriginalCursorPosition.X,
                                                                       pCookedReadData->_BackupLimit,
                                                                       pCookedReadData->_CurrentPosition));
                    if (CheckBisectProcessW(pCookedReadData->_screenInfo,
                                            pCookedReadData->_BackupLimit,
                                            pCookedReadData->_CurrentPosition + 2,
                                            sScreenBufferSizeX - pCookedReadData->_OriginalCursorPosition.X,
                                            pCookedReadData->_OriginalCursorPosition.X,
                                            true))
                    {
                        if ((CurrentPosition.X == -2) || (CurrentPosition.X == -1))
                        {
                            CurrentPosition.X--;
                        }
                    }

                    UpdateCursorPosition = true;
                }
            }
            break;
        case VK_RIGHT:
        case VK_F1:
            // we don't need to check for end of buffer here because we've
            // already done it.
            if (dwKeyState & (RIGHT_CTRL_PRESSED | LEFT_CTRL_PRESSED))
            {
                if (wch != VK_F1)
                {
                    if (pCookedReadData->_CurrentPosition < (pCookedReadData->_BytesRead / sizeof(WCHAR)))
                    {
                        PWCHAR NextWord = pCookedReadData->_BufPtr;

                        // A bit better word skipping.
                        PWCHAR BufLast = pCookedReadData->_BackupLimit + pCookedReadData->_BytesRead / sizeof(WCHAR);

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

                        pCookedReadData->_BufPtr = NextWord;
                        pCookedReadData->_CurrentPosition = (ULONG)(pCookedReadData->_BufPtr - pCookedReadData->_BackupLimit);
                        CurrentPosition = pCookedReadData->_OriginalCursorPosition;
                        CurrentPosition.X = (SHORT)(CurrentPosition.X +
                                                    RetrieveTotalNumberOfSpaces(pCookedReadData->_OriginalCursorPosition.X,
                                                                                pCookedReadData->_BackupLimit,
                                                                                pCookedReadData->_CurrentPosition));
                        if (CheckBisectStringW(pCookedReadData->_BackupLimit,
                                               pCookedReadData->_CurrentPosition + 1,
                                               sScreenBufferSizeX - pCookedReadData->_OriginalCursorPosition.X))
                        {
                            CurrentPosition.X++;
                        }
                        UpdateCursorPosition = true;
                    }
                }
            }
            else
            {
                // If not at the end of the line, move cursor position right.
                if (pCookedReadData->_CurrentPosition < (pCookedReadData->_BytesRead / sizeof(WCHAR)))
                {
                    CurrentPosition = pCookedReadData->_screenInfo.GetTextBuffer().GetCursor().GetPosition();
                    CurrentPosition.X = (SHORT)(CurrentPosition.X +
                                                RetrieveNumberOfSpaces(pCookedReadData->_OriginalCursorPosition.X,
                                                                       pCookedReadData->_BackupLimit,
                                                                       pCookedReadData->_CurrentPosition));
                    if (CheckBisectProcessW(pCookedReadData->_screenInfo,
                                            pCookedReadData->_BackupLimit,
                                            pCookedReadData->_CurrentPosition + 2,
                                            sScreenBufferSizeX - pCookedReadData->_OriginalCursorPosition.X,
                                            pCookedReadData->_OriginalCursorPosition.X,
                                            true))
                    {
                        if (CurrentPosition.X == (sScreenBufferSizeX - 1))
                            CurrentPosition.X++;
                    }

                    pCookedReadData->_BufPtr++;
                    pCookedReadData->_CurrentPosition++;
                    UpdateCursorPosition = true;

                    // if at the end of the line, copy a character from the same position in the last command
                }
                else if (pCookedReadData->_CommandHistory)
                {
                    size_t NumSpaces;
                    const auto LastCommand = pCookedReadData->_CommandHistory->GetLastCommand();
                    if (!LastCommand.empty() && LastCommand.size() > pCookedReadData->_CurrentPosition)
                    {
                        *pCookedReadData->_BufPtr = LastCommand[pCookedReadData->_CurrentPosition];
                        pCookedReadData->_BytesRead += sizeof(WCHAR);
                        pCookedReadData->_CurrentPosition++;
                        if (pCookedReadData->_Echo)
                        {
                            CharsToWrite = sizeof(WCHAR);
                            Status = WriteCharsLegacy(pCookedReadData->_screenInfo,
                                                      pCookedReadData->_BackupLimit,
                                                      pCookedReadData->_BufPtr,
                                                      pCookedReadData->_BufPtr,
                                                      &CharsToWrite,
                                                      &NumSpaces,
                                                      pCookedReadData->_OriginalCursorPosition.X,
                                                      WC_DESTRUCTIVE_BACKSPACE | WC_KEEP_CURSOR_VISIBLE | WC_ECHO,
                                                      &ScrollY);
                            FAIL_FAST_IF_NTSTATUS_FAILED(Status);
                            pCookedReadData->_OriginalCursorPosition.Y += ScrollY;
                            pCookedReadData->_NumberOfVisibleChars += NumSpaces;
                        }
                        pCookedReadData->_BufPtr += 1;
                    }
                }
            }
            break;

        case VK_F2:
        {
            const HRESULT hr = CopyToCharPopup(pCookedReadData);
            if (S_FALSE == hr)
            {
                // We couldn't make the popup, so loop around and read the next character.
                break;
            }
            else
            {
                return hr;
            }
        }
        case VK_F3:
            // Copy the remainder of the previous command to the current command.
            if (pCookedReadData->_CommandHistory)
            {
                size_t NumSpaces, cchCount;

                const auto LastCommand = pCookedReadData->_CommandHistory->GetLastCommand();
                if (!LastCommand.empty() && LastCommand.size() > pCookedReadData->_CurrentPosition)
                {
                    cchCount = LastCommand.size() - pCookedReadData->_CurrentPosition;
                    const auto bufferSpan = pCookedReadData->SpanAtPointer();
                    std::copy_n(LastCommand.cbegin() + pCookedReadData->_CurrentPosition, cchCount, bufferSpan.begin());
                    pCookedReadData->_CurrentPosition += cchCount;
                    cchCount *= sizeof(WCHAR);
                    pCookedReadData->_BytesRead = std::max(LastCommand.size() * sizeof(wchar_t), pCookedReadData->_BytesRead);
                    if (pCookedReadData->_Echo)
                    {
                        Status = WriteCharsLegacy(pCookedReadData->_screenInfo,
                                                  pCookedReadData->_BackupLimit,
                                                  pCookedReadData->_BufPtr,
                                                  pCookedReadData->_BufPtr,
                                                  &cchCount,
                                                  &NumSpaces,
                                                  pCookedReadData->_OriginalCursorPosition.X,
                                                  WC_DESTRUCTIVE_BACKSPACE | WC_KEEP_CURSOR_VISIBLE | WC_ECHO,
                                                  &ScrollY);
                        FAIL_FAST_IF_NTSTATUS_FAILED(Status);
                        pCookedReadData->_OriginalCursorPosition.Y += ScrollY;
                        pCookedReadData->_NumberOfVisibleChars += NumSpaces;
                    }
                    pCookedReadData->_BufPtr += cchCount / sizeof(WCHAR);
                }

            }
            break;

        case VK_F4:
        {
            const HRESULT hr = CopyFromCharPopup(pCookedReadData);
            if (S_FALSE == hr)
            {
                // We couldn't display a popup. Go around a loop behind.
                break;
            }
            else
            {
                return hr;
            }
        }
        case VK_F6:
        {
            // place a ctrl-z in the current command line
            size_t NumSpaces = 0;

            *pCookedReadData->_BufPtr = (WCHAR)0x1a;  // ctrl-z
            pCookedReadData->_BytesRead += sizeof(WCHAR);
            pCookedReadData->_CurrentPosition++;
            if (pCookedReadData->_Echo)
            {
                CharsToWrite = sizeof(WCHAR);
                Status = WriteCharsLegacy(pCookedReadData->_screenInfo,
                                          pCookedReadData->_BackupLimit,
                                          pCookedReadData->_BufPtr,
                                          pCookedReadData->_BufPtr,
                                          &CharsToWrite,
                                          &NumSpaces,
                                          pCookedReadData->_OriginalCursorPosition.X,
                                          WC_DESTRUCTIVE_BACKSPACE | WC_KEEP_CURSOR_VISIBLE | WC_ECHO,
                                          &ScrollY);
                FAIL_FAST_IF_NTSTATUS_FAILED(Status);
                pCookedReadData->_OriginalCursorPosition.Y += ScrollY;
                pCookedReadData->_NumberOfVisibleChars += NumSpaces;
            }
            pCookedReadData->_BufPtr += 1;
            break;
        }
        case VK_F7:
            if (dwKeyState & (RIGHT_ALT_PRESSED | LEFT_ALT_PRESSED))
            {
                if (pCookedReadData->_CommandHistory)
                {
                    pCookedReadData->_CommandHistory->Empty();
                    pCookedReadData->_CommandHistory->Flags |= CLE_ALLOCATED;
                }
            }
            break;

        case VK_F8:
            if (pCookedReadData->_CommandHistory)
            {
                // Cycles through the stored commands that start with the characters in the current command.
                SHORT index;
                if (pCookedReadData->_CommandHistory->FindMatchingCommand({ pCookedReadData->_BackupLimit, pCookedReadData->_CurrentPosition },
                                                                          pCookedReadData->_CommandHistory->LastDisplayed,
                                                                          index,
                                                                          CommandHistory::MatchOptions::None))
                {
                    SHORT CurrentPos;
                    COORD CursorPosition;

                    // save cursor position
                    CurrentPos = (SHORT)pCookedReadData->_CurrentPosition;
                    CursorPosition = pCookedReadData->_screenInfo.GetTextBuffer().GetCursor().GetPosition();

                    DeleteCommandLine(pCookedReadData, true);
                    Status = NTSTATUS_FROM_HRESULT(pCookedReadData->_CommandHistory->RetrieveNth((SHORT)index,
                                                                                                 pCookedReadData->SpanWholeBuffer(),
                                                                                                 pCookedReadData->_BytesRead));
                    FAIL_FAST_IF_FALSE(pCookedReadData->_BackupLimit == pCookedReadData->_BufPtr);
                    if (pCookedReadData->_Echo)
                    {
                        Status = WriteCharsLegacy(pCookedReadData->_screenInfo,
                                                  pCookedReadData->_BackupLimit,
                                                  pCookedReadData->_BufPtr,
                                                  pCookedReadData->_BufPtr,
                                                  &pCookedReadData->_BytesRead,
                                                  &pCookedReadData->_NumberOfVisibleChars,
                                                  pCookedReadData->_OriginalCursorPosition.X,
                                                  WC_DESTRUCTIVE_BACKSPACE | WC_KEEP_CURSOR_VISIBLE | WC_ECHO,
                                                  &ScrollY);
                        FAIL_FAST_IF_NTSTATUS_FAILED(Status);
                        pCookedReadData->_OriginalCursorPosition.Y += ScrollY;
                    }
                    CursorPosition.Y += ScrollY;

                    // restore cursor position
                    pCookedReadData->_BufPtr = pCookedReadData->_BackupLimit + CurrentPos;
                    pCookedReadData->_CurrentPosition = CurrentPos;
                    Status = pCookedReadData->_screenInfo.SetCursorPosition(CursorPosition, true);
                    FAIL_FAST_IF_NTSTATUS_FAILED(Status);
                }
            }
            break;
        case VK_F9:
        {
            const HRESULT hr = CommandNumberPopup(pCookedReadData);
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
        case VK_F10:
            // Alt+F10 clears the aliases for specifically cmd.exe.
            if (dwKeyState & (RIGHT_ALT_PRESSED | LEFT_ALT_PRESSED))
            {
                Alias::s_ClearCmdExeAliases();
            }
            break;
        case VK_INSERT:
            pCookedReadData->_InsertMode = !pCookedReadData->_InsertMode;
            pCookedReadData->_screenInfo.SetCursorDBMode((!!pCookedReadData->_InsertMode != gci.GetInsertMode()));
            break;
        case VK_DELETE:
            if (!pCookedReadData->AtEol())
            {
                COORD CursorPosition;

                bool fStartFromDelim = IsWordDelim(*pCookedReadData->_BufPtr);

            del_repeat:
                // save cursor position
                CursorPosition = pCookedReadData->_screenInfo.GetTextBuffer().GetCursor().GetPosition();

                // Delete commandline.
#pragma prefast(suppress:__WARNING_BUFFER_OVERFLOW, "Not sure why prefast is getting confused here")
                DeleteCommandLine(pCookedReadData, false);

                // Delete char.
                pCookedReadData->_BytesRead -= sizeof(WCHAR);
                memmove(pCookedReadData->_BufPtr,
                        pCookedReadData->_BufPtr + 1,
                        pCookedReadData->_BytesRead - (pCookedReadData->_CurrentPosition * sizeof(WCHAR)));

                {
                    PWCHAR buf = (PWCHAR)((PBYTE)pCookedReadData->_BackupLimit + pCookedReadData->_BytesRead);
                    *buf = (WCHAR)' ';
                }

                // Write commandline.
                if (pCookedReadData->_Echo)
                {
                    Status = WriteCharsLegacy(pCookedReadData->_screenInfo,
                                              pCookedReadData->_BackupLimit,
                                              pCookedReadData->_BackupLimit,
                                              pCookedReadData->_BackupLimit,
                                              &pCookedReadData->_BytesRead,
                                              &pCookedReadData->_NumberOfVisibleChars,
                                              pCookedReadData->_OriginalCursorPosition.X,
                                              WC_DESTRUCTIVE_BACKSPACE | WC_KEEP_CURSOR_VISIBLE | WC_ECHO,
                                              nullptr);
                    FAIL_FAST_IF_NTSTATUS_FAILED(Status);
                }

                // restore cursor position
                if (CheckBisectProcessW(pCookedReadData->_screenInfo,
                                        pCookedReadData->_BackupLimit,
                                        pCookedReadData->_CurrentPosition + 1,
                                        sScreenBufferSizeX - pCookedReadData->_OriginalCursorPosition.X,
                                        pCookedReadData->_OriginalCursorPosition.X,
                                        true))
                {
                    CursorPosition.X++;
                }
                CurrentPosition = CursorPosition;
                if (pCookedReadData->_Echo)
                {
                    Status = AdjustCursorPosition(pCookedReadData->_screenInfo, CurrentPosition, true, nullptr);
                    FAIL_FAST_IF_NTSTATUS_FAILED(Status);
                }

                // If Ctrl key is pressed, delete a word.
                // If the start point was word delimiter, just remove delimiters portion only.
                if ((dwKeyState & CTRL_PRESSED) &&
                    !pCookedReadData->AtEol() &&
                    fStartFromDelim ^ !IsWordDelim(*pCookedReadData->_BufPtr))
                {
                    goto del_repeat;
                }
            }
            break;
        default:
            FAIL_FAST_HR(E_NOTIMPL);
            break;
        }
    }

    if (UpdateCursorPosition && pCookedReadData->_Echo)
    {
        Status = AdjustCursorPosition(pCookedReadData->_screenInfo, CurrentPosition, true, nullptr);
        FAIL_FAST_IF_NTSTATUS_FAILED(Status);
    }

    return STATUS_SUCCESS;
}
