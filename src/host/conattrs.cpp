/*++
Copyright (c) Microsoft Corporation

Module Name:
- conattrs.cpp

Abstract:
- Defines common operations on console attributes, especially in regards to 
    finding the nearest color from a color table.

Author(s):
- Mike Griese (migrie) 01-Sept-2017
--*/

#include "precomp.h"
#include "..\inc\conattrs.hpp"
#include <math.h>

struct _HSL
{
    double h, s, l;

    // constructs an HSL color from a RGB Color.
    _HSL(_In_ const COLORREF rgb)
    {
        const double r = (double) GetRValue(rgb);
        const double g = (double) GetGValue(rgb);
        const double b = (double) GetBValue(rgb);
        double min, max, diff, sum;

        max = max(max(r, g), b);
        min = min(min(r, g), b);

        diff = max - min;
        sum = max + min;
        // Luminence
        l = max / 255.0;

        // Saturation
        s = (max == 0) ? 0 : diff / max;

        //Hue
        double q = (diff == 0)? 0 : 60.0/diff;
        if (max == r)
        {
            h = (g < b)? ((360.0 + q * (g - b))/360.0) : ((q * (g - b))/360.0);
        }
        else if (max == g)
        {
            h = (120.0 + q * (b - r))/360.0;
        }
        else if (max == b)
        {
            h = (240.0 + q * (r - g))/360.0;
        }
        else
        {
            h = 0;
        }
    }
};

//Routine Description:
// Finds the "distance" between a given HSL color and an RGB color, using the HSL color space.
//   This function is designed such that the caller would convert one RGB color to HSL ahead of time,
//   then compare many RGB colors to that first color.
//Arguments:
// - phslColorA - a pointer to the first color, as a HSL color.
// - rgbColorB - The second color to compare, in RGB color.
// Return value:
// The "distance" between the two.
static double _FindDifference(_In_ const _HSL* const phslColorA, _In_ const COLORREF rgbColorB)
{
    const _HSL hslColorB = _HSL(rgbColorB);
    return sqrt( pow((hslColorB.h - phslColorA->h), 2) +
                 pow((hslColorB.s - phslColorA->s), 2) +
                 pow((hslColorB.l - phslColorA->l), 2) );
}

//Routine Description:
// For a given RGB color Color, finds the nearest color from the array ColorTable, and returns the index of that match.
//Arguments:
// - Color - The RGB color to fine the nearest color to.
// - ColorTable - The array of colors to find a nearest color from.
// - cColorTable - The number of elements in ColorTable
// Return value:
// The index in ColorTable of the nearest match to Color.
WORD FindNearestTableIndex(_In_ COLORREF const Color, _In_reads_(cColorTable) const COLORREF* const ColorTable, _In_ const WORD cColorTable)
{
    const _HSL hslColor = _HSL(Color);
    WORD closest = 0;
    double minDiff = _FindDifference(&hslColor, ColorTable[0]);
    for (WORD i = 1; i < cColorTable; i++)
    {
        double diff = _FindDifference(&hslColor, ColorTable[i]);
        if (diff < minDiff)
        {
            minDiff = diff;
            closest = i;
        }
    }
    return closest;
}

// Function Description:
// - Converts the value of a xterm color table index to the windows color table equivalent.
// Arguments:
// - xtermTableEntry: the xterm color table index
// Return Value:
// - The windows color table equivalent.
WORD XtermToWindowsIndex(_In_ const size_t xtermTableEntry)
{
    const bool fRed = IsFlagSet(xtermTableEntry, 0x01);
    const bool fGreen = IsFlagSet(xtermTableEntry, 0x02);
    const bool fBlue = IsFlagSet(xtermTableEntry, 0x04);
    const bool fBright = IsFlagSet(xtermTableEntry, 0x08);

    return (fRed ? 0x4 : 0x0) +
           (fGreen ? 0x2 : 0x0) +
           (fBlue ? 0x1 : 0x0) +
           (fBright ? 0x8 : 0x0);
}
