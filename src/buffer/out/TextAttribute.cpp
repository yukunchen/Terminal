/********************************************************
 *                                                       *
 *   Copyright (C) Microsoft. All rights reserved.       *
 *                                                       *
 ********************************************************/

#include "precomp.h"
#include "TextAttribute.hpp"

#pragma warning(push)
#pragma warning(disable: ALL_CPPCORECHECK_WARNINGS)
#include "../interactivity/inc/ServiceLocator.hpp"
#pragma warning(pop)


WORD TextAttribute::GetLegacyAttributes() const noexcept
{
    return _wAttrLegacy;
}

bool TextAttribute::IsLegacy() const noexcept
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
    COLORREF rgbColor{ 0 };
    if (_fUseRgbColor)
    {
        rgbColor = _rgbForeground;
    }
    else
    {
        const byte iColorTableIndex = LOBYTE(_wAttrLegacy) & 0x0F;

        FAIL_FAST_IF_FALSE(iColorTableIndex >= 0);
        FAIL_FAST_IF_FALSE(iColorTableIndex < gci.GetColorTableSize());

        const auto table = gsl::make_span(gci.GetColorTable(), gci.GetColorTableSize());
        rgbColor = table[iColorTableIndex];
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
    COLORREF rgbColor{ 0 };
    if (_fUseRgbColor)
    {
        rgbColor = _rgbBackground;
    }
    else
    {
        const byte iColorTableIndex = (LOBYTE(_wAttrLegacy) & 0xF0) >> 4;

        FAIL_FAST_IF_FALSE(iColorTableIndex >= 0);
        FAIL_FAST_IF_FALSE(iColorTableIndex < gci.GetColorTableSize());

        const auto table = gsl::make_span(gci.GetColorTable(), gci.GetColorTableSize());
        rgbColor = table[iColorTableIndex];
    }
    return rgbColor;
}

void TextAttribute::SetFromLegacy(const WORD wLegacy) noexcept
{
    _wAttrLegacy = wLegacy;
    _fUseRgbColor = false;
}

void TextAttribute::SetMetaAttributes(const WORD wMeta) noexcept
{
    UpdateFlagsInMask(_wAttrLegacy, META_ATTRS, wMeta);
}

void TextAttribute::SetForeground(const COLORREF rgbForeground)
{
    _rgbForeground = rgbForeground;
    if (!_fUseRgbColor)
    {
        _rgbBackground = GetRgbBackground();
    }
    _fUseRgbColor = true;
}

void TextAttribute::SetBackground(const COLORREF rgbBackground)
{
    _rgbBackground = rgbBackground;
    if (!_fUseRgbColor)
    {
        _rgbForeground = GetRgbForeground();
    }
    _fUseRgbColor = true;
}

void TextAttribute::SetColor(const COLORREF rgbColor, const bool fIsForeground)
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

bool TextAttribute::_IsReverseVideo() const noexcept
{
    return IsFlagSet(_wAttrLegacy, COMMON_LVB_REVERSE_VIDEO);
}

bool TextAttribute::IsLeadingByte() const noexcept
{
    return IsFlagSet(_wAttrLegacy, COMMON_LVB_LEADING_BYTE);
}

bool TextAttribute::IsTrailingByte() const noexcept
{
    return IsFlagSet(_wAttrLegacy, COMMON_LVB_LEADING_BYTE);
}

bool TextAttribute::IsTopHorizontalDisplayed() const noexcept
{
    return IsFlagSet(_wAttrLegacy, COMMON_LVB_GRID_HORIZONTAL);
}

bool TextAttribute::IsBottomHorizontalDisplayed() const noexcept
{
    return IsFlagSet(_wAttrLegacy, COMMON_LVB_UNDERSCORE);
}

bool TextAttribute::IsLeftVerticalDisplayed() const noexcept
{
    return IsFlagSet(_wAttrLegacy, COMMON_LVB_GRID_LVERTICAL);
}

bool TextAttribute::IsRightVerticalDisplayed() const noexcept
{
    return IsFlagSet(_wAttrLegacy, COMMON_LVB_GRID_RVERTICAL);
}
