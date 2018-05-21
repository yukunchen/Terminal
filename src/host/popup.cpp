/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/
#include "precomp.h"

#include "popup.h"

#include "_output.h"
#include "output.h"

#include "dbcs.h"
#include "srvinit.h"

#include "resource.h"

#include "..\interactivity\inc\ServiceLocator.hpp"

#pragma hdrstop

// Routine Description:
// - Creates an object representing an interactive popup overlay during cooked mode command line editing.
// - NOTE: Modifies global popup count (and adjusts cursor visibility as appropriate.)
// Arguments:
// - screenInfo - Reference to screen on which the popup should be drawn/overlayed.
// - proposedSize - Suggested size of the popup. May be adjusted based on screen size.
// - history - Pointer to list of commands that were run to be displayed on some types of popups
// - func - Which type of popup is this?
Popup::Popup(SCREEN_INFORMATION& screenInfo, const COORD proposedSize, CommandHistory* const history, const PopFunc func) :
    CurrentCommand(0),
    NumberRead(0),
    _callbackOption(func),
    BottomIndex(history->LastDisplayed),
    _screenInfo(screenInfo),
    _history(history)
{
    Attributes = screenInfo.GetPopupAttributes()->GetLegacyAttributes();

    std::fill_n(NumberBuffer, ARRAYSIZE(NumberBuffer), UNICODE_NULL);

    const COORD size = _CalculateSize(screenInfo, proposedSize);
    const COORD origin = _CalculateOrigin(screenInfo, size);

    _region.Left = origin.X;
    _region.Top = origin.Y;
    _region.Right = gsl::narrow<SHORT>(origin.X + size.X - 1i16);
    _region.Bottom = gsl::narrow<SHORT>(origin.Y + size.Y - 1i16);

    _oldScreenSize = screenInfo.GetScreenBufferSize();

    SMALL_RECT TargetRect;
    TargetRect.Left = 0;
    TargetRect.Top = _region.Top;
    TargetRect.Right = _oldScreenSize.X - 1;
    TargetRect.Bottom = _region.Bottom;

    std::vector<std::vector<OutputCell>> outputCells;
    LOG_IF_FAILED(ReadScreenBuffer(screenInfo, outputCells, &TargetRect));
    FAIL_FAST_IF(outputCells.empty());
    // copy the data into the char info buffer
    for (auto& row : outputCells)
    {
        for (auto& cell : row)
        {
            _oldContents.push_back(cell.ToCharInfo());
        }
    }

    CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    gci.PopupCount++;
    if (1 == gci.PopupCount)
    {
        // If this is the first popup to be shown, stop the cursor from appearing/blinking
        screenInfo.GetTextBuffer().GetCursor().SetIsPopupShown(true);
    }

    _DrawBorder();
}

// Routine Description:
// - Cleans up a popup object
// - NOTE: Modifies global popup count (and adjusts cursor visibility as appropriate.)
Popup::~Popup()
{
    CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    gci.PopupCount--;
    if (0 == gci.PopupCount)
    {
        // Notify we're done showing popups.
        gci.GetActiveOutputBuffer().GetTextBuffer().GetCursor().SetIsPopupShown(false);
    }
}

// Routine Description:
// - Performs drawing operations relative to the given popup object.
// - This completely encapsulates backing up the data in the buffer,
//   drawing the background and border for the popup, then filling
//   the actual popup title and information to the screen.
void Popup::Draw()
{
    _DrawBorder();

    switch (_callbackOption)
    {
    case PopFunc::CommandNumber:
        _DrawPrompt(ID_CONSOLE_MSGCMDLINEF9);
        break;
    case PopFunc::CopyToChar:
        _DrawPrompt(ID_CONSOLE_MSGCMDLINEF2);
        break;
    case PopFunc::CopyFromChar:
        _DrawPrompt(ID_CONSOLE_MSGCMDLINEF4);
        break;
    case PopFunc::CommandList:
        _DrawList();
        break;
    default:
        THROW_HR(E_NOTIMPL);
    }
}

