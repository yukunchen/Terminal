/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "renderer.hpp"

#pragma hdrstop

using namespace Microsoft::Console::Render;

// Routine Description:
// - Converts a coordinate into a 1x1 rectangle (with exclusive bottom/right) representing the same location.
// Arguments:
// - Coordinate
// Return Value:
// - Rectangle representing same 1x1 area as given coordinate.
SMALL_RECT Renderer::_RegionFromCoord(_In_ const COORD* const pcoord) const
{
    SMALL_RECT sr;
    sr.Left = pcoord->X;
    sr.Right = sr.Left + 1;
    sr.Top = pcoord->Y;
    sr.Bottom = sr.Top + 1;

    return sr;
}

// Routine Description:
// - Takes text attriute data (compressed 16 color data) and looks up the respective RGB color value in our color table.
// Arguments:
// - Word attribute data from the text buffer for a particular character.
// Return Value:
// - RGB color mapped through the console color table.
COLORREF Renderer::_ConvertAttrToRGB(_In_ const BYTE bAttr)
{
    #pragma prefast(suppress: __WARNING_ACCESSIBILITY_COLORAPI, "Using console window specific colors")
    COLORREF* rgColorTable;
    size_t cColors;

    // Retrieve color table data.
    _pData->GetColorTable(&rgColorTable, &cColors);

    // Mask attributes down to the last nibble (0-15).
    size_t iColor = bAttr & 0x0F;

    // Ensure color requested is within the table (count - 1 is the final index of the table)
    iColor = min(iColor, cColors - 1);

    return rgColorTable[iColor];
}
