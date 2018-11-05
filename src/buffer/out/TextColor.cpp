/********************************************************
 *                                                       *
 *   Copyright (C) Microsoft. All rights reserved.       *
 *                                                       *
 ********************************************************/

#include "precomp.h"
#include "TextColor.h"

bool TextColor::IsLegacy() const noexcept
{
    return !(IsDefault() || IsRgb());
}

bool TextColor::IsDefault() const noexcept
{
    return _meta[IS_DEFAULT_BIT];
}

bool TextColor::IsRgb() const noexcept
{
    return _meta[IS_RGB_BIT];
}

void TextColor::SetColor(const COLORREF rgbColor)
{
    _meta = IS_RGB;
    _red = GetRValue(rgbColor);
    _green = GetGValue(rgbColor);
    _blue = GetBValue(rgbColor);
}

void TextColor::SetIndex(const BYTE index)
{
    _meta = IS_INDEX;
    _red = index;
}

void TextColor::SetDefault()
{
    _meta = IS_DEFAULT;
}

COLORREF TextColor::GetColor(std::basic_string_view<COLORREF> colorTable, const COLORREF defaultColor, bool isBold) const
{
    if (IsDefault())
    {
        return defaultColor;
    }
    else if (IsRgb())
    {
        return _GetRGB();
    }
    else
    {
        FAIL_FAST_IF(colorTable.size() < _red);
        // If the color is already bright (it's in index [8,15] or it's a
        //       256color value [16,255], then boldness does nothing.
        if (isBold && _red < 8)
        {
            FAIL_FAST_IF(colorTable.size() < 16);
            FAIL_FAST_IF(_red + 8 > colorTable.size());
            return colorTable[_red + 8];
        }
        else
        {
            return colorTable[_red];
        }
    }
}

COLORREF TextColor::_GetRGB() const
{
    return RGB(_red, _green, _blue);
}
