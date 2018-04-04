/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/
#include "precomp.h"

#include "cmdline.h"

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

#define COPY_TO_CHAR_PROMPT_LENGTH 26
#define COPY_FROM_CHAR_PROMPT_LENGTH 28

#define COMMAND_NUMBER_PROMPT_LENGTH 22
#define COMMAND_NUMBER_LENGTH 5
#define MINIMUM_COMMAND_PROMPT_SIZE COMMAND_NUMBER_LENGTH

#define POPUP_SIZE_X(POPUP) (SHORT)(((POPUP)->Region.Right - (POPUP)->Region.Left - 1))
#define POPUP_SIZE_Y(POPUP) (SHORT)(((POPUP)->Region.Bottom - (POPUP)->Region.Top - 1))
#define COMMAND_NUMBER_SIZE 8   // size of command number buffer



#define UCLP_WRAP   1

// fwd decls
void DrawCommandListBorder(_In_ PCLE_POPUP const Popup, _In_ PSCREEN_INFORMATION const ScreenInfo);
PCOMMAND GetLastCommand(_In_ PCOMMAND_HISTORY CommandHistory);
SHORT FindMatchingCommand(_In_ PCOMMAND_HISTORY CommandHistory,
                          _In_reads_bytes_(CurrentCommandLength) PCWCHAR CurrentCommand,
                          _In_ ULONG CurrentCommandLength,
                          _In_ SHORT CurrentIndex,
                          _In_ DWORD Flags);
[[nodiscard]]
NTSTATUS CommandNumberPopup(_In_ COOKED_READ_DATA* const CookedReadData);
void DrawCommandListPopup(_In_ PCLE_POPUP const Popup,
                          _In_ SHORT const CurrentCommand,
                          _In_ PCOMMAND_HISTORY const CommandHistory,
                          _In_ PSCREEN_INFORMATION const ScreenInfo);
void UpdateCommandListPopup(_In_ SHORT Delta,
                            _Inout_ PSHORT CurrentCommand,
                            _In_ PCOMMAND_HISTORY const CommandHistory,
                            _In_ PCLE_POPUP Popup,
                            _In_ PSCREEN_INFORMATION const ScreenInfo,
                            _In_ DWORD const Flags);
[[nodiscard]]
NTSTATUS RetrieveCommand(_In_ PCOMMAND_HISTORY CommandHistory,
                         _In_ WORD VirtualKeyCode,
                         _In_reads_bytes_(BufferSize) PWCHAR Buffer,
                         _In_ ULONG BufferSize,
                         _Out_ PULONG CommandSize);
UINT LoadStringEx(_In_ HINSTANCE hModule, _In_ UINT wID, _Out_writes_(cchBufferMax) LPWSTR lpBuffer, _In_ UINT cchBufferMax, _In_ WORD wLangId);

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

[[nodiscard]]
NTSTATUS BeginPopup(_In_ PSCREEN_INFORMATION ScreenInfo, _In_ PCOMMAND_HISTORY CommandHistory, _In_ COORD PopupSize)
{
    CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    // determine popup dimensions
    COORD Size = PopupSize;
    Size.X += 2;    // add borders
    Size.Y += 2;    // add borders
    if (Size.X >= (SHORT)(ScreenInfo->GetScreenWindowSizeX()))
    {
        Size.X = (SHORT)(ScreenInfo->GetScreenWindowSizeX());
    }
    if (Size.Y >= (SHORT)(ScreenInfo->GetScreenWindowSizeY()))
    {
        Size.Y = (SHORT)(ScreenInfo->GetScreenWindowSizeY());
    }

    // make sure there's enough room for the popup borders

    if (Size.X < 2 || Size.Y < 2)
    {
        return STATUS_BUFFER_TOO_SMALL;
    }

    // determine origin.  center popup on window
    COORD Origin;
    Origin.X = (SHORT)((ScreenInfo->GetScreenWindowSizeX() - Size.X) / 2 + ScreenInfo->GetBufferViewport().Left);
    Origin.Y = (SHORT)((ScreenInfo->GetScreenWindowSizeY() - Size.Y) / 2 + ScreenInfo->GetBufferViewport().Top);

    // allocate a popup structure
    PCLE_POPUP const Popup = new CLE_POPUP();
    if (Popup == nullptr)
    {
        return STATUS_NO_MEMORY;
    }

    // allocate a buffer
    Popup->OldScreenSize = ScreenInfo->GetScreenBufferSize();
    const size_t cOldContents = Popup->OldScreenSize.X * Size.Y;
    Popup->OldContents = new CHAR_INFO[cOldContents];
    if (Popup->OldContents == nullptr)
    {
        delete Popup;
        return STATUS_NO_MEMORY;
    }

    // fill in popup structure
    InsertHeadList(&CommandHistory->PopupList, &Popup->ListLink);
    Popup->Region.Left = Origin.X;
    Popup->Region.Top = Origin.Y;
    Popup->Region.Right = (SHORT)(Origin.X + Size.X - 1);
    Popup->Region.Bottom = (SHORT)(Origin.Y + Size.Y - 1);
    Popup->Attributes = ScreenInfo->GetPopupAttributes()->GetLegacyAttributes();
    Popup->BottomIndex = COMMAND_INDEX_TO_NUM(CommandHistory->LastDisplayed, CommandHistory);

    // copy old contents
    SMALL_RECT TargetRect;
    TargetRect.Left = 0;
    TargetRect.Top = Popup->Region.Top;
    TargetRect.Right = Popup->OldScreenSize.X - 1;
    TargetRect.Bottom = Popup->Region.Bottom;
    std::vector<std::vector<OutputCell>> outputCells;
    LOG_IF_FAILED(ReadScreenBuffer(ScreenInfo, outputCells, &TargetRect));
    assert(!outputCells.empty());
    assert(cOldContents == outputCells.size() * outputCells[0].size());
    // copy the data into the char info buffer
    CHAR_INFO* pCurrCharInfo = Popup->OldContents;
    for (auto& row : outputCells)
    {
        for (auto& cell : row)
        {
            *pCurrCharInfo = cell.ToCharInfo();
            ++pCurrCharInfo;
        }
    }


    gci.PopupCount++;
    if (1 == gci.PopupCount)
    {
        // If this is the first popup to be shown, stop the cursor from appearing/blinking
        ScreenInfo->TextInfo->GetCursor()->SetIsPopupShown(true);
    }

    DrawCommandListBorder(Popup, ScreenInfo);
    return STATUS_SUCCESS;
}

[[nodiscard]]
NTSTATUS EndPopup(_In_ PSCREEN_INFORMATION ScreenInfo, _In_ PCOMMAND_HISTORY CommandHistory)
{
    CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    ASSERT(!CLE_NO_POPUPS(CommandHistory));
    if (CLE_NO_POPUPS(CommandHistory))
    {
        return STATUS_UNSUCCESSFUL;
    }

    PCLE_POPUP const Popup = CONTAINING_RECORD(CommandHistory->PopupList.Flink, CLE_POPUP, ListLink);

    // restore previous contents to screen
    COORD Size;
    Size.X = Popup->OldScreenSize.X;
    Size.Y = (SHORT)(Popup->Region.Bottom - Popup->Region.Top + 1);

    SMALL_RECT SourceRect;
    SourceRect.Left = 0;
    SourceRect.Top = Popup->Region.Top;
    SourceRect.Right = Popup->OldScreenSize.X - 1;
    SourceRect.Bottom = Popup->Region.Bottom;

    LOG_IF_FAILED(WriteScreenBuffer(ScreenInfo, Popup->OldContents, &SourceRect));
    WriteToScreen(ScreenInfo, SourceRect);

    // Free popup structure.
    RemoveEntryList(&Popup->ListLink);
    delete[] Popup->OldContents;
    delete Popup;
    gci.PopupCount--;

    if (gci.PopupCount == 0)
    {
        // Notify we're done showing popups.
        ScreenInfo->TextInfo->GetCursor()->SetIsPopupShown(false);
    }

    return STATUS_SUCCESS;
}

