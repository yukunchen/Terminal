/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "conimeinfo.h"
#include "conareainfo.h"

#include "_output.h"
#include "dbcs.h"

#include "../interactivity/inc/ServiceLocator.hpp"
#include "../types/inc/Utf16Parser.hpp"

// Attributes flags:
#define COMMON_LVB_GRID_SINGLEFLAG 0x2000   // DBCS: Grid attribute: use for ime cursor.

ConsoleImeInfo::ConsoleImeInfo() :
    CompStrData(nullptr),
    SavedCursorVisible(FALSE)
{

}

ConsoleImeInfo::~ConsoleImeInfo()
{
    if (CompStrData != nullptr)
    {
        delete[] CompStrData;
    }
}

// Routine Description:
// - Copies default attribute (color) data from the active screen buffer into the conversion area buffers
void ConsoleImeInfo::RefreshAreaAttributes()
{
    const auto& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    const auto attributes = gci.GetActiveOutputBuffer().GetAttributes();

    for (auto& area : ConvAreaCompStr)
    {
        area.ScreenBuffer->SetAttributes(attributes);
    }
}

void ConsoleImeInfo::WriteCompMessage(const LPCONIME_UICOMPMESSAGE msg)
{
    ClearAllAreas();

    const auto cch = msg->dwCompStrLen / sizeof(wchar_t);
    const std::wstring_view text((LPWSTR)((PBYTE)msg + msg->dwCompStrOffset), cch);
    const std::basic_string_view<BYTE> attributes((PBYTE)msg + msg->dwCompAttrOffset, cch);
    const std::basic_string_view<WORD> colorArray((PWORD)msg->CompAttrColor, CONIME_ATTRCOLOR_SIZE);
    _WriteUndeterminedChars(text, attributes, colorArray);
}

// Routine Description:
// - Clears out all conversion areas
void ConsoleImeInfo::ClearAllAreas()
{
    for (auto& area : ConvAreaCompStr)
    {
        if (!area.IsHidden())
        {
            area.ClearArea();
        }
    }
}

// Routine Description:
// - Resizes all conversion areas to the new dimensions
// Arguments:
// - newSize - New size for conversion areas
// Return Value:
// - S_OK or appropriate failure HRESULT.
HRESULT ConsoleImeInfo::ResizeAllAreas(const COORD newSize)
{
    for (auto& area : ConvAreaCompStr)
    {
        if (!area.IsHidden())
        {
            area.SetHidden(true);
            area.Paint();
        }

        RETURN_IF_FAILED(area.Resize(newSize));
    }

    return S_OK;
}

// Routine Description:
// - Adds another conversion area to the current list of conversion areas (lines) available for IME candidate text
// Arguments:
// - <none>
// Return Value:
// - Status successful or appropriate HRESULT response.
[[nodiscard]]
HRESULT ConsoleImeInfo::_AddConversionArea()
{
    const CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();

    COORD bufferSize = gci.GetActiveOutputBuffer().GetScreenBufferSize();
    bufferSize.Y = 1;

    COORD windowSize;
    windowSize.X = gci.GetActiveOutputBuffer().GetScreenWindowSizeX();
    windowSize.Y = gci.GetActiveOutputBuffer().GetScreenWindowSizeY();

    CHAR_INFO fill;
    fill.Attributes = gci.GetActiveOutputBuffer().GetAttributes().GetLegacyAttributes();

    CHAR_INFO popupFill;
    popupFill.Attributes = gci.GetActiveOutputBuffer().GetPopupAttributes()->GetLegacyAttributes();

    const FontInfo& fontInfo = gci.GetActiveOutputBuffer().GetTextBuffer().GetCurrentFont();

    try
    {
        ConvAreaCompStr.emplace_back(bufferSize,
                                     windowSize,
                                     fill,
                                     popupFill,
                                     fontInfo);
    }
    CATCH_RETURN();

    RefreshAreaAttributes();

    return S_OK;
}

TextAttribute ConsoleImeInfo::s_RetrieveAttributeAt(const size_t pos,
                                                    const std::basic_string_view<BYTE> attributes,
                                                    const std::basic_string_view<WORD> colorArray)
{
    // Encoded attribute is the shorthand information passed from the IME
    // that contains a cursor position packed in along with which color in the 
    // given array should apply to the text.
    auto encodedAttribute = attributes[pos];

    // Legacy attribute is in the color/line format that is understood for drawing
    // We use the lower 3 bits (0-7) from the encoded attribute as the array index to start
    // creating our legacy attribute.
    WORD legacyAttribute = colorArray[encodedAttribute & 0x7];
       
    if (IsFlagSet(encodedAttribute, CONIME_CURSOR_RIGHT))
    {
        SetFlag(legacyAttribute, COMMON_LVB_GRID_SINGLEFLAG);
        SetFlag(legacyAttribute, COMMON_LVB_GRID_RVERTICAL);
    }
    else if (IsFlagSet(encodedAttribute, CONIME_CURSOR_LEFT))
    {
        SetFlag(legacyAttribute, COMMON_LVB_GRID_SINGLEFLAG);
        SetFlag(legacyAttribute, COMMON_LVB_GRID_LVERTICAL);
    }
    
    return TextAttribute(legacyAttribute);
}

