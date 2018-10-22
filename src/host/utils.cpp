/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/
#include "precomp.h"

#include "utils.hpp"

#include "..\interactivity\inc\ServiceLocator.hpp"

short CalcWindowSizeX(const SMALL_RECT& rect) noexcept
{
    return rect.Right - rect.Left + 1;
}

short CalcWindowSizeY(const SMALL_RECT& rect) noexcept
{
    return rect.Bottom - rect.Top + 1;
}

short CalcCursorYOffsetInPixels(const short sFontSizeY, const ULONG ulSize) noexcept
{
    // TODO: MSFT 10229700 - Note, we want to likely enforce that this isn't negative.
    // Pretty sure there's not a valid case for negative offsets here.
    return (short)((sFontSizeY)-(ulSize));
}

WORD ConvertStringToDec(_In_ PCWSTR pwchToConvert, _Out_opt_ PCWSTR * const ppwchEnd) noexcept
{
    WORD val = 0;

    while (*pwchToConvert != L'\0')
    {
        WCHAR ch = *pwchToConvert;
        if (L'0' <= ch && ch <= L'9')
        {
            val = (val * 10) + (ch - L'0');
        }
        else
        {
            break;
        }

        pwchToConvert++;
    }

    if (nullptr != ppwchEnd)
    {
        *ppwchEnd = pwchToConvert;
    }

    return val;
}

// Routine Description:
// - Compares two coordinate positions to determine whether they're the same, left, or right within the given buffer size
// Arguments:
// - bufferSize - The size of the buffer to use for measurements.
// - coordFirst - The first coordinate position
// - coordSecond - The second coordinate position
// Return Value:
// -  Negative if First is to the left of the Second.
// -  0 if First and Second are the same coordinate.
// -  Positive if First is to the right of the Second.
// -  This is so you can do s_CompareCoords(first, second) <= 0 for "first is left or the same as second".
//    (the < looks like a left arrow :D)
// -  The magnitude of the result is the distance between the two coordinates when typing characters into the buffer (left to right, top to bottom)
int Utils::s_CompareCoords(const COORD bufferSize, const COORD coordFirst, const COORD coordSecond) noexcept
{
    const short cRowWidth = bufferSize.X;

    // Assert that our coordinates are within the expected boundaries
    const short cRowHeight = bufferSize.Y;
    FAIL_FAST_IF(!(coordFirst.X >= 0 && coordFirst.X < cRowWidth));
    FAIL_FAST_IF(!(coordSecond.X >= 0 && coordSecond.X < cRowWidth));
    FAIL_FAST_IF(!(coordFirst.Y >= 0 && coordFirst.Y < cRowHeight));
    FAIL_FAST_IF(!(coordSecond.Y >= 0 && coordSecond.Y < cRowHeight));

    // First set the distance vertically
    //   If first is on row 4 and second is on row 6, first will be -2 rows behind second * an 80 character row would be -160.
    //   For the same row, it'll be 0 rows * 80 character width = 0 difference.
    int retVal = (coordFirst.Y - coordSecond.Y) * cRowWidth;

    // Now adjust for horizontal differences
    //   If first is in position 15 and second is in position 30, first is -15 left in relation to 30.
    retVal += (coordFirst.X - coordSecond.X);

    // Further notes:
    //   If we already moved behind one row, this will help correct for when first is right of second.
    //     For example, with row 4, col 79 and row 5, col 0 as first and second respectively, the distance is -1.
    //     Assume the row width is 80.
    //     Step one will set the retVal as -80 as first is one row behind the second.
    //     Step two will then see that first is 79 - 0 = +79 right of second and add 79
    //     The total is -80 + 79 = -1.
    return retVal;
}

// Routine Description:
// - Compares two coordinate positions to determine whether they're the same, left, or right
// Arguments:
// - coordFirst - The first coordinate position
// - coordSecond - The second coordinate position
// Return Value:
// -  Negative if First is to the left of the Second.
// -  0 if First and Second are the same coordinate.
// -  Positive if First is to the right of the Second.
// -  This is so you can do s_CompareCoords(first, second) <= 0 for "first is left or the same as second".
//    (the < looks like a left arrow :D)
// -  The magnitude of the result is the distance between the two coordinates when typing characters into the buffer (left to right, top to bottom)
int Utils::s_CompareCoords(const COORD coordFirst, const COORD coordSecond) noexcept
{
    const CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    // find the width of one row
    const COORD coordScreenBufferSize = gci.GetActiveOutputBuffer().GetScreenBufferSize();
    return s_CompareCoords(coordScreenBufferSize, coordFirst, coordSecond);
}

// Routine Description:
// - Finds the opposite corner given a rectangle and one of its corners.
// - For example, finds the bottom right corner given a rectangle and its top left corner.
// Arguments:
// - srRectangle - The rectangle to check
// - coordCorner - One of the corners of the given rectangle
// Return Value:
// - The opposite corner of the one given.
COORD Utils::s_GetOppositeCorner(const SMALL_RECT srRectangle, const COORD coordCorner) noexcept
{
    // Assert we were given coordinates that are indeed one of the corners of the rectangle.
    FAIL_FAST_IF(!(coordCorner.X == srRectangle.Left || coordCorner.X == srRectangle.Right));
    FAIL_FAST_IF(!(coordCorner.Y == srRectangle.Top || coordCorner.Y == srRectangle.Bottom));

    return { (srRectangle.Left == coordCorner.X) ? srRectangle.Right : srRectangle.Left,
             (srRectangle.Top == coordCorner.Y) ? srRectangle.Bottom : srRectangle.Top };
}
