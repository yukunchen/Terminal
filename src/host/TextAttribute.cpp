/********************************************************
 *                                                       *
 *   Copyright (C) Microsoft. All rights reserved.       *
 *                                                       *
 ********************************************************/

#include "precomp.h"
#include "TextAttribute.hpp"

#include "../interactivity/inc/ServiceLocator.hpp"

TextAttribute::TextAttribute()
{
    _wAttrLegacy = 0;
    _fUseRgbColor = false;
    _rgbForeground = RGB(0, 0, 0);
    _rgbBackground = RGB(0, 0, 0);
}

TextAttribute::TextAttribute(_In_ const WORD wLegacyAttr)
{
    _wAttrLegacy = wLegacyAttr;
    _fUseRgbColor = false;
    _rgbForeground = RGB(0, 0, 0);
    _rgbBackground = RGB(0, 0, 0);
}

TextAttribute::TextAttribute(_In_ const COLORREF rgbForeground, _In_ const COLORREF rgbBackground)
{
    _wAttrLegacy = 0;
    _rgbForeground = rgbForeground;
    _rgbBackground = rgbBackground;
    _fUseRgbColor = true;
}

WORD TextAttribute::GetLegacyAttributes() const
{
    return _wAttrLegacy;
}

bool TextAttribute::IsLegacy() const
{
    return _fUseRgbColor == false;
}

// Arguments:
// - None
// Return Value:
// - color that should be displayed as the foreground color
COLORREF TextAttribute::CalculateRgbForeground() const
{
    return _IsReverseVideo() ? GetRgbBackground() : GetRgbForeground();
}

// Routine Description:
// - Calculates rgb background color based off of current color table and active modification attributes
// Arguments:
// - None
// Return Value:
// - color that should be displayed as the background color
COLORREF TextAttribute::CalculateRgbBackground() const
{
    return _IsReverseVideo() ? GetRgbForeground() : GetRgbBackground();
}

// Routine Description:
// - gets rgb foreground color, possibly based off of current color table. Does not take active modification
// attributes into account
// Arguments:
// - None
// Return Value:
// - color that is stored as the foreground color
COLORREF TextAttribute::GetRgbForeground() const
{
    const CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    COLORREF rgbColor;
    if (_fUseRgbColor)
    {
        rgbColor = _rgbForeground;
    }
    else
    {
        const byte iColorTableIndex = LOBYTE(_wAttrLegacy) & 0x0F;

        ASSERT(iColorTableIndex >= 0);
        ASSERT(iColorTableIndex < gci.GetColorTableSize());

        rgbColor = gci.GetColorTable()[iColorTableIndex];
    }
    return rgbColor;
}

// Routine Description:
// - gets rgb background color, possibly based off of current color table. Does not take active modification
// attributes into account
// Arguments:
// - None
// Return Value:
// - color that is stored as the background color
COLORREF TextAttribute::GetRgbBackground() const
{
    const CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    COLORREF rgbColor;
    if (_fUseRgbColor)
    {
        rgbColor = _rgbBackground;
    }
    else
    {
        const byte iColorTableIndex = (LOBYTE(_wAttrLegacy) & 0xF0) >> 4;

        ASSERT(iColorTableIndex >= 0);
        ASSERT(iColorTableIndex < gci.GetColorTableSize());

        rgbColor = gci.GetColorTable()[iColorTableIndex];
    }
    return rgbColor;
}

void TextAttribute::SetFrom(_In_ const TextAttribute& otherAttr)
{
    *this = otherAttr;
}

void TextAttribute::SetFromLegacy(_In_ const WORD wLegacy)
{
    _wAttrLegacy = wLegacy;
    _fUseRgbColor = false;
}

void TextAttribute::SetMetaAttributes(_In_ const WORD wMeta)
{
    UpdateFlagsInMask(_wAttrLegacy, META_ATTRS, wMeta);
}

void TextAttribute::SetForeground(_In_ const COLORREF rgbForeground)
{
    _rgbForeground = rgbForeground;
    if (!_fUseRgbColor)
    {
        _rgbBackground = GetRgbBackground();
    }
    _fUseRgbColor = true;
}

void TextAttribute::SetBackground(_In_ const COLORREF rgbBackground)
{
    _rgbBackground = rgbBackground;
    if (!_fUseRgbColor)
    {
        _rgbForeground = GetRgbForeground();
    }
    _fUseRgbColor = true;
}

void TextAttribute::SetColor(_In_ const COLORREF rgbColor, _In_ const bool fIsForeground)
{
    if (fIsForeground)
    {
        SetForeground(rgbColor);
    }
    else
    {
        SetBackground(rgbColor);
    }
}

bool TextAttribute::IsEqual(_In_ const TextAttribute& otherAttr) const
{
    return _wAttrLegacy == otherAttr._wAttrLegacy &&
           _fUseRgbColor == otherAttr._fUseRgbColor &&
           _rgbForeground == otherAttr._rgbForeground &&
           _rgbBackground == otherAttr._rgbBackground;
}

bool TextAttribute::IsEqualToLegacy(_In_ const WORD wLegacy) const
{
    return _wAttrLegacy == wLegacy && !_fUseRgbColor;
}

bool TextAttribute::_IsReverseVideo() const
{
    return IsFlagSet(_wAttrLegacy, COMMON_LVB_REVERSE_VIDEO);
}

bool TextAttribute::IsLeadingByte() const
{
    return IsFlagSet(_wAttrLegacy, COMMON_LVB_LEADING_BYTE);
}

bool TextAttribute::IsTrailingByte() const
{
    return IsFlagSet(_wAttrLegacy, COMMON_LVB_LEADING_BYTE);
}

bool TextAttribute::IsTopHorizontalDisplayed() const
{
    return IsFlagSet(_wAttrLegacy, COMMON_LVB_GRID_HORIZONTAL);
}

bool TextAttribute::IsBottomHorizontalDisplayed() const
{
    return IsFlagSet(_wAttrLegacy, COMMON_LVB_UNDERSCORE);
}

bool TextAttribute::IsLeftVerticalDisplayed() const
{
    return IsFlagSet(_wAttrLegacy, COMMON_LVB_GRID_LVERTICAL);
}

bool TextAttribute::IsRightVerticalDisplayed() const
{
    return IsFlagSet(_wAttrLegacy, COMMON_LVB_GRID_RVERTICAL);
}
