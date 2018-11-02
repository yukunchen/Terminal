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

COLORREF TextColor::GetColor(gsl::span<COLORREF>& colorTable, COLORREF defaultColor) const
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
        return colorTable[_red];
    }
}

COLORREF TextColor::_GetRGB() const
{
    return RGB(_red, _green, _blue);
}
