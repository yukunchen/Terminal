/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "renderer.hpp"

#pragma hdrstop

using namespace Microsoft::Console::Render;
using namespace Microsoft::Console::Types;

// Routine Description:
// - Takes text attriute data (compressed 16 color data) and looks up the respective RGB color value in our color table.
// Arguments:
// - Word attribute data from the text buffer for a particular character.
// Return Value:
// - RGB color mapped through the console color table.
COLORREF Renderer::_ConvertAttrToRGB(const BYTE bAttr)
{
    COLORREF* rgColorTable;
    size_t cColors;

    // Retrieve color table data.
    _pData->GetColorTable(&rgColorTable, &cColors);

    // Mask attributes down to the last nibble (0-15).
    size_t iColor = bAttr & 0x0F;

    // Ensure color requested is within the table (count - 1 is the final index of the table)
    iColor = std::min(iColor, cColors - 1);

    return rgColorTable[iColor];
}