// Routine Description:
// - Converts IME-formatted information into OutputCells to determine what can fit into each
//   displayable cell inside the console output buffer.
// Arguments:
// - text - Text data provided by the IME
// - attributes - Encoded color and cursor position data provided by the IME
// - colorArray - Array of color values provided by the IME.
// Return Value:
// - Vector of OutputCells where each one represents one cell of the output buffer.
std::vector<OutputCell> ConsoleImeInfo::s_ConvertToCells(const std::wstring_view text,
                                                         const std::basic_string_view<BYTE> attributes,
                                                         const std::basic_string_view<WORD> colorArray)
{
    std::vector<OutputCell> cells;

    // - Convert incoming wchar_t stream into UTF-16 units.
    const auto glyphs = Utf16Parser::Parse(text);

    // - Walk through all of the grouped up text, match up the correct attribute to it, and make a new cell.
    size_t attributesUsed = 0;
    for (const auto& glyph : glyphs)
    {
        // Collect up attributes that apply to this glyph range.
        auto drawingAttr = s_RetrieveAttributeAt(attributesUsed, attributes, colorArray);
        attributesUsed++;

        // The IME gave us an attribute for every glyph position in a surrogate pair.
        // But the only important information will be the cursor position.
        // Check all additional attributes to see if the cursor resides on top of them.
        for (size_t i = 1; i < glyph.size(); i++)
        {
            TextAttribute additionalAttr = s_RetrieveAttributeAt(attributesUsed, attributes, colorArray);
            attributesUsed++;
            if (additionalAttr.IsLeftVerticalDisplayed())
            {
                drawingAttr.SetLeftVerticalDisplayed(true);
            }
            if (additionalAttr.IsRightVerticalDisplayed())
            {
                drawingAttr.SetRightVerticalDisplayed(true);
            }
        }

        // We have to determine if the glyph range is 1 column or two.
        // If it's full width, it's two, and we need to make sure we don't draw the cursor
        // right down the middle of the character.
        // Otherwise it's one column and we'll push it in with the default empty DbcsAttribute.
        DbcsAttribute dbcsAttr;
        if (IsGlyphFullWidth(glyph))
        {
            auto leftHalfAttr = drawingAttr;

            // Don't draw lines in the middle of full width glyphs.
            // If we need a right vertical, don't apply it to the left side of the character
            if (leftHalfAttr.IsRightVerticalDisplayed())
            {
                leftHalfAttr.SetRightVerticalDisplayed(false);
            }

            dbcsAttr.SetLeading();
            cells.emplace_back(glyph, dbcsAttr, leftHalfAttr);
            dbcsAttr.SetTrailing();

            // If we need a left vertical, don't apply it to the right side of the character
            if (drawingAttr.IsLeftVerticalDisplayed())
            {
                drawingAttr.SetLeftVerticalDisplayed(false);
            }
        }
        cells.emplace_back(glyph, dbcsAttr, drawingAttr);
    }

    return cells;
}