// Routine Description:
// - Draws the outlines of the popup area in the screen buffer
void Popup::_DrawBorder()
{
    // fill attributes of top line
    COORD WriteCoord;
    WriteCoord.X = _region.Left;
    WriteCoord.Y = _region.Top;
    FillOutputAttributes(_screenInfo, Attributes, WriteCoord, Width() + 2);

    // draw upper left corner
    FillOutputW(_screenInfo, { _screenInfo.LineChar[UPPER_LEFT_CORNER] }, WriteCoord, 1);

    // draw upper bar
    WriteCoord.X += 1;
    FillOutputW(_screenInfo, { _screenInfo.LineChar[HORIZONTAL_LINE] }, WriteCoord, Width());

    // draw upper right corner
    WriteCoord.X = _region.Right;
    FillOutputW(_screenInfo, { _screenInfo.LineChar[UPPER_RIGHT_CORNER] }, WriteCoord, 1);

    for (SHORT i = 0; i < Height(); i++)
    {
        WriteCoord.Y += 1;
        WriteCoord.X = _region.Left;

        // fill attributes
        FillOutputAttributes(_screenInfo, Attributes, WriteCoord, Width() + 2);

        FillOutputW(_screenInfo, { _screenInfo.LineChar[VERTICAL_LINE] }, WriteCoord, 1);

        WriteCoord.X = _region.Right;
        FillOutputW(_screenInfo, { _screenInfo.LineChar[VERTICAL_LINE] }, WriteCoord, 1);
    }

    // _DrawBorder bottom line.
    // Fill attributes of top line.
    WriteCoord.X = _region.Left;
    WriteCoord.Y = _region.Bottom;
    FillOutputAttributes(_screenInfo, Attributes, WriteCoord, Width() + 2);

    // _DrawBorder bottom left corner.
    WriteCoord.X = _region.Left;
    FillOutputW(_screenInfo, { _screenInfo.LineChar[BOTTOM_LEFT_CORNER] }, WriteCoord, 1);

    // _DrawBorder lower bar.
    WriteCoord.X += 1;
    FillOutputW(_screenInfo, { _screenInfo.LineChar[HORIZONTAL_LINE] }, WriteCoord, Width());

    // draw lower right corner
    WriteCoord.X = _region.Right;
    FillOutputW(_screenInfo, { _screenInfo.LineChar[BOTTOM_RIGHT_CORNER] }, WriteCoord, 1);
}

// Routine Description:
// - Draws prompt information in the popup area to tell the user what to enter.
// Arguments:
// - id - Resource ID for string to display to user
void Popup::_DrawPrompt(const UINT id)
{
    std::wstring text = _LoadString(id);

    // _DrawBorder empty popup.
    COORD WriteCoord;
    WriteCoord.X = (SHORT)(_region.Left + 1);
    WriteCoord.Y = (SHORT)(_region.Top + 1);
    size_t lStringLength = Width();
    for (SHORT i = 0; i < Height(); i++)
    {
        lStringLength = gsl::narrow<ULONG>(FillOutputAttributes(_screenInfo,
                                                                Attributes,
                                                                WriteCoord,
                                                                static_cast<size_t>(lStringLength)));

        lStringLength = gsl::narrow<ULONG>(FillOutputW(_screenInfo,
                                                       { UNICODE_SPACE },
                                                       WriteCoord,
                                                       static_cast<size_t>(lStringLength)));

        WriteCoord.Y += 1;
    }

    WriteCoord.X = (SHORT)(_region.Left + 1);
    WriteCoord.Y = (SHORT)(_region.Top + 1);

    // write prompt to screen
    lStringLength = text.size();
    if (lStringLength > (ULONG)Width())
    {
        text = text.substr(0, Width());
    }

    std::vector<wchar_t> promptChars(text.cbegin(), text.cend());
    WriteOutputStringW(_screenInfo, promptChars, WriteCoord);
}

