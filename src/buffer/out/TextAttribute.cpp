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
    return (_wAttrLegacy | (_isBold ? FOREGROUND_INTENSITY : 0));
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
        const byte iColorTableIndex = (LOBYTE(GetLegacyAttributes()) & FG_ATTRS);

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
        const byte iColorTableIndex = (LOBYTE(_wAttrLegacy) & BG_ATTRS) >> 4;

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

bool TextAttribute::IsBold() const noexcept
{
    return _isBold;
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

void TextAttribute::SetLeftVerticalDisplayed(const bool isDisplayed) noexcept
{
    UpdateFlag(_wAttrLegacy, COMMON_LVB_GRID_LVERTICAL, isDisplayed);
}

void TextAttribute::SetRightVerticalDisplayed(const bool isDisplayed) noexcept
{
    UpdateFlag(_wAttrLegacy, COMMON_LVB_GRID_RVERTICAL, isDisplayed);
}

void TextAttribute::Embolden() noexcept
{
    _SetBoldness(true);
}

void TextAttribute::Debolden() noexcept
{
    _SetBoldness(false);
}

void TextAttribute::_SetBoldness(const bool isBold) noexcept
{
    const CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();

    // If we're changing our boldness, and we're an RGB attr, check if our color
    //      is a darkened/brightened version of a color table entry. If it is,
    //      then we'll instead use the bright/dark version of that color table
    //      value as our new RGB color.
    if ((_isBold != isBold) && _fUseRgbColor)
    {
        const auto table = gsl::make_span(gci.GetColorTable(), gci.GetColorTableSize());
        const size_t start = isBold ? 0 : gci.GetColorTableSize()/2;
        const size_t end = isBold ? gci.GetColorTableSize()/2 : gci.GetColorTableSize();
        const auto shift = FOREGROUND_INTENSITY * (isBold ? 1 : -1);
        for (size_t i = start; i < end; i++)
        {
            if (table[i] == _rgbForeground)
            {
                _rgbForeground = table[i + shift];
                break;
            }
        }
    }
    _isBold = isBold;
}