// Routine Description:
// - Walks through the cells given and attempts to fill a conversion area line with as much data as can fit.
// - Each conversion area represents one line of the display starting at the cursor position filling to the right edge
//   of the display.
// - The first conversion area should be placed from the screen buffer's current cursor position to the right
//   edge of the viewport.
// - All subsequent areas should use one entire line of the viewport.
// Arguments:
// - begin - Beginning position in OutputCells for iteration
// - end - Ending position in OutputCells for iteration
// - pos - Reference to the coordinate position in the viewport that this conversion area will occupy.
//       - Updated to set up the next conversion area down a line (and to the left viewport edge)
// - view - The rectangle representing the viewable area of the screen right now to let us know how many cells can fit.
// - screenInfo - A reference to the screen information we will use for accessibility notifications
// Return Value:
// - Updated begin position for the next call. It will normally be >begin and <= end.
//   However, if text couldn't fit in our line (full-width character starting at the very last cell)
//   then we will give back the same begin and update the position for the next call to try again.
//   If the viewport is deemed too small, we'll skip past it and advance begin past the entire full-width character.
std::vector<OutputCell>::const_iterator ConsoleImeInfo::_WriteConversionArea(const std::vector<OutputCell>::const_iterator begin,
                                                                             const std::vector<OutputCell>::const_iterator end,
                                                                             COORD& pos,
                                                                             const SMALL_RECT view,
                                                                             SCREEN_INFORMATION& screenInfo)
{
    // The position in the viewport where we will start inserting cells for this conversion area
    // NOTE: We might exit early if there's not enough space to fit here, so we take a copy of 
    //       the original and increment it up front.
    const auto insertionPos = pos;

    // Advance the cursor position to set up the next call for success (insert the next conversion area
    // at the beginning of the following line)
    pos.X = view.Left;
    pos.Y++;

    // The index of the last column in the viewport. (view is inclusive)
    const auto finalViewColumn = view.Right;

    // The maximum number of cells we can insert into a line.
    const auto lineWidth = finalViewColumn - insertionPos.X + 1; // +1 because view was inclusive

    // The iterator to the beginning position to form our line
    const auto lineBegin = begin;

    // The total number of cells we could insert.
    const auto size = end - begin;
    FAIL_FAST_IF(size <= 0); // It's a programming error to have <= 0 cells to insert.

    // The end is the smaller of the remaining number of cells or the amount of line cells we can write before
    // hitting the right edge of the viewport
    auto lineEnd = lineBegin + std::min(size, (ptrdiff_t)lineWidth);

    // We must attempt to compensate for ending on a leading byte. We can't split a full-width character across lines.
    // As such, if the last item is a leading byte, back the end up by one.
    FAIL_FAST_IF(lineEnd <= lineBegin); // We should have at least 1 space we can back up.

    // Get the last cell in the run and if it's a leading byte, move the end position back one so we don't
    // try to insert it.
    const auto lastCell = lineEnd - 1;
    if (lastCell->DbcsAttr().IsLeading())
    {
        lineEnd--;
    }

    // Copy out the substring into a vector.
    const std::vector<OutputCell> lineVec(lineBegin, lineEnd);

    // Add a conversion area to the internal state to hold this line.
    THROW_IF_FAILED(_AddConversionArea());

    // Get the added conversion area.
    auto& area = ConvAreaCompStr.back();

    // Write our text into the conversion area.
    area.ScreenBuffer->WriteLine(lineVec, 0, insertionPos.X);

    // Set the viewport and positioning parameters for the conversion area to describe to the renderer
    // the appropriate location to overlay this conversion area on top of the main screen buffer inside the viewport.
    const SMALL_RECT region{ insertionPos.X, 0, gsl::narrow<SHORT>(insertionPos.X + lineVec.size() - 1), 0 };
    area.SetWindowInfo(region);
    area.SetViewPos({ 0 - view.Left, insertionPos.Y - view.Top });

    // Make it visible and paint it.
    area.SetHidden(false);
    area.Paint();

    // Notify accessibility that we have updated the text in this display region within the viewport.
    screenInfo.NotifyAccessibilityEventing(insertionPos.X, insertionPos.Y, gsl::narrow<SHORT>(insertionPos.X + lineVec.size() - 1), insertionPos.Y);

    // Hand back the iterator representing the end of what we used to be fed into the beginning of the next call.
    return lineEnd;
}

// Routine Description:
// - Takes information from the IME message to write the "undetermined" text to the 
//   conversion area overlays on the screen.
// - The "undetermined" text represents the word or phrase that the user is currently building
//   using the IME. They haven't "determined" what they want yet, so it's "undetermined" right now.
// Arguments:
// - text - View into the text characters provided by the IME.
// - attributes - Attributes specifying which color and cursor positioning information should apply to 
//                each text character. This view must be the same size as the text view.
// - colorArray - 8 colors to be used to format the text for display
void ConsoleImeInfo::_WriteUndeterminedChars(const std::wstring_view text,
                                             const std::basic_string_view<BYTE> attributes,
                                             const std::basic_string_view<WORD> colorArray)
{
    CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    SCREEN_INFORMATION& screenInfo = gci.GetActiveOutputBuffer();

    // Ensure cursor is visible for prompt line
    screenInfo.MakeCurrentCursorVisible();

    // Clear out existing conversion areas.
    ConvAreaCompStr.clear();

    // If the text length and attribute length don't match,
    // it's a programming error on our part. We control the sizes here.
    FAIL_FAST_IF(text.size() != attributes.size());

    // If we have no text, return. We've already cleared above.
    if (text.size() < 1)
    {
        return;
    }

    // Convert data-to-be-stored into OutputCells.
    const auto cells = s_ConvertToCells(text, attributes, colorArray);

    // Get some starting position information of where to place the conversion areas on top of the existing 
    // screen buffer and viewport positioning.
    // Each conversion area write will adjust these to set up any subsequent calls to go onto the next line.
    auto pos = screenInfo.GetTextBuffer().GetCursor().GetPosition();
    const auto view = screenInfo.GetBufferViewport();
    // Set cursor position relative to viewport

    // Set up our iterators. We will walk through the entire set of cells from beginning to end.
    // The first time, we will give the iterators as the whole span and the begin
    // will be moved forward by the conversion area write to set up the next call.
    auto begin = cells.cbegin();
    const auto end = cells.cend();

    // Write over and over updating the beginning iterator until we reach the end.
    do
    {
        begin = _WriteConversionArea(begin, end, pos, view, screenInfo);
    } while (begin < end);
}
