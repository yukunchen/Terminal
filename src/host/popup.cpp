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
Popup::Popup(SCREEN_INFORMATION& screenInfo, const COORD proposedSize) :
    _screenInfo(screenInfo)
{
    _attributes = screenInfo.GetPopupAttributes()->GetLegacyAttributes();

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
    const auto countWas = gci.PopupCount.fetch_add(1ui16);
    if (0 == countWas)
    {
        // If this is the first popup to be shown, stop the cursor from appearing/blinking
        screenInfo.GetTextBuffer().GetCursor().SetIsPopupShown(true);
    }
}

// Routine Description:
// - Cleans up a popup object
// - NOTE: Modifies global popup count (and adjusts cursor visibility as appropriate.)
Popup::~Popup()
{
    CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    const auto countWas = gci.PopupCount.fetch_sub(1i16);
    if (1 == countWas)
    {
        // Notify we're done showing popups.
        gci.GetActiveOutputBuffer().GetTextBuffer().GetCursor().SetIsPopupShown(false);
    }
}

void Popup::Draw()
{
    _DrawBorder();
    _DrawContent();
}

// Routine Description:
// - Draws the outlines of the popup area in the screen buffer
void Popup::_DrawBorder()
{
    // fill attributes of top line
    COORD WriteCoord;
    WriteCoord.X = _region.Left;
    WriteCoord.Y = _region.Top;
    FillOutputAttributes(_screenInfo, _attributes, WriteCoord, Width() + 2);

    // draw upper left corner
    FillOutputW(_screenInfo, _screenInfo.LineChar[UPPER_LEFT_CORNER], WriteCoord, 1);

    // draw upper bar
    WriteCoord.X += 1;
    FillOutputW(_screenInfo, _screenInfo.LineChar[HORIZONTAL_LINE], WriteCoord, Width());

    // draw upper right corner
    WriteCoord.X = _region.Right;
    FillOutputW(_screenInfo, _screenInfo.LineChar[UPPER_RIGHT_CORNER], WriteCoord, 1);

    for (SHORT i = 0; i < Height(); i++)
    {
        WriteCoord.Y += 1;
        WriteCoord.X = _region.Left;

        // fill attributes
        FillOutputAttributes(_screenInfo, _attributes, WriteCoord, Width() + 2);

        FillOutputW(_screenInfo, _screenInfo.LineChar[VERTICAL_LINE], WriteCoord, 1);

        WriteCoord.X = _region.Right;
        FillOutputW(_screenInfo, _screenInfo.LineChar[VERTICAL_LINE], WriteCoord, 1);
    }

    // Draw bottom line.
    // Fill attributes of top line.
    WriteCoord.X = _region.Left;
    WriteCoord.Y = _region.Bottom;
    FillOutputAttributes(_screenInfo, _attributes, WriteCoord, Width() + 2);

    // Draw bottom left corner.
    WriteCoord.X = _region.Left;
    FillOutputW(_screenInfo, _screenInfo.LineChar[BOTTOM_LEFT_CORNER], WriteCoord, 1);

    // Draw lower bar.
    WriteCoord.X += 1;
    FillOutputW(_screenInfo, _screenInfo.LineChar[HORIZONTAL_LINE], WriteCoord, Width());

    // draw lower right corner
    WriteCoord.X = _region.Right;
    FillOutputW(_screenInfo, _screenInfo.LineChar[BOTTOM_RIGHT_CORNER], WriteCoord, 1);
}

// Routine Description:
// - Draws prompt information in the popup area to tell the user what to enter.
// Arguments:
// - id - Resource ID for string to display to user
void Popup::_DrawPrompt(const UINT id)
{
    std::wstring text = _LoadString(id);

    // Draw empty popup.
    COORD WriteCoord;
    WriteCoord.X = _region.Left + 1i16;
    WriteCoord.Y = _region.Top + 1i16;
    size_t lStringLength = Width();
    for (SHORT i = 0; i < Height(); i++)
    {
        lStringLength = gsl::narrow<ULONG>(FillOutputAttributes(_screenInfo,
                                                                _attributes,
                                                                WriteCoord,
                                                                static_cast<size_t>(lStringLength)));

        lStringLength = gsl::narrow<ULONG>(FillOutputW(_screenInfo,
                                                       UNICODE_SPACE,
                                                       WriteCoord,
                                                       static_cast<size_t>(lStringLength)));

        WriteCoord.Y += 1;
    }

    WriteCoord.X = _region.Left + 1i16;
    WriteCoord.Y = _region.Top + 1i16;

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
    Size.Y = _region.Bottom - _region.Top + 1i16;

    SMALL_RECT SourceRect;
    SourceRect.Left = 0i16;
    SourceRect.Top = _region.Top;
    SourceRect.Right = _oldScreenSize.X - 1i16;
    SourceRect.Bottom = _region.Bottom;

    LOG_IF_FAILED(WriteScreenBuffer(_screenInfo, _oldContents.data(), &SourceRect));
    WriteToScreen(_screenInfo, SourceRect);
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
    CursorPosition.X = _region.Right - static_cast<SHORT>(MINIMUM_COMMAND_PROMPT_SIZE);
    CursorPosition.Y = _region.Top + 1i16;
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