// Routine Description:
// - Draws a list of commands for the user to choose from
void Popup::_DrawList()
{
    // draw empty popup
    COORD WriteCoord;
    WriteCoord.X = (SHORT)(_region.Left + 1);
    WriteCoord.Y = (SHORT)(_region.Top + 1);
    size_t lStringLength = Width();
    for (SHORT i = 0; i < Height(); ++i)
    {
        lStringLength = FillOutputAttributes(_screenInfo,
                                             Attributes,
                                             WriteCoord,
                                             lStringLength);
        lStringLength = FillOutputW(_screenInfo,
                                    { UNICODE_SPACE },
                                    WriteCoord,
                                    lStringLength);
        WriteCoord.Y += 1;
    }

    WriteCoord.Y = (SHORT)(_region.Top + 1);
    SHORT i = std::max(gsl::narrow<SHORT>(BottomIndex - Height() + 1), 0i16);
    for (; i <= BottomIndex; i++)
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
        if (CommandNumberLength > (ULONG)Width())
        {
            CommandNumberLength = (ULONG)Width();
        }

        WriteCoord.X = (SHORT)(_region.Left + 1);
        try
        {
            std::vector<char> chars{ CommandNumberPtr, CommandNumberPtr + CommandNumberLength };
            CommandNumberLength = WriteOutputStringA(_screenInfo,
                                                     chars,
                                                     WriteCoord);
        }
        CATCH_LOG();

        // write command to screen
        auto command = _history->GetNth(i);
        lStringLength = command.size();
        {
            size_t lTmpStringLength = lStringLength;
            LONG lPopupLength = (LONG)(Width() - CommandNumberLength);
            PCWCHAR lpStr = command.data();
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

        std::vector<wchar_t> chars{ command.data(), command.data() + lStringLength };
        WriteOutputStringW(_screenInfo, chars, WriteCoord);

        // write attributes to screen
        if (i == CurrentCommand)
        {
            WriteCoord.X = (SHORT)(_region.Left + 1);
            WORD PopupLegacyAttributes = Attributes.GetLegacyAttributes();
            // inverted attributes
            WORD const attr = (WORD)(((PopupLegacyAttributes << 4) & 0xf0) | ((PopupLegacyAttributes >> 4) & 0x0f));
            lStringLength = Width();
            lStringLength = FillOutputAttributes(_screenInfo, attr, WriteCoord, lStringLength);
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
void Popup::Update(const SHORT originalDelta, const bool wrap)
{
    SHORT delta = originalDelta;
    if (delta == 0)
    {
        return;
    }
    SHORT const Size = (SHORT)Height();

    SHORT CurCmdNum;
    SHORT NewCmdNum;

    if (wrap)
    {
        CurCmdNum = CurrentCommand;
        NewCmdNum = CurCmdNum + delta;
    }
    else
    {
        CurCmdNum = CurrentCommand;
        NewCmdNum = CurCmdNum + delta;
        if (NewCmdNum >= _history->GetNumberOfCommands())
        {
            NewCmdNum = (SHORT)(_history->GetNumberOfCommands() - 1);
        }
        else if (NewCmdNum < 0)
        {
            NewCmdNum = 0;
        }
    }
    delta = NewCmdNum - CurCmdNum;

    bool Scroll = false;
    // determine amount to scroll, if any
    if (NewCmdNum <= BottomIndex - Size)
    {
        BottomIndex += delta;
        if (BottomIndex < (SHORT)(Size - 1))
        {
            BottomIndex = (SHORT)(Size - 1);
        }
        Scroll = true;
    }
    else if (NewCmdNum > BottomIndex)
    {
        BottomIndex += delta;
        if (BottomIndex >= _history->GetNumberOfCommands())
        {
            BottomIndex = (SHORT)(_history->GetNumberOfCommands() - 1);
        }
        Scroll = true;
    }

    // write commands to popup
    if (Scroll)
    {
        CurrentCommand = NewCmdNum;
        _DrawList();
    }
    else
    {
        _UpdateHighlight(CurrentCommand, NewCmdNum);
        CurrentCommand = NewCmdNum;
    }
}

// Routine Description:
// - Adjusts the highligted line in a list of commands
// Arguments:
// - OldCurrentCommand - The previous command highlighted
// - NewCurrentCommand - The new command to be highlighted.
void Popup::_UpdateHighlight(const SHORT OldCurrentCommand, const SHORT NewCurrentCommand)
{
    SHORT TopIndex;
    if (BottomIndex < Height())
    {
        TopIndex = 0;
    }
    else
    {
        TopIndex = (SHORT)(BottomIndex - Height() + 1);
    }
    const WORD PopupLegacyAttributes = Attributes.GetLegacyAttributes();
    COORD WriteCoord;
    WriteCoord.X = (SHORT)(_region.Left + 1);
    size_t lStringLength = Width();

    WriteCoord.Y = (SHORT)(_region.Top + 1 + OldCurrentCommand - TopIndex);
    lStringLength = FillOutputAttributes(_screenInfo, PopupLegacyAttributes, WriteCoord, lStringLength);

    // highlight new command
    WriteCoord.Y = (SHORT)(_region.Top + 1 + NewCurrentCommand - TopIndex);

    // inverted attributes
    WORD const attr = (WORD)(((PopupLegacyAttributes << 4) & 0xf0) | ((PopupLegacyAttributes >> 4) & 0x0f));
    lStringLength = FillOutputAttributes(_screenInfo, attr, WriteCoord, lStringLength);
}

// Routine Description:
// - Updates the colors of the backed up information inside this popup.
// - This is useful if user preferences change while a popup is displayed.
// Arguments:
// - newAttr - The new default color for text in the buffer
// - newPopupAttr - The new color for text in popups
// - oldAttr - The previous default color for text in the buffer
// - oldPopupAttr - The previous color for text in popups
void Popup::UpdateStoredColors(const WORD newAttr, const WORD newPopupAttr,
                               const WORD oldAttr, const WORD oldPopupAttr)
{
    // We also want to find and replace the inversion of the popup colors in case there are highlights
    WORD const oldPopupAttrInv = (WORD)(((oldPopupAttr << 4) & 0xf0) | ((oldPopupAttr >> 4) & 0x0f));
    WORD const newPopupAttrInv = (WORD)(((newPopupAttr << 4) & 0xf0) | ((newPopupAttr >> 4) & 0x0f));

    for (auto& ci : _oldContents)
    {
        if (ci.Attributes == oldAttr)
        {
            ci.Attributes = newAttr;
        }
        else if (ci.Attributes == oldPopupAttr)
        {
            ci.Attributes = newPopupAttr;
        }
        else if (ci.Attributes == oldPopupAttrInv)
        {
            ci.Attributes = newPopupAttrInv;
        }

    }
}

// Routine Description:
// - Cleans up a popup by restoring the stored buffer information to the region of
//   the screen that the popup was covering and frees resources.
void Popup::End()
{
    // restore previous contents to screen
    COORD Size;
    Size.X = _oldScreenSize.X;
    Size.Y = (SHORT)(_region.Bottom - _region.Top + 1);

    SMALL_RECT SourceRect;
    SourceRect.Left = 0;
    SourceRect.Top = _region.Top;
    SourceRect.Right = _oldScreenSize.X - 1;
    SourceRect.Bottom = _region.Bottom;

    LOG_IF_FAILED(WriteScreenBuffer(_screenInfo, _oldContents.data(), &SourceRect));
    WriteToScreen(_screenInfo, SourceRect);
}

// Routine Description:
// - For popups delayed by a wait for more data, this helps us re-run the input logic
//   for the specific popup type selected on construction.
// Arguments:
// - data - The cooked read data object representing the current input state
// Return Value:
// - Passthrough of one of the original popup servicing routines (see cmdline.cpp)
HRESULT Popup::DoCallback(COOKED_READ_DATA* const data)
{
    try
    {
        switch (_callbackOption)
        {
        case PopFunc::CommandList:
            return ProcessCommandListInput(data);
        case PopFunc::CommandNumber:
            return ProcessCommandNumberInput(data);
        case PopFunc::CopyFromChar:
            return ProcessCopyFromCharInput(data);
        case PopFunc::CopyToChar:
            return ProcessCopyToCharInput(data);
        default:
            return E_NOTIMPL;
        }
    }
    CATCH_RETURN();
}

// Routine Description:
// - Helper to calculate the size of the popup.
// Arguments:
// - screenInfo - Screen buffer we will be drawing into
// - proposedSize - The suggested size of the popup that may need to be adjusted to fit
// Return Value:
// - Coordinate size that the popup should consume in the screen buffer
COORD Popup::_CalculateSize(const SCREEN_INFORMATION& screenInfo, const COORD proposedSize)
{
    // determine popup dimensions
    COORD size = proposedSize;
    size.X += 2;    // add borders
    size.Y += 2;    // add borders
    if (size.X >= (SHORT)(screenInfo.GetScreenWindowSizeX()))
    {
        size.X = (SHORT)(screenInfo.GetScreenWindowSizeX());
    }
    if (size.Y >= (SHORT)(screenInfo.GetScreenWindowSizeY()))
    {
        size.Y = (SHORT)(screenInfo.GetScreenWindowSizeY());
    }

    // make sure there's enough room for the popup borders
    THROW_HR_IF(E_NOT_SUFFICIENT_BUFFER, size.X < 2 || size.Y < 2);

    return size;
}

// Routine Description:
// - Helper to calculate the origin point (within the screen buffer) for the popup
// Arguments:
// - screenInfo - Screen buffer we will be drawing into
// - size - The size that the popup will consume
// Return Value:
// - Coordinate position of the origin point of the popup
COORD Popup::_CalculateOrigin(const SCREEN_INFORMATION& screenInfo, const COORD size)
{
    // determine origin.  center popup on window
    COORD origin;
    origin.X = (SHORT)((screenInfo.GetScreenWindowSizeX() - size.X) / 2 + screenInfo.GetBufferViewport().Left);
    origin.Y = (SHORT)((screenInfo.GetScreenWindowSizeY() - size.Y) / 2 + screenInfo.GetBufferViewport().Top);
    return origin;
}

// Routine Description:
// - Helper to return the width of the popup in columns
// Return Value:
// - Width of popup inside attached screen buffer.
SHORT Popup::Width() const noexcept
{
    return _region.Right - _region.Left - 1i16;
}

// Routine Description:
// - Helper to return the height of the popup in columns
// Return Value:
// - Height of popup inside attached screen buffer.
SHORT Popup::Height() const noexcept
{
    return _region.Bottom - _region.Top - 1i16;
}

// Routine Description:
// - Helper to get the position on top of some types of popup dialogs where
//   we should overlay the cursor for user input.
// Return Value:
// - Coordinate location on the popup where the cursor should be placed.
COORD Popup::GetCursorPosition() const noexcept
{
    COORD CursorPosition;
    CursorPosition.X = (SHORT)(_region.Right - MINIMUM_COMMAND_PROMPT_SIZE);
    CursorPosition.Y = (SHORT)(_region.Top + 1);
    return CursorPosition;
}

// Routine Description:
// - Helper to retrieve string resources from our resource files.
// Arguments:
// - id - Resource id from resource.h to the string we need to load.
// Return Value:
// - The string resource
std::wstring Popup::_LoadString(const UINT id)
{
    const CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    WCHAR ItemString[70];
    size_t ItemLength = 0;
    LANGID LangId;

    const NTSTATUS Status = GetConsoleLangId(gci.OutputCP, &LangId);
    if (NT_SUCCESS(Status))
    {
        ItemLength = s_LoadStringEx(ServiceLocator::LocateGlobals().hInstance, id, ItemString, ARRAYSIZE(ItemString), LangId);
    }
    if (!NT_SUCCESS(Status) || ItemLength == 0)
    {
        ItemLength = LoadStringW(ServiceLocator::LocateGlobals().hInstance, id, ItemString, ARRAYSIZE(ItemString));
    }

    return std::wstring(ItemString, ItemLength);
}

// Routine Description:
// - Helper to retrieve string resources from a MUI with a particular LANGID.
// Arguments:
// - hModule - The module related to loading the resource
// - wID - The resource ID number
// - lpBuffer - Buffer to place string data when read.
// - cchBufferMax - Size of buffer
// - wLangId - Language ID of resources that we should retrieve.
UINT Popup::s_LoadStringEx(_In_ HINSTANCE hModule, _In_ UINT wID, _Out_writes_(cchBufferMax) LPWSTR lpBuffer, _In_ UINT cchBufferMax, _In_ WORD wLangId)
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