void CleanUpPopups(_In_ COOKED_READ_DATA* const CookedReadData)
{
    PCOMMAND_HISTORY const CommandHistory = CookedReadData->_CommandHistory;
    if (CommandHistory == nullptr)
    {
        return;
    }

    while (!CLE_NO_POPUPS(CommandHistory))
    {
        LOG_IF_FAILED(EndPopup(CookedReadData->_pScreenInfo, CommandHistory));
    }
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

void CommandLine::Hide(_In_ bool const fUpdateFields)
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

void DeleteCommandLine(_Inout_ COOKED_READ_DATA* const pCookedReadData, _In_ const BOOL fUpdateFields)
{
    DWORD CharsToWrite = pCookedReadData->_NumberOfVisibleChars;
    COORD coordOriginalCursor = pCookedReadData->_OriginalCursorPosition;
    const COORD coordBufferSize = pCookedReadData->_pScreenInfo->GetScreenBufferSize();

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

    LOG_IF_FAILED(FillOutput(pCookedReadData->_pScreenInfo,
                             L' ',
                             coordOriginalCursor,
                             CONSOLE_FALSE_UNICODE,    // faster than real unicode
                             &CharsToWrite));

    if (fUpdateFields)
    {
        pCookedReadData->_BufPtr = pCookedReadData->_BackupLimit;
        pCookedReadData->_BytesRead = 0;
        pCookedReadData->_CurrentPosition = 0;
        pCookedReadData->_NumberOfVisibleChars = 0;
    }

    LOG_IF_FAILED(pCookedReadData->_pScreenInfo->SetCursorPosition(pCookedReadData->_OriginalCursorPosition, true));
}

void RedrawCommandLine(_Inout_ COOKED_READ_DATA* const pCookedReadData)
{
    if (pCookedReadData->_Echo)
    {
        // Draw the command line
        pCookedReadData->_OriginalCursorPosition = pCookedReadData->_pScreenInfo->TextInfo->GetCursor()->GetPosition();

        SHORT ScrollY = 0;
#pragma prefast(suppress:28931, "Status is not unused. It's used in debug assertions.")
        NTSTATUS Status = WriteCharsLegacy(pCookedReadData->_pScreenInfo,
                                           pCookedReadData->_BackupLimit,
                                           pCookedReadData->_BackupLimit,
                                           pCookedReadData->_BackupLimit,
                                           &pCookedReadData->_BytesRead,
                                           &pCookedReadData->_NumberOfVisibleChars,
                                           pCookedReadData->_OriginalCursorPosition.X,
                                           WC_DESTRUCTIVE_BACKSPACE | WC_KEEP_CURSOR_VISIBLE | WC_ECHO,
                                           &ScrollY);
        ASSERT(NT_SUCCESS(Status));

        pCookedReadData->_OriginalCursorPosition.Y += ScrollY;

        // Move the cursor back to the right position
        COORD CursorPosition = pCookedReadData->_OriginalCursorPosition;
        CursorPosition.X += (SHORT)RetrieveTotalNumberOfSpaces(pCookedReadData->_OriginalCursorPosition.X,
                                                               pCookedReadData->_BackupLimit,
                                                               pCookedReadData->_CurrentPosition);
        if (CheckBisectStringW(pCookedReadData->_BackupLimit,
                               pCookedReadData->_CurrentPosition,
                               pCookedReadData->_pScreenInfo->GetScreenBufferSize().X - pCookedReadData->_OriginalCursorPosition.X))
        {
            CursorPosition.X++;
        }
        Status = AdjustCursorPosition(pCookedReadData->_pScreenInfo, CursorPosition, TRUE, nullptr);
        ASSERT(NT_SUCCESS(Status));
    }
}

// Routine Description:
// - This routine copies the commandline specified by Index into the cooked read buffer
void SetCurrentCommandLine(_In_ COOKED_READ_DATA* const CookedReadData, _In_ SHORT Index) // index, not command number
{
    DeleteCommandLine(CookedReadData, TRUE);
#pragma prefast(suppress:28931, "Status is not unused. Used by assertions.")
    NTSTATUS Status = RetrieveNthCommand(CookedReadData->_CommandHistory, Index, CookedReadData->_BackupLimit, CookedReadData->_BufferSize, &CookedReadData->_BytesRead);
    ASSERT(NT_SUCCESS(Status));
    ASSERT(CookedReadData->_BackupLimit == CookedReadData->_BufPtr);
    if (CookedReadData->_Echo)
    {
        SHORT ScrollY = 0;
        Status = WriteCharsLegacy(CookedReadData->_pScreenInfo,
                                  CookedReadData->_BackupLimit,
                                  CookedReadData->_BufPtr,
                                  CookedReadData->_BufPtr,
                                  &CookedReadData->_BytesRead,
                                  &CookedReadData->_NumberOfVisibleChars,
                                  CookedReadData->_OriginalCursorPosition.X,
                                  WC_DESTRUCTIVE_BACKSPACE | WC_KEEP_CURSOR_VISIBLE | WC_ECHO,
                                  &ScrollY);
        ASSERT(NT_SUCCESS(Status));
        CookedReadData->_OriginalCursorPosition.Y += ScrollY;
    }

    DWORD const CharsToWrite = CookedReadData->_BytesRead / sizeof(WCHAR);
    CookedReadData->_CurrentPosition = CharsToWrite;
    CookedReadData->_BufPtr = CookedReadData->_BackupLimit + CharsToWrite;
}

// Routine Description:
// - This routine handles the command list popup.  It returns when we're out of input or the user has selected a command line.
// Return Value:
// - CONSOLE_STATUS_WAIT - we ran out of input, so a wait block was created
// - CONSOLE_STATUS_READ_COMPLETE - user hit return
[[nodiscard]]
NTSTATUS ProcessCommandListInput(_In_ COOKED_READ_DATA* const pCookedReadData)
{
    const CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    PCOMMAND_HISTORY const pCommandHistory = pCookedReadData->_CommandHistory;
    PCLE_POPUP const Popup = CONTAINING_RECORD(pCommandHistory->PopupList.Flink, CLE_POPUP, ListLink);
    NTSTATUS Status = STATUS_SUCCESS;
    INPUT_READ_HANDLE_DATA* const pInputReadHandleData = pCookedReadData->GetInputReadHandleData();
    InputBuffer* const pInputBuffer = pCookedReadData->GetInputBuffer();

    for (;;)
    {
        WCHAR Char;
        bool commandLinePopupKeys = false;

        Status = GetChar(pInputBuffer,
                         &Char,
                         true,
                         nullptr,
                         &commandLinePopupKeys,
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
        if (commandLinePopupKeys)
        {
            switch (Char)
            {
            case VK_F9:
            {
                // prompt the user to enter the desired command number. copy that command to the command line.
                COORD PopupSize;
                if (pCookedReadData->_CommandHistory &&
                    pCookedReadData->_pScreenInfo->GetScreenBufferSize().X >= MINIMUM_COMMAND_PROMPT_SIZE + 2)
                {
                    // 2 is for border
                    PopupSize.X = COMMAND_NUMBER_PROMPT_LENGTH + COMMAND_NUMBER_LENGTH;
                    PopupSize.Y = 1;
                    Status = BeginPopup(pCookedReadData->_pScreenInfo, pCookedReadData->_CommandHistory, PopupSize);
                    if (NT_SUCCESS(Status))
                    {
                        // CommandNumberPopup does EndPopup call
                        return CommandNumberPopup(pCookedReadData);
                    }
                }
                break;
            }
            case VK_ESCAPE:
                LOG_IF_FAILED(EndPopup(pCookedReadData->_pScreenInfo, pCommandHistory));
                return CONSOLE_STATUS_WAIT_NO_BLOCK;
            case VK_UP:
                UpdateCommandListPopup(-1, &Popup->CurrentCommand, pCommandHistory, Popup, pCookedReadData->_pScreenInfo, 0);
                break;
            case VK_DOWN:
                UpdateCommandListPopup(1, &Popup->CurrentCommand, pCommandHistory, Popup, pCookedReadData->_pScreenInfo, 0);
                break;
            case VK_END:
                // Move waaay forward, UpdateCommandListPopup() can handle it.
                UpdateCommandListPopup((SHORT)(pCommandHistory->NumberOfCommands),
                                       &Popup->CurrentCommand,
                                       pCommandHistory,
                                       Popup,
                                       pCookedReadData->_pScreenInfo,
                                       0);
                break;
            case VK_HOME:
                // Move waaay back, UpdateCommandListPopup() can handle it.
                UpdateCommandListPopup((SHORT)-(pCommandHistory->NumberOfCommands),
                                       &Popup->CurrentCommand,
                                       pCommandHistory,
                                       Popup,
                                       pCookedReadData->_pScreenInfo,
                                       0);
                break;
            case VK_PRIOR:
                UpdateCommandListPopup((SHORT)-POPUP_SIZE_Y(Popup),
                                       &Popup->CurrentCommand,
                                       pCommandHistory,
                                       Popup,
                                       pCookedReadData->_pScreenInfo,
                                       0);
                break;
            case VK_NEXT:
                UpdateCommandListPopup(POPUP_SIZE_Y(Popup),
                                       &Popup->CurrentCommand,
                                       pCommandHistory,
                                       Popup,
                                       pCookedReadData->_pScreenInfo, 0);
                break;
            case VK_LEFT:
            case VK_RIGHT:
                Index = Popup->CurrentCommand;
                LOG_IF_FAILED(EndPopup(pCookedReadData->_pScreenInfo, pCommandHistory));
                SetCurrentCommandLine(pCookedReadData, Index);
                return CONSOLE_STATUS_WAIT_NO_BLOCK;
            default:
                break;
            }
        }
        else if (Char == UNICODE_CARRIAGERETURN)
        {
            ULONG i, lStringLength;
            DWORD LineCount = 1;
            Index = Popup->CurrentCommand;
            LOG_IF_FAILED(EndPopup(pCookedReadData->_pScreenInfo, pCommandHistory));
            SetCurrentCommandLine(pCookedReadData, Index);
            lStringLength = pCookedReadData->_BytesRead;
            ProcessCookedReadInput(pCookedReadData, UNICODE_CARRIAGERETURN, 0, &Status);
            // complete read
            if (pCookedReadData->_Echo)
            {
                // check for alias
                i = pCookedReadData->_BufferSize;
                if (NT_SUCCESS(MatchAndCopyAlias(pCookedReadData->_BackupLimit,
                                                 (USHORT)lStringLength,
                                                 pCookedReadData->_BackupLimit,
                                                 (PUSHORT)& i,
                                                 pCookedReadData->ExeName,
                                                 pCookedReadData->ExeNameLength,
                                                 &LineCount)))
                {
                    pCookedReadData->_BytesRead = i;
                }
            }

            Status = STATUS_SUCCESS;
            DWORD dwNumBytes;
            if (pCookedReadData->_BytesRead > pCookedReadData->_UserBufferSize || LineCount > 1)
            {
                if (LineCount > 1)
                {
                    PWSTR Tmp;
                    SetFlag(pInputReadHandleData->InputHandleFlags, INPUT_READ_HANDLE_DATA::HandleFlags::MultiLineInput);
                    for (Tmp = pCookedReadData->_BackupLimit; *Tmp != UNICODE_LINEFEED; Tmp++)
                    {
                        ASSERT(Tmp < (pCookedReadData->_BackupLimit + pCookedReadData->_BytesRead));
                    }
                    dwNumBytes = (ULONG)(Tmp - pCookedReadData->_BackupLimit + 1) * sizeof(*Tmp);
                }
                else
                {
                    dwNumBytes = pCookedReadData->_UserBufferSize;
                }
                SetFlag(pInputReadHandleData->InputHandleFlags, INPUT_READ_HANDLE_DATA::HandleFlags::InputPending);
                pInputReadHandleData->BufPtr = pCookedReadData->_BackupLimit;
                pInputReadHandleData->BytesAvailable = pCookedReadData->_BytesRead - dwNumBytes;
                pInputReadHandleData->CurrentBufPtr = (PWCHAR)((PBYTE)pCookedReadData->_BackupLimit + dwNumBytes);
                memmove(pCookedReadData->_UserBuffer, pCookedReadData->_BackupLimit, dwNumBytes);
            }
            else
            {
                dwNumBytes = pCookedReadData->_BytesRead;
                memmove(pCookedReadData->_UserBuffer, pCookedReadData->_BackupLimit, dwNumBytes);
            }

            if (!pCookedReadData->_fIsUnicode)
            {
                PCHAR TransBuffer;

                // If ansi, translate string.
                TransBuffer = (PCHAR) new BYTE[dwNumBytes];
                if (TransBuffer == nullptr)
                {
                    return STATUS_NO_MEMORY;
                }

                dwNumBytes = (ULONG)ConvertToOem(gci.CP,
                                                 pCookedReadData->_UserBuffer,
                                                 dwNumBytes / sizeof(WCHAR),
                                                 TransBuffer,
                                                 dwNumBytes);
                memmove(pCookedReadData->_UserBuffer, TransBuffer, dwNumBytes);
                delete[] TransBuffer;
            }

            *(pCookedReadData->pdwNumBytes) = dwNumBytes;

            return CONSOLE_STATUS_READ_COMPLETE;
        }
        else
        {
            Index = FindMatchingCommand(pCookedReadData->_CommandHistory, &Char, 1 * sizeof(WCHAR), Popup->CurrentCommand, FMCFL_JUST_LOOKING);
            if (Index != -1)
            {
                UpdateCommandListPopup((SHORT)(Index - Popup->CurrentCommand),
                                       &Popup->CurrentCommand,
                                       pCommandHistory,
                                       Popup,
                                       pCookedReadData->_pScreenInfo,
                                       UCLP_WRAP);
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
        bool commandLinePopupKeys = false;
        Status = GetChar(pInputBuffer,
                         &Char,
                         TRUE,
                         nullptr,
                         &commandLinePopupKeys,
                         nullptr);
        if (!NT_SUCCESS(Status))
        {
            if (Status != CONSOLE_STATUS_WAIT)
            {
                pCookedReadData->_BytesRead = 0;
            }

            return Status;
        }

        if (commandLinePopupKeys)
        {
            switch (Char)
            {
            case VK_ESCAPE:
                LOG_IF_FAILED(EndPopup(pCookedReadData->_pScreenInfo, pCookedReadData->_CommandHistory));
                return CONSOLE_STATUS_WAIT_NO_BLOCK;
            }
        }

        LOG_IF_FAILED(EndPopup(pCookedReadData->_pScreenInfo, pCookedReadData->_CommandHistory));

        int i;  // char index (not byte)
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
            CursorPosition = pCookedReadData->_pScreenInfo->TextInfo->GetCursor()->GetPosition();

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
                Status = WriteCharsLegacy(pCookedReadData->_pScreenInfo,
                                          pCookedReadData->_BackupLimit,
                                          pCookedReadData->_BackupLimit,
                                          pCookedReadData->_BackupLimit,
                                          &pCookedReadData->_BytesRead,
                                          &pCookedReadData->_NumberOfVisibleChars,
                                          pCookedReadData->_OriginalCursorPosition.X,
                                          WC_DESTRUCTIVE_BACKSPACE | WC_KEEP_CURSOR_VISIBLE | WC_ECHO,
                                          nullptr);
                ASSERT(NT_SUCCESS(Status));
            }

            // restore cursor position
            Status = pCookedReadData->_pScreenInfo->SetCursorPosition(CursorPosition, TRUE);
            ASSERT(NT_SUCCESS(Status));
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
        bool commandLinePopupKeys = false;
        Status = GetChar(pInputBuffer,
                         &Char,
                         true,
                         nullptr,
                         &commandLinePopupKeys,
                         nullptr);
        if (!NT_SUCCESS(Status))
        {
            if (Status != CONSOLE_STATUS_WAIT)
            {
                pCookedReadData->_BytesRead = 0;
            }
            return Status;
        }

        if (commandLinePopupKeys)
        {
            switch (Char)
            {
            case VK_ESCAPE:
                LOG_IF_FAILED(EndPopup(pCookedReadData->_pScreenInfo, pCookedReadData->_CommandHistory));
                return CONSOLE_STATUS_WAIT_NO_BLOCK;
            }
        }

        LOG_IF_FAILED(EndPopup(pCookedReadData->_pScreenInfo, pCookedReadData->_CommandHistory));

        // copy up to specified char
        PCOMMAND const LastCommand = GetLastCommand(pCookedReadData->_CommandHistory);
        if (LastCommand)
        {
            int i;

            // find specified char in last command
            for (i = pCookedReadData->_CurrentPosition + 1; i < (int)(LastCommand->CommandLength / sizeof(WCHAR)); i++)
            {
                if (LastCommand->Command[i] == Char)
                {
                    break;
                }
            }

            // If we found it, copy up to it.
            if (i < (int)(LastCommand->CommandLength / sizeof(WCHAR)) &&
                ((USHORT)(LastCommand->CommandLength / sizeof(WCHAR)) > ((USHORT)pCookedReadData->_CurrentPosition)))
            {
                int j = i - pCookedReadData->_CurrentPosition;
                ASSERT(j > 0);
                memmove(pCookedReadData->_BufPtr,
                        &LastCommand->Command[pCookedReadData->_CurrentPosition],
                        j * sizeof(WCHAR));
                pCookedReadData->_CurrentPosition += j;
                j *= sizeof(WCHAR);
                pCookedReadData->_BytesRead = std::max(pCookedReadData->_BytesRead,
                                                       gsl::narrow<ULONG>(pCookedReadData->_CurrentPosition * sizeof(WCHAR)));
                if (pCookedReadData->_Echo)
                {
                    DWORD NumSpaces;
                    SHORT ScrollY = 0;

                    Status = WriteCharsLegacy(pCookedReadData->_pScreenInfo,
                                              pCookedReadData->_BackupLimit,
                                              pCookedReadData->_BufPtr,
                                              pCookedReadData->_BufPtr,
                                              (PDWORD)&j,
                                              &NumSpaces,
                                              pCookedReadData->_OriginalCursorPosition.X,
                                              WC_DESTRUCTIVE_BACKSPACE | WC_KEEP_CURSOR_VISIBLE | WC_ECHO,
                                              &ScrollY);
                    ASSERT(NT_SUCCESS(Status));
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
    PCOMMAND_HISTORY const CommandHistory = pCookedReadData->_CommandHistory;
    PCLE_POPUP const Popup = CONTAINING_RECORD(CommandHistory->PopupList.Flink, CLE_POPUP, ListLink);
    NTSTATUS Status = STATUS_SUCCESS;
    InputBuffer* const pInputBuffer = pCookedReadData->GetInputBuffer();
    for (;;)
    {
        WCHAR Char;
        bool commandLinePopupKeys = false;

        Status = GetChar(pInputBuffer,
                         &Char,
                         TRUE,
                         nullptr,
                         &commandLinePopupKeys,
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
                DWORD CharsToWrite = sizeof(WCHAR);
                const TextAttribute realAttributes = pCookedReadData->_pScreenInfo->GetAttributes();
                pCookedReadData->_pScreenInfo->SetAttributes(Popup->Attributes);
                DWORD NumSpaces;
                Status = WriteCharsLegacy(pCookedReadData->_pScreenInfo,
                                          Popup->NumberBuffer,
                                          &Popup->NumberBuffer[Popup->NumberRead],
                                          &Char,
                                          &CharsToWrite,
                                          &NumSpaces,
                                          pCookedReadData->_OriginalCursorPosition.X,
                                          WC_DESTRUCTIVE_BACKSPACE | WC_KEEP_CURSOR_VISIBLE | WC_ECHO,
                                          nullptr);
                ASSERT(NT_SUCCESS(Status));
                pCookedReadData->_pScreenInfo->SetAttributes(realAttributes);
                Popup->NumberBuffer[Popup->NumberRead] = Char;
                Popup->NumberRead += 1;
            }
        }
        else if (Char == UNICODE_BACKSPACE)
        {
            if (Popup->NumberRead > 0)
            {
                DWORD CharsToWrite = sizeof(WCHAR);
                const TextAttribute realAttributes = pCookedReadData->_pScreenInfo->GetAttributes();
                pCookedReadData->_pScreenInfo->SetAttributes(Popup->Attributes);
                DWORD NumSpaces;
                Status = WriteCharsLegacy(pCookedReadData->_pScreenInfo,
                                          Popup->NumberBuffer,
                                          &Popup->NumberBuffer[Popup->NumberRead],
                                          &Char,
                                          &CharsToWrite,
                                          &NumSpaces,
                                          pCookedReadData->_OriginalCursorPosition.X,
                                          WC_DESTRUCTIVE_BACKSPACE | WC_KEEP_CURSOR_VISIBLE | WC_ECHO,
                                          nullptr);

                ASSERT(NT_SUCCESS(Status));
                pCookedReadData->_pScreenInfo->SetAttributes(realAttributes);
                Popup->NumberBuffer[Popup->NumberRead] = (WCHAR)' ';
                Popup->NumberRead -= 1;
            }
        }
        else if (Char == (WCHAR)VK_ESCAPE)
        {
            LOG_IF_FAILED(EndPopup(pCookedReadData->_pScreenInfo, pCookedReadData->_CommandHistory));
            if (!CLE_NO_POPUPS(CommandHistory))
            {
                LOG_IF_FAILED(EndPopup(pCookedReadData->_pScreenInfo, pCookedReadData->_CommandHistory));
            }

            // Note that CookedReadData's OriginalCursorPosition is the position before ANY text was entered on the edit line.
            // We want to use the position before the cursor was moved for this popup handler specifically, which may be *anywhere* in the edit line
            // and will be synchronized with the pointers in the CookedReadData structure (BufPtr, etc.)
            LOG_IF_FAILED(pCookedReadData->_pScreenInfo->SetCursorPosition(pCookedReadData->BeforeDialogCursorPosition, TRUE));
        }
        else if (Char == UNICODE_CARRIAGERETURN)
        {
            CHAR NumberBuffer[6];
            int i;

            // This is guaranteed above.
            __analysis_assume(Popup->NumberRead < 6);
            for (i = 0; i < Popup->NumberRead; i++)
            {
                ASSERT(i < ARRAYSIZE(NumberBuffer));
                NumberBuffer[i] = (CHAR)Popup->NumberBuffer[i];
            }
            NumberBuffer[i] = 0;

            SHORT CommandNumber = (SHORT)atoi(NumberBuffer);
            if ((WORD)CommandNumber >= (WORD)pCookedReadData->_CommandHistory->NumberOfCommands)
            {
                CommandNumber = (SHORT)(pCookedReadData->_CommandHistory->NumberOfCommands - 1);
            }

            LOG_IF_FAILED(EndPopup(pCookedReadData->_pScreenInfo, pCookedReadData->_CommandHistory));
            if (!CLE_NO_POPUPS(CommandHistory))
            {
                LOG_IF_FAILED(EndPopup(pCookedReadData->_pScreenInfo, pCookedReadData->_CommandHistory));
            }
            SetCurrentCommandLine(pCookedReadData, COMMAND_NUM_TO_INDEX(CommandNumber, pCookedReadData->_CommandHistory));
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
    PCOMMAND_HISTORY const CommandHistory = CookedReadData->_CommandHistory;
    PCLE_POPUP const Popup = CONTAINING_RECORD(CommandHistory->PopupList.Flink, CLE_POPUP, ListLink);

    SHORT const CurrentCommand = COMMAND_INDEX_TO_NUM(CommandHistory->LastDisplayed, CommandHistory);

    if (CurrentCommand < (SHORT)(CommandHistory->NumberOfCommands - POPUP_SIZE_Y(Popup)))
    {
        Popup->BottomIndex = std::max(CurrentCommand, gsl::narrow<SHORT>(POPUP_SIZE_Y(Popup) - 1));
    }
    else
    {
        Popup->BottomIndex = (SHORT)(CommandHistory->NumberOfCommands - 1);
    }
    Popup->CurrentCommand = CommandHistory->LastDisplayed;
    DrawCommandListPopup(Popup, CommandHistory->LastDisplayed, CommandHistory, CookedReadData->_pScreenInfo);
    Popup->PopupInputRoutine = (PCLE_POPUP_INPUT_ROUTINE)ProcessCommandListInput;
    return ProcessCommandListInput(CookedReadData);
}

VOID DrawPromptPopup(_In_ PCLE_POPUP Popup, _In_ PSCREEN_INFORMATION ScreenInfo, _In_reads_(PromptLength) PWCHAR Prompt, _In_ ULONG PromptLength)
{
    // Draw empty popup.
    COORD WriteCoord;
    WriteCoord.X = (SHORT)(Popup->Region.Left + 1);
    WriteCoord.Y = (SHORT)(Popup->Region.Top + 1);
    ULONG lStringLength = POPUP_SIZE_X(Popup);
    for (SHORT i = 0; i < POPUP_SIZE_Y(Popup); i++)
    {
        LOG_IF_FAILED(FillOutput(ScreenInfo,
                                 Popup->Attributes.GetLegacyAttributes(),
                                 WriteCoord,
                                 CONSOLE_ATTRIBUTE,
                                 &lStringLength));
        LOG_IF_FAILED(FillOutput(ScreenInfo,
                                 (WCHAR)' ',
                                 WriteCoord,
                                 CONSOLE_FALSE_UNICODE,   // faster that real unicode
                                 &lStringLength));

        WriteCoord.Y += 1;
    }

    WriteCoord.X = (SHORT)(Popup->Region.Left + 1);
    WriteCoord.Y = (SHORT)(Popup->Region.Top + 1);

    // write prompt to screen
    lStringLength = PromptLength;
    if (lStringLength > (ULONG)POPUP_SIZE_X(Popup))
    {
        lStringLength = (ULONG)(POPUP_SIZE_X(Popup));
    }

    LOG_IF_FAILED(WriteOutputString(ScreenInfo, Prompt, WriteCoord, CONSOLE_REAL_UNICODE, &lStringLength, nullptr));
}

// Routine Description:
// - This routine handles the "delete up to this char" popup.  It puts up the popup, then calls ProcessCopyFromCharInput to get and process input.
// Return Value:
// - CONSOLE_STATUS_WAIT - we ran out of input, so a wait block was created
// - STATUS_SUCCESS - read was fully completed (user hit return)
[[nodiscard]]
NTSTATUS CopyFromCharPopup(_In_ COOKED_READ_DATA* CookedReadData)
{
    const CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    WCHAR ItemString[70];
    int ItemLength = 0;
    LANGID LangId;
    NTSTATUS Status = GetConsoleLangId(gci.OutputCP, &LangId);
    if (NT_SUCCESS(Status))
    {
        ItemLength = LoadStringEx(ServiceLocator::LocateGlobals().hInstance, ID_CONSOLE_MSGCMDLINEF4, ItemString, ARRAYSIZE(ItemString), LangId);
    }

    if (!NT_SUCCESS(Status) || ItemLength == 0)
    {
        ItemLength = LoadStringW(ServiceLocator::LocateGlobals().hInstance, ID_CONSOLE_MSGCMDLINEF4, ItemString, ARRAYSIZE(ItemString));
    }

    PCOMMAND_HISTORY const CommandHistory = CookedReadData->_CommandHistory;
    PCLE_POPUP const Popup = CONTAINING_RECORD(CommandHistory->PopupList.Flink, CLE_POPUP, ListLink);

    DrawPromptPopup(Popup, CookedReadData->_pScreenInfo, ItemString, ItemLength);
    Popup->PopupInputRoutine = (PCLE_POPUP_INPUT_ROUTINE)ProcessCopyFromCharInput;

    return ProcessCopyFromCharInput(CookedReadData);
}

// Routine Description:
// - This routine handles the "copy up to this char" popup.  It puts up the popup, then calls ProcessCopyToCharInput to get and process input.
// Return Value:
// - CONSOLE_STATUS_WAIT - we ran out of input, so a wait block was created
// - STATUS_SUCCESS - read was fully completed (user hit return)
[[nodiscard]]
NTSTATUS CopyToCharPopup(_In_ COOKED_READ_DATA* CookedReadData)
{
    const CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    WCHAR ItemString[70];
    int ItemLength = 0;
    LANGID LangId;

    NTSTATUS Status = GetConsoleLangId(gci.OutputCP, &LangId);
    if (NT_SUCCESS(Status))
    {
        ItemLength = LoadStringEx(ServiceLocator::LocateGlobals().hInstance, ID_CONSOLE_MSGCMDLINEF2, ItemString, ARRAYSIZE(ItemString), LangId);
    }

    if (!NT_SUCCESS(Status) || ItemLength == 0)
    {
        ItemLength = LoadStringW(ServiceLocator::LocateGlobals().hInstance, ID_CONSOLE_MSGCMDLINEF2, ItemString, ARRAYSIZE(ItemString));
    }

    PCOMMAND_HISTORY const CommandHistory = CookedReadData->_CommandHistory;
    PCLE_POPUP const Popup = CONTAINING_RECORD(CommandHistory->PopupList.Flink, CLE_POPUP, ListLink);
    DrawPromptPopup(Popup, CookedReadData->_pScreenInfo, ItemString, ItemLength);
    Popup->PopupInputRoutine = (PCLE_POPUP_INPUT_ROUTINE)ProcessCopyToCharInput;
    return ProcessCopyToCharInput(CookedReadData);
}

// Routine Description:
// - This routine handles the "enter command number" popup.  It puts up the popup, then calls ProcessCommandNumberInput to get and process input.
// Return Value:
// - CONSOLE_STATUS_WAIT - we ran out of input, so a wait block was created
// - STATUS_SUCCESS - read was fully completed (user hit return)
[[nodiscard]]
NTSTATUS CommandNumberPopup(_In_ COOKED_READ_DATA* const CookedReadData)
{
    const CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    WCHAR ItemString[70];
    int ItemLength = 0;
    LANGID LangId;

    NTSTATUS Status = GetConsoleLangId(gci.OutputCP, &LangId);
    if (NT_SUCCESS(Status))
    {
        ItemLength = LoadStringEx(ServiceLocator::LocateGlobals().hInstance, ID_CONSOLE_MSGCMDLINEF9, ItemString, ARRAYSIZE(ItemString), LangId);
    }
    if (!NT_SUCCESS(Status) || ItemLength == 0)
    {
        ItemLength = LoadStringW(ServiceLocator::LocateGlobals().hInstance, ID_CONSOLE_MSGCMDLINEF9, ItemString, ARRAYSIZE(ItemString));
    }

    PCOMMAND_HISTORY const CommandHistory = CookedReadData->_CommandHistory;
    PCLE_POPUP const Popup = CONTAINING_RECORD(CommandHistory->PopupList.Flink, CLE_POPUP, ListLink);

    if (ItemLength > POPUP_SIZE_X(Popup) - COMMAND_NUMBER_LENGTH)
    {
        ItemLength = POPUP_SIZE_X(Popup) - COMMAND_NUMBER_LENGTH;
    }
    DrawPromptPopup(Popup, CookedReadData->_pScreenInfo, ItemString, ItemLength);

    // Save the original cursor position in case the user cancels out of the dialog
    CookedReadData->BeforeDialogCursorPosition = CookedReadData->_pScreenInfo->TextInfo->GetCursor()->GetPosition();

    // Move the cursor into the dialog so the user can type multiple characters for the command number
    COORD CursorPosition;
    CursorPosition.X = (SHORT)(Popup->Region.Right - MINIMUM_COMMAND_PROMPT_SIZE);
    CursorPosition.Y = (SHORT)(Popup->Region.Top + 1);
    LOG_IF_FAILED(CookedReadData->_pScreenInfo->SetCursorPosition(CursorPosition, TRUE));

    // Prepare the popup
    Popup->NumberRead = 0;
    Popup->PopupInputRoutine = (PCLE_POPUP_INPUT_ROUTINE)ProcessCommandNumberInput;

    // Transfer control to the handler routine
    return ProcessCommandNumberInput(CookedReadData);
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
                            _In_ const DWORD dwKeyState)
{
    const CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    COORD CurrentPosition = { 0 };
    DWORD CharsToWrite;
    NTSTATUS Status;
    SHORT ScrollY = 0;
    const SHORT sScreenBufferSizeX = pCookedReadData->_pScreenInfo->GetScreenBufferSize().X;

    BOOL UpdateCursorPosition = FALSE;
    if (wch == VK_F7 && (dwKeyState & (RIGHT_CTRL_PRESSED | LEFT_CTRL_PRESSED | RIGHT_ALT_PRESSED | LEFT_ALT_PRESSED)) == 0)
    {
        COORD PopupSize;

        if (pCookedReadData->_CommandHistory && pCookedReadData->_CommandHistory->NumberOfCommands)
        {
            PopupSize.X = 40;
            PopupSize.Y = 10;
            Status = BeginPopup(pCookedReadData->_pScreenInfo, pCookedReadData->_CommandHistory, PopupSize);
            if (NT_SUCCESS(Status))
            {
                // CommandListPopup does EndPopup call
                return CommandListPopup(pCookedReadData);
            }
        }
    }
    else
    {
        switch (wch)
        {
        case VK_ESCAPE:
            DeleteCommandLine(pCookedReadData, TRUE);
            break;
        case VK_UP:
        case VK_DOWN:
        case VK_F5:
            if (wch == VK_F5)
                wch = VK_UP;
            // for doskey compatibility, buffer isn't circular
            if (wch == VK_UP && !AtFirstCommand(pCookedReadData->_CommandHistory) || wch == VK_DOWN && !AtLastCommand(pCookedReadData->_CommandHistory))
            {
                DeleteCommandLine(pCookedReadData, TRUE);
                Status = RetrieveCommand(pCookedReadData->_CommandHistory,
                                         wch,
                                         pCookedReadData->_BackupLimit,
                                         pCookedReadData->_BufferSize,
                                         &pCookedReadData->_BytesRead);
                ASSERT(pCookedReadData->_BackupLimit == pCookedReadData->_BufPtr);
                if (pCookedReadData->_Echo)
                {
                    Status = WriteCharsLegacy(pCookedReadData->_pScreenInfo,
                                              pCookedReadData->_BackupLimit,
                                              pCookedReadData->_BufPtr,
                                              pCookedReadData->_BufPtr,
                                              &pCookedReadData->_BytesRead,
                                              &pCookedReadData->_NumberOfVisibleChars,
                                              pCookedReadData->_OriginalCursorPosition.X,
                                              WC_DESTRUCTIVE_BACKSPACE | WC_KEEP_CURSOR_VISIBLE | WC_ECHO,
                                              &ScrollY);
                    ASSERT(NT_SUCCESS(Status));
                    pCookedReadData->_OriginalCursorPosition.Y += ScrollY;
                }
                CharsToWrite = pCookedReadData->_BytesRead / sizeof(WCHAR);
                pCookedReadData->_CurrentPosition = CharsToWrite;
                pCookedReadData->_BufPtr = pCookedReadData->_BackupLimit + CharsToWrite;
            }
            break;
        case VK_PRIOR:
        case VK_NEXT:
            if (pCookedReadData->_CommandHistory && pCookedReadData->_CommandHistory->NumberOfCommands)
            {
                // display oldest or newest command
                SHORT CommandNumber;
                if (wch == VK_PRIOR)
                {
                    CommandNumber = 0;
                }
                else
                {
                    CommandNumber = (SHORT)(pCookedReadData->_CommandHistory->NumberOfCommands - 1);
                }
                DeleteCommandLine(pCookedReadData, TRUE);
                Status = RetrieveNthCommand(pCookedReadData->_CommandHistory,
                                            COMMAND_NUM_TO_INDEX(CommandNumber, pCookedReadData->_CommandHistory),
                                            pCookedReadData->_BackupLimit,
                                            pCookedReadData->_BufferSize,
                                            &pCookedReadData->_BytesRead);
                ASSERT(pCookedReadData->_BackupLimit == pCookedReadData->_BufPtr);
                if (pCookedReadData->_Echo)
                {
                    Status = WriteCharsLegacy(pCookedReadData->_pScreenInfo,
                                              pCookedReadData->_BackupLimit,
                                              pCookedReadData->_BufPtr,
                                              pCookedReadData->_BufPtr,
                                              &pCookedReadData->_BytesRead,
                                              &pCookedReadData->_NumberOfVisibleChars,
                                              pCookedReadData->_OriginalCursorPosition.X,
                                              WC_DESTRUCTIVE_BACKSPACE | WC_KEEP_CURSOR_VISIBLE | WC_ECHO,
                                              &ScrollY);
                    ASSERT(NT_SUCCESS(Status));
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
                DeleteCommandLine(pCookedReadData, FALSE);
                pCookedReadData->_BytesRead = pCookedReadData->_CurrentPosition * sizeof(WCHAR);
                if (pCookedReadData->_Echo)
                {
                    Status = WriteCharsLegacy(pCookedReadData->_pScreenInfo,
                                              pCookedReadData->_BackupLimit,
                                              pCookedReadData->_BackupLimit,
                                              pCookedReadData->_BackupLimit,
                                              &pCookedReadData->_BytesRead,
                                              &pCookedReadData->_NumberOfVisibleChars,
                                              pCookedReadData->_OriginalCursorPosition.X,
                                              WC_DESTRUCTIVE_BACKSPACE | WC_KEEP_CURSOR_VISIBLE | WC_ECHO,
                                              nullptr);
                    ASSERT(NT_SUCCESS(Status));
                }
            }
            else
            {
                pCookedReadData->_CurrentPosition = pCookedReadData->_BytesRead / sizeof(WCHAR);
                pCookedReadData->_BufPtr = pCookedReadData->_BackupLimit + pCookedReadData->_CurrentPosition;
                CurrentPosition.X = (SHORT)(pCookedReadData->_OriginalCursorPosition.X + pCookedReadData->_NumberOfVisibleChars);
                CurrentPosition.Y = pCookedReadData->_OriginalCursorPosition.Y;
                if (CheckBisectProcessW(pCookedReadData->_pScreenInfo,
                                        pCookedReadData->_BackupLimit,
                                        pCookedReadData->_CurrentPosition,
                                        sScreenBufferSizeX - pCookedReadData->_OriginalCursorPosition.X,
                                        pCookedReadData->_OriginalCursorPosition.X,
                                        TRUE))
                {
                    CurrentPosition.X++;
                }
                UpdateCursorPosition = TRUE;
            }
            break;
        case VK_HOME:
            if (dwKeyState & (RIGHT_CTRL_PRESSED | LEFT_CTRL_PRESSED))
            {
                DeleteCommandLine(pCookedReadData, FALSE);
                pCookedReadData->_BytesRead -= pCookedReadData->_CurrentPosition * sizeof(WCHAR);
                pCookedReadData->_CurrentPosition = 0;
                memmove(pCookedReadData->_BackupLimit, pCookedReadData->_BufPtr, pCookedReadData->_BytesRead);
                pCookedReadData->_BufPtr = pCookedReadData->_BackupLimit;
                if (pCookedReadData->_Echo)
                {
                    Status = WriteCharsLegacy(pCookedReadData->_pScreenInfo,
                                              pCookedReadData->_BackupLimit,
                                              pCookedReadData->_BackupLimit,
                                              pCookedReadData->_BackupLimit,
                                              &pCookedReadData->_BytesRead,
                                              &pCookedReadData->_NumberOfVisibleChars,
                                              pCookedReadData->_OriginalCursorPosition.X,
                                              WC_DESTRUCTIVE_BACKSPACE | WC_KEEP_CURSOR_VISIBLE | WC_ECHO,
                                              nullptr);
                    ASSERT(NT_SUCCESS(Status));
                }
                CurrentPosition = pCookedReadData->_OriginalCursorPosition;
                UpdateCursorPosition = TRUE;
            }
            else
            {
                pCookedReadData->_CurrentPosition = 0;
                pCookedReadData->_BufPtr = pCookedReadData->_BackupLimit;
                CurrentPosition = pCookedReadData->_OriginalCursorPosition;
                UpdateCursorPosition = TRUE;
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
                                ASSERT(LastWord > pCookedReadData->_BackupLimit);
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
                                    ASSERT(LastWord > pCookedReadData->_BackupLimit);
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
                                    ASSERT(LastWord > pCookedReadData->_BackupLimit);
                                    if (IsWordDelim(*LastWord))
                                    {
                                        break;
                                    }
                                }
                            }
                        }
                        ASSERT(LastWord >= pCookedReadData->_BackupLimit);
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

                    UpdateCursorPosition = TRUE;
                }
            }
            else
            {
                if (pCookedReadData->_BufPtr != pCookedReadData->_BackupLimit)
                {
                    pCookedReadData->_BufPtr--;
                    pCookedReadData->_CurrentPosition--;
                    CurrentPosition.X = pCookedReadData->_pScreenInfo->TextInfo->GetCursor()->GetPosition().X;
                    CurrentPosition.Y = pCookedReadData->_pScreenInfo->TextInfo->GetCursor()->GetPosition().Y;
                    CurrentPosition.X = (SHORT)(CurrentPosition.X -
                                                RetrieveNumberOfSpaces(pCookedReadData->_OriginalCursorPosition.X,
                                                                       pCookedReadData->_BackupLimit,
                                                                       pCookedReadData->_CurrentPosition));
                    if (CheckBisectProcessW(pCookedReadData->_pScreenInfo,
                                            pCookedReadData->_BackupLimit,
                                            pCookedReadData->_CurrentPosition + 2,
                                            sScreenBufferSizeX - pCookedReadData->_OriginalCursorPosition.X,
                                            pCookedReadData->_OriginalCursorPosition.X,
                                            TRUE))
                    {
                        if ((CurrentPosition.X == -2) || (CurrentPosition.X == -1))
                        {
                            CurrentPosition.X--;
                        }
                    }

                    UpdateCursorPosition = TRUE;
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

                        ASSERT(NextWord < BufLast);
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
                        UpdateCursorPosition = TRUE;
                    }
                }
            }
            else
            {
                // If not at the end of the line, move cursor position right.
                if (pCookedReadData->_CurrentPosition < (pCookedReadData->_BytesRead / sizeof(WCHAR)))
                {
                    CurrentPosition = pCookedReadData->_pScreenInfo->TextInfo->GetCursor()->GetPosition();
                    CurrentPosition.X = (SHORT)(CurrentPosition.X +
                                                RetrieveNumberOfSpaces(pCookedReadData->_OriginalCursorPosition.X,
                                                                       pCookedReadData->_BackupLimit,
                                                                       pCookedReadData->_CurrentPosition));
                    if (CheckBisectProcessW(pCookedReadData->_pScreenInfo,
                                            pCookedReadData->_BackupLimit,
                                            pCookedReadData->_CurrentPosition + 2,
                                            sScreenBufferSizeX - pCookedReadData->_OriginalCursorPosition.X,
                                            pCookedReadData->_OriginalCursorPosition.X,
                                            TRUE))
                    {
                        if (CurrentPosition.X == (sScreenBufferSizeX - 1))
                            CurrentPosition.X++;
                    }

                    pCookedReadData->_BufPtr++;
                    pCookedReadData->_CurrentPosition++;
                    UpdateCursorPosition = TRUE;

                    // if at the end of the line, copy a character from the same position in the last command
                }
                else if (pCookedReadData->_CommandHistory)
                {
                    PCOMMAND LastCommand;
                    DWORD NumSpaces;
                    LastCommand = GetLastCommand(pCookedReadData->_CommandHistory);
                    if (LastCommand && (USHORT)(LastCommand->CommandLength / sizeof(WCHAR)) > (USHORT)pCookedReadData->_CurrentPosition)
                    {
                        *pCookedReadData->_BufPtr = LastCommand->Command[pCookedReadData->_CurrentPosition];
                        pCookedReadData->_BytesRead += sizeof(WCHAR);
                        pCookedReadData->_CurrentPosition++;
                        if (pCookedReadData->_Echo)
                        {
                            CharsToWrite = sizeof(WCHAR);
                            Status = WriteCharsLegacy(pCookedReadData->_pScreenInfo,
                                                      pCookedReadData->_BackupLimit,
                                                      pCookedReadData->_BufPtr,
                                                      pCookedReadData->_BufPtr,
                                                      &CharsToWrite,
                                                      &NumSpaces,
                                                      pCookedReadData->_OriginalCursorPosition.X,
                                                      WC_DESTRUCTIVE_BACKSPACE | WC_KEEP_CURSOR_VISIBLE | WC_ECHO,
                                                      &ScrollY);
                            ASSERT(NT_SUCCESS(Status));
                            pCookedReadData->_OriginalCursorPosition.Y += ScrollY;
                            pCookedReadData->_NumberOfVisibleChars += NumSpaces;
                        }
                        pCookedReadData->_BufPtr += 1;
                    }
                }
            }
            break;

        case VK_F2:
            // copy the previous command to the current command, up to but
            // not including the character specified by the user.  the user
            // is prompted via popup to enter a character.
            if (pCookedReadData->_CommandHistory)
            {
                COORD PopupSize;

                PopupSize.X = COPY_TO_CHAR_PROMPT_LENGTH + 2;
                PopupSize.Y = 1;
                Status = BeginPopup(pCookedReadData->_pScreenInfo, pCookedReadData->_CommandHistory, PopupSize);
                if (NT_SUCCESS(Status))
                {
                    // CopyToCharPopup does EndPopup call
                    return CopyToCharPopup(pCookedReadData);
                }
            }
            break;

        case VK_F3:
            // Copy the remainder of the previous command to the current command.
            if (pCookedReadData->_CommandHistory)
            {
                PCOMMAND LastCommand;
                DWORD NumSpaces, cchCount;

                LastCommand = GetLastCommand(pCookedReadData->_CommandHistory);
                if (LastCommand && (USHORT)(LastCommand->CommandLength / sizeof(WCHAR)) > (USHORT)pCookedReadData->_CurrentPosition)
                {
                    cchCount = (LastCommand->CommandLength / sizeof(WCHAR)) - pCookedReadData->_CurrentPosition;

#pragma prefast(suppress:__WARNING_POTENTIAL_BUFFER_OVERFLOW_HIGH_PRIORITY, "This is fine")
                    memmove(pCookedReadData->_BufPtr, &LastCommand->Command[pCookedReadData->_CurrentPosition], cchCount * sizeof(WCHAR));
                    pCookedReadData->_CurrentPosition += cchCount;
                    cchCount *= sizeof(WCHAR);
                    pCookedReadData->_BytesRead = std::max(static_cast<ULONG>(LastCommand->CommandLength), pCookedReadData->_BytesRead);
                    if (pCookedReadData->_Echo)
                    {
                        Status = WriteCharsLegacy(pCookedReadData->_pScreenInfo,
                                                  pCookedReadData->_BackupLimit,
                                                  pCookedReadData->_BufPtr,
                                                  pCookedReadData->_BufPtr,
                                                  &cchCount,
                                                  (PULONG)&NumSpaces,
                                                  pCookedReadData->_OriginalCursorPosition.X,
                                                  WC_DESTRUCTIVE_BACKSPACE | WC_KEEP_CURSOR_VISIBLE | WC_ECHO,
                                                  &ScrollY);
                        ASSERT(NT_SUCCESS(Status));
                        pCookedReadData->_OriginalCursorPosition.Y += ScrollY;
                        pCookedReadData->_NumberOfVisibleChars += NumSpaces;
                    }
                    pCookedReadData->_BufPtr += cchCount / sizeof(WCHAR);
                }

            }
            break;

        case VK_F4:
            // Delete the current command from cursor position to the
            // letter specified by the user. The user is prompted via
            // popup to enter a character.
            if (pCookedReadData->_CommandHistory)
            {
                COORD PopupSize;

                PopupSize.X = COPY_FROM_CHAR_PROMPT_LENGTH + 2;
                PopupSize.Y = 1;
                Status = BeginPopup(pCookedReadData->_pScreenInfo, pCookedReadData->_CommandHistory, PopupSize);
                if (NT_SUCCESS(Status))
                {
                    // CopyFromCharPopup does EndPopup call
                    return CopyFromCharPopup(pCookedReadData);
                }
            }
            break;
        case VK_F6:
        {
            // place a ctrl-z in the current command line
            DWORD NumSpaces = 0;

            *pCookedReadData->_BufPtr = (WCHAR)0x1a;  // ctrl-z
            pCookedReadData->_BytesRead += sizeof(WCHAR);
            pCookedReadData->_CurrentPosition++;
            if (pCookedReadData->_Echo)
            {
                CharsToWrite = sizeof(WCHAR);
                Status = WriteCharsLegacy(pCookedReadData->_pScreenInfo,
                                          pCookedReadData->_BackupLimit,
                                          pCookedReadData->_BufPtr,
                                          pCookedReadData->_BufPtr,
                                          &CharsToWrite,
                                          &NumSpaces,
                                          pCookedReadData->_OriginalCursorPosition.X,
                                          WC_DESTRUCTIVE_BACKSPACE | WC_KEEP_CURSOR_VISIBLE | WC_ECHO,
                                          &ScrollY);
                ASSERT(NT_SUCCESS(Status));
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
                    EmptyCommandHistory(pCookedReadData->_CommandHistory);
                    pCookedReadData->_CommandHistory->Flags |= CLE_ALLOCATED;
                }
            }
            break;

        case VK_F8:
            if (pCookedReadData->_CommandHistory)
            {
                SHORT i;

                // Cycles through the stored commands that start with the characters in the current command.
                i = FindMatchingCommand(pCookedReadData->_CommandHistory,
                                        pCookedReadData->_BackupLimit,
                                        pCookedReadData->_CurrentPosition * sizeof(WCHAR),
                                        pCookedReadData->_CommandHistory->LastDisplayed,
                                        0);
                if (i != -1)
                {
                    SHORT CurrentPos;
                    COORD CursorPosition;

                    // save cursor position
                    CurrentPos = (SHORT)pCookedReadData->_CurrentPosition;
                    CursorPosition = pCookedReadData->_pScreenInfo->TextInfo->GetCursor()->GetPosition();

                    DeleteCommandLine(pCookedReadData, TRUE);
                    Status = RetrieveNthCommand(pCookedReadData->_CommandHistory,
                                                i,
                                                pCookedReadData->_BackupLimit,
                                                pCookedReadData->_BufferSize,
                                                &pCookedReadData->_BytesRead);
                    ASSERT(pCookedReadData->_BackupLimit == pCookedReadData->_BufPtr);
                    if (pCookedReadData->_Echo)
                    {
                        Status = WriteCharsLegacy(pCookedReadData->_pScreenInfo,
                                                  pCookedReadData->_BackupLimit,
                                                  pCookedReadData->_BufPtr,
                                                  pCookedReadData->_BufPtr,
                                                  &pCookedReadData->_BytesRead,
                                                  &pCookedReadData->_NumberOfVisibleChars,
                                                  pCookedReadData->_OriginalCursorPosition.X,
                                                  WC_DESTRUCTIVE_BACKSPACE | WC_KEEP_CURSOR_VISIBLE | WC_ECHO,
                                                  &ScrollY);
                        ASSERT(NT_SUCCESS(Status));
                        pCookedReadData->_OriginalCursorPosition.Y += ScrollY;
                    }
                    CursorPosition.Y += ScrollY;

                    // restore cursor position
                    pCookedReadData->_BufPtr = pCookedReadData->_BackupLimit + CurrentPos;
                    pCookedReadData->_CurrentPosition = CurrentPos;
                    Status = pCookedReadData->_pScreenInfo->SetCursorPosition(CursorPosition, TRUE);
                    ASSERT(NT_SUCCESS(Status));
                }
            }
            break;
        case VK_F9:
        {
            // prompt the user to enter the desired command number. copy that command to the command line.
            COORD PopupSize;

            if (pCookedReadData->_CommandHistory &&
                pCookedReadData->_CommandHistory->NumberOfCommands &&
                sScreenBufferSizeX >= MINIMUM_COMMAND_PROMPT_SIZE + 2)
            {   // 2 is for border
                PopupSize.X = COMMAND_NUMBER_PROMPT_LENGTH + COMMAND_NUMBER_LENGTH;
                PopupSize.Y = 1;
                Status = BeginPopup(pCookedReadData->_pScreenInfo, pCookedReadData->_CommandHistory, PopupSize);
                if (NT_SUCCESS(Status))
                {
                    // CommandNumberPopup does EndPopup call
                    return CommandNumberPopup(pCookedReadData);
                }
            }
            break;
        }
        case VK_F10:
            // Alt+F10 clears the aliases for specifically cmd.exe.
            if (dwKeyState & (RIGHT_ALT_PRESSED | LEFT_ALT_PRESSED))
            {
                ClearCmdExeAliases();
            }
            break;
        case VK_INSERT:
            pCookedReadData->_InsertMode = !pCookedReadData->_InsertMode;
            pCookedReadData->_pScreenInfo->SetCursorDBMode((!!pCookedReadData->_InsertMode != gci.GetInsertMode()));
            break;
        case VK_DELETE:
            if (!AT_EOL(pCookedReadData))
            {
                COORD CursorPosition;

                BOOL fStartFromDelim = IsWordDelim(*pCookedReadData->_BufPtr);

            del_repeat:
                // save cursor position
                CursorPosition = pCookedReadData->_pScreenInfo->TextInfo->GetCursor()->GetPosition();

                // Delete commandline.
#pragma prefast(suppress:__WARNING_BUFFER_OVERFLOW, "Not sure why prefast is getting confused here")
                DeleteCommandLine(pCookedReadData, FALSE);

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
                    Status = WriteCharsLegacy(pCookedReadData->_pScreenInfo,
                                              pCookedReadData->_BackupLimit,
                                              pCookedReadData->_BackupLimit,
                                              pCookedReadData->_BackupLimit,
                                              &pCookedReadData->_BytesRead,
                                              &pCookedReadData->_NumberOfVisibleChars,
                                              pCookedReadData->_OriginalCursorPosition.X,
                                              WC_DESTRUCTIVE_BACKSPACE | WC_KEEP_CURSOR_VISIBLE | WC_ECHO,
                                              nullptr);
                    ASSERT(NT_SUCCESS(Status));
                }

                // restore cursor position
                if (CheckBisectProcessW(pCookedReadData->_pScreenInfo,
                                        pCookedReadData->_BackupLimit,
                                        pCookedReadData->_CurrentPosition + 1,
                                        sScreenBufferSizeX - pCookedReadData->_OriginalCursorPosition.X,
                                        pCookedReadData->_OriginalCursorPosition.X,
                                        TRUE))
                {
                    CursorPosition.X++;
                }
                CurrentPosition = CursorPosition;
                if (pCookedReadData->_Echo)
                {
                    Status = AdjustCursorPosition(pCookedReadData->_pScreenInfo, CurrentPosition, TRUE, nullptr);
                    ASSERT(NT_SUCCESS(Status));
                }

                // If Ctrl key is pressed, delete a word.
                // If the start point was word delimiter, just remove delimiters portion only.
                if ((dwKeyState & CTRL_PRESSED) &&
                    !AT_EOL(pCookedReadData) &&
                    fStartFromDelim ^ !IsWordDelim(*pCookedReadData->_BufPtr))
                {
                    goto del_repeat;
                }
            }
            break;
        default:
            ASSERT(FALSE);
            break;
        }
    }

    if (UpdateCursorPosition && pCookedReadData->_Echo)
    {
        Status = AdjustCursorPosition(pCookedReadData->_pScreenInfo, CurrentPosition, TRUE, nullptr);
        ASSERT(NT_SUCCESS(Status));
    }

    return STATUS_SUCCESS;
}

void DrawCommandListBorder(_In_ PCLE_POPUP const Popup, _In_ PSCREEN_INFORMATION const ScreenInfo)
{
    // fill attributes of top line
    COORD WriteCoord;
    WriteCoord.X = Popup->Region.Left;
    WriteCoord.Y = Popup->Region.Top;
    ULONG Length = POPUP_SIZE_X(Popup) + 2;
    LOG_IF_FAILED(FillOutput(ScreenInfo, Popup->Attributes.GetLegacyAttributes(), WriteCoord, CONSOLE_ATTRIBUTE, &Length));

    // draw upper left corner
    Length = 1;
    LOG_IF_FAILED(FillOutput(ScreenInfo, ScreenInfo->LineChar[UPPER_LEFT_CORNER], WriteCoord, CONSOLE_REAL_UNICODE, &Length));

    // draw upper bar
    WriteCoord.X += 1;
    Length = POPUP_SIZE_X(Popup);
    LOG_IF_FAILED(FillOutput(ScreenInfo, ScreenInfo->LineChar[HORIZONTAL_LINE], WriteCoord, CONSOLE_REAL_UNICODE, &Length));

    // draw upper right corner
    WriteCoord.X = Popup->Region.Right;
    Length = 1;
    LOG_IF_FAILED(FillOutput(ScreenInfo, ScreenInfo->LineChar[UPPER_RIGHT_CORNER], WriteCoord, CONSOLE_REAL_UNICODE, &Length));

    for (SHORT i = 0; i < POPUP_SIZE_Y(Popup); i++)
    {
        WriteCoord.Y += 1;
        WriteCoord.X = Popup->Region.Left;

        // fill attributes
        Length = POPUP_SIZE_X(Popup) + 2;
        LOG_IF_FAILED(FillOutput(ScreenInfo, Popup->Attributes.GetLegacyAttributes(), WriteCoord, CONSOLE_ATTRIBUTE, &Length));
        Length = 1;
        LOG_IF_FAILED(FillOutput(ScreenInfo, ScreenInfo->LineChar[VERTICAL_LINE], WriteCoord, CONSOLE_REAL_UNICODE, &Length));
        WriteCoord.X = Popup->Region.Right;
        Length = 1;
        LOG_IF_FAILED(FillOutput(ScreenInfo, ScreenInfo->LineChar[VERTICAL_LINE], WriteCoord, CONSOLE_REAL_UNICODE, &Length));
    }

    // Draw bottom line.
    // Fill attributes of top line.
    WriteCoord.X = Popup->Region.Left;
    WriteCoord.Y = Popup->Region.Bottom;
    Length = POPUP_SIZE_X(Popup) + 2;
    LOG_IF_FAILED(FillOutput(ScreenInfo, Popup->Attributes.GetLegacyAttributes(), WriteCoord, CONSOLE_ATTRIBUTE, &Length));

    // Draw bottom left corner.
    Length = 1;
    WriteCoord.X = Popup->Region.Left;
    LOG_IF_FAILED(FillOutput(ScreenInfo, ScreenInfo->LineChar[BOTTOM_LEFT_CORNER], WriteCoord, CONSOLE_REAL_UNICODE, &Length));

    // Draw lower bar.
    WriteCoord.X += 1;
    Length = POPUP_SIZE_X(Popup);
    LOG_IF_FAILED(FillOutput(ScreenInfo, ScreenInfo->LineChar[HORIZONTAL_LINE], WriteCoord, CONSOLE_REAL_UNICODE, &Length));

    // draw lower right corner
    WriteCoord.X = Popup->Region.Right;
    Length = 1;
    LOG_IF_FAILED(FillOutput(ScreenInfo, ScreenInfo->LineChar[BOTTOM_RIGHT_CORNER], WriteCoord, CONSOLE_REAL_UNICODE, &Length));
}

void UpdateHighlight(_In_ PCLE_POPUP Popup,
                     _In_ SHORT OldCurrentCommand, // command number, not index
                     _In_ SHORT NewCurrentCommand,
                     _In_ PSCREEN_INFORMATION ScreenInfo)
{
    SHORT TopIndex;
    if (Popup->BottomIndex < POPUP_SIZE_Y(Popup))
    {
        TopIndex = 0;
    }
    else
    {
        TopIndex = (SHORT)(Popup->BottomIndex - POPUP_SIZE_Y(Popup) + 1);
    }
    const WORD PopupLegacyAttributes = Popup->Attributes.GetLegacyAttributes();
    COORD WriteCoord;
    WriteCoord.X = (SHORT)(Popup->Region.Left + 1);
    ULONG lStringLength = POPUP_SIZE_X(Popup);

    WriteCoord.Y = (SHORT)(Popup->Region.Top + 1 + OldCurrentCommand - TopIndex);
    LOG_IF_FAILED(FillOutput(ScreenInfo, PopupLegacyAttributes, WriteCoord, CONSOLE_ATTRIBUTE, &lStringLength));

    // highlight new command
    WriteCoord.Y = (SHORT)(Popup->Region.Top + 1 + NewCurrentCommand - TopIndex);

    // inverted attributes
    WORD const Attributes = (WORD)(((PopupLegacyAttributes << 4) & 0xf0) | ((PopupLegacyAttributes >> 4) & 0x0f));
    LOG_IF_FAILED(FillOutput(ScreenInfo, Attributes, WriteCoord, CONSOLE_ATTRIBUTE, &lStringLength));
}

void DrawCommandListPopup(_In_ PCLE_POPUP const Popup,
                          _In_ SHORT const CurrentCommand,
                          _In_ PCOMMAND_HISTORY const CommandHistory,
                          _In_ PSCREEN_INFORMATION const ScreenInfo)
{
    // draw empty popup
    COORD WriteCoord;
    WriteCoord.X = (SHORT)(Popup->Region.Left + 1);
    WriteCoord.Y = (SHORT)(Popup->Region.Top + 1);
    ULONG lStringLength = POPUP_SIZE_X(Popup);
    for (SHORT i = 0; i < POPUP_SIZE_Y(Popup); ++i)
    {
        LOG_IF_FAILED(FillOutput(ScreenInfo,
                                 Popup->Attributes.GetLegacyAttributes(),
                                 WriteCoord,
                                 CONSOLE_ATTRIBUTE,
                                 &lStringLength));
        LOG_IF_FAILED(FillOutput(ScreenInfo,
                                 (WCHAR)' ',
                                 WriteCoord,
                                 CONSOLE_FALSE_UNICODE,   // faster than real unicode
                                 &lStringLength));
        WriteCoord.Y += 1;
    }

    WriteCoord.Y = (SHORT)(Popup->Region.Top + 1);
    SHORT i = std::max(gsl::narrow<SHORT>(Popup->BottomIndex - POPUP_SIZE_Y(Popup) + 1), 0i16);
    for (; i <= Popup->BottomIndex; i++)
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
        if (CommandNumberLength > (ULONG)POPUP_SIZE_X(Popup))
        {
            CommandNumberLength = (ULONG)POPUP_SIZE_X(Popup);
        }

        WriteCoord.X = (SHORT)(Popup->Region.Left + 1);
        LOG_IF_FAILED(WriteOutputString(ScreenInfo,
                                        CommandNumberPtr,
                                        WriteCoord,
                                        CONSOLE_ASCII,
                                        (PULONG)& CommandNumberLength,
                                        nullptr));

        // write command to screen
        lStringLength = CommandHistory->Commands[COMMAND_NUM_TO_INDEX(i, CommandHistory)]->CommandLength / sizeof(WCHAR);
        {
            DWORD lTmpStringLength = lStringLength;
            LONG lPopupLength = (LONG)(POPUP_SIZE_X(Popup) - CommandNumberLength);
            LPWSTR lpStr = CommandHistory->Commands[COMMAND_NUM_TO_INDEX(i, CommandHistory)]->Command;
            while (lTmpStringLength--)
            {
                if (IsCharFullWidth(*lpStr++))
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

        WriteCoord.X = (SHORT)(WriteCoord.X + CommandNumberLength);
        {
            PWCHAR TransBuffer;

            TransBuffer = new WCHAR[lStringLength];
            if (TransBuffer == nullptr)
            {
                return;
            }

            memmove(TransBuffer, CommandHistory->Commands[COMMAND_NUM_TO_INDEX(i, CommandHistory)]->Command, lStringLength * sizeof(WCHAR));
            LOG_IF_FAILED(WriteOutputString(ScreenInfo, TransBuffer, WriteCoord, CONSOLE_REAL_UNICODE, &lStringLength, nullptr));
            delete[] TransBuffer;
        }

        // write attributes to screen
        if (COMMAND_NUM_TO_INDEX(i, CommandHistory) == CurrentCommand)
        {
            WriteCoord.X = (SHORT)(Popup->Region.Left + 1);
            WORD PopupLegacyAttributes = Popup->Attributes.GetLegacyAttributes();
            // inverted attributes
            WORD const Attributes = (WORD)(((PopupLegacyAttributes << 4) & 0xf0) | ((PopupLegacyAttributes >> 4) & 0x0f));
            lStringLength = POPUP_SIZE_X(Popup);
            LOG_IF_FAILED(FillOutput(ScreenInfo, Attributes, WriteCoord, CONSOLE_ATTRIBUTE, &lStringLength));
        }

        WriteCoord.Y += 1;
    }
}

void UpdateCommandListPopup(_In_ SHORT Delta,
                            _Inout_ PSHORT CurrentCommand,   // real index, not command #
                            _In_ PCOMMAND_HISTORY const CommandHistory,
                            _In_ PCLE_POPUP Popup,
                            _In_ PSCREEN_INFORMATION const ScreenInfo,
                            _In_ DWORD const Flags)
{
    if (Delta == 0)
    {
        return;
    }
    SHORT const Size = POPUP_SIZE_Y(Popup);

    SHORT CurCmdNum;
    SHORT NewCmdNum;

    if (Flags & UCLP_WRAP)
    {
        CurCmdNum = *CurrentCommand;
        NewCmdNum = CurCmdNum + Delta;
        NewCmdNum = COMMAND_INDEX_TO_NUM(NewCmdNum, CommandHistory);
        CurCmdNum = COMMAND_INDEX_TO_NUM(CurCmdNum, CommandHistory);
    }
    else
    {
        CurCmdNum = COMMAND_INDEX_TO_NUM(*CurrentCommand, CommandHistory);
        NewCmdNum = CurCmdNum + Delta;
        if (NewCmdNum >= CommandHistory->NumberOfCommands)
        {
            NewCmdNum = (SHORT)(CommandHistory->NumberOfCommands - 1);
        }
        else if (NewCmdNum < 0)
        {
            NewCmdNum = 0;
        }
    }
    Delta = NewCmdNum - CurCmdNum;

    BOOL Scroll = FALSE;
    // determine amount to scroll, if any
    if (NewCmdNum <= Popup->BottomIndex - Size)
    {
        Popup->BottomIndex += Delta;
        if (Popup->BottomIndex < (SHORT)(Size - 1))
        {
            Popup->BottomIndex = (SHORT)(Size - 1);
        }
        Scroll = TRUE;
    }
    else if (NewCmdNum > Popup->BottomIndex)
    {
        Popup->BottomIndex += Delta;
        if (Popup->BottomIndex >= CommandHistory->NumberOfCommands)
        {
            Popup->BottomIndex = (SHORT)(CommandHistory->NumberOfCommands - 1);
        }
        Scroll = TRUE;
    }

    // write commands to popup
    if (Scroll)
    {
        DrawCommandListPopup(Popup, COMMAND_NUM_TO_INDEX(NewCmdNum, CommandHistory), CommandHistory, ScreenInfo);
    }
    else
    {
        UpdateHighlight(Popup, COMMAND_INDEX_TO_NUM((*CurrentCommand), CommandHistory), NewCmdNum, ScreenInfo);
    }

    *CurrentCommand = COMMAND_NUM_TO_INDEX(NewCmdNum, CommandHistory);
}

UINT LoadStringEx(_In_ HINSTANCE hModule, _In_ UINT wID, _Out_writes_(cchBufferMax) LPWSTR lpBuffer, _In_ UINT cchBufferMax, _In_ WORD wLangId)
{
    // Make sure the parms are valid.
    if (lpBuffer == nullptr)
    {
        return 0;
    }

    UINT cch = 0;

    // String Tables are broken up into 16 string segments.  Find the segment containing the string we are interested in.
    HANDLE const hResInfo = FindResourceEx(hModule, RT_STRING, (LPTSTR)((LONG_PTR)(((USHORT)wID >> 4) + 1)), wLangId);
    if (hResInfo != nullptr)
    {
        // Load that segment.
        HANDLE const hStringSeg = (HRSRC)LoadResource(hModule, (HRSRC)hResInfo);

        // Lock the resource.
        LPTSTR lpsz;
        if (hStringSeg != nullptr && (lpsz = (LPTSTR)LockResource(hStringSeg)) != nullptr)
        {
            // Move past the other strings in this segment. (16 strings in a segment -> & 0x0F)
            wID &= 0x0F;
            for (;;)
            {
                cch = *((WCHAR *)lpsz++);   // PASCAL like string count
                // first WCHAR is count of WCHARs
                if (wID-- == 0)
                {
                    break;
                }

                lpsz += cch;    // Step to start if next string
            }

            // chhBufferMax == 0 means return a pointer to the read-only resource buffer.
            if (cchBufferMax == 0)
            {
                *(LPTSTR *)lpBuffer = lpsz;
            }
            else
            {
                // Account for the nullptr
                cchBufferMax--;

                // Don't copy more than the max allowed.
                if (cch > cchBufferMax)
                    cch = cchBufferMax;

                // Copy the string into the buffer.
                memmove(lpBuffer, lpsz, cch * sizeof(WCHAR));
            }
        }
    }

    // Append a nullptr.
    if (cchBufferMax != 0)
    {
        lpBuffer[cch] = 0;
    }

    return cch;
}
