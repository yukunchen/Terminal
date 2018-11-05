/********************************************************
 *                                                       *
 *   Copyright (C) Microsoft. All rights reserved.       *
 *                                                       *
 ********************************************************/

#include "precomp.h"
#include "TextAttribute.hpp"
#include "..\..\inc\conattrs.hpp"

bool TextAttribute::IsLegacy() const noexcept
{
    return _foreground.IsLegacy() && _background.IsLegacy();
}

// Arguments:
// - None
// Return Value:
// - color that should be displayed as the foreground color
COLORREF TextAttribute::CalculateRgbForeground(std::basic_string_view<COLORREF> colorTable,
                                               COLORREF defaultColor) const
{
    return _IsReverseVideo() ? GetRgbBackground(colorTable, defaultColor) : GetRgbForeground(colorTable, defaultColor);
}

// Routine Description:
// - Calculates rgb background color based off of current color table and active modification attributes
// Arguments:
// - None
// Return Value:
// - color that should be displayed as the background color
COLORREF TextAttribute::CalculateRgbBackground(std::basic_string_view<COLORREF> colorTable,
                                               COLORREF defaultColor) const
{
    return _IsReverseVideo() ? GetRgbForeground(colorTable, defaultColor) : GetRgbBackground(colorTable, defaultColor);
}

// Routine Description:
// - gets rgb foreground color, possibly based off of current color table. Does not take active modification
// attributes into account
// Arguments:
// - None
// Return Value:
// - color that is stored as the foreground color
COLORREF TextAttribute::GetRgbForeground(std::basic_string_view<COLORREF> colorTable,
                                         COLORREF defaultColor) const
{
    // TODO: GetRgbForeground should be affected by our boldness -
    // Bold should use the top 8 of the color table if we're a legacy attribute.
    return _foreground.GetColor(colorTable, defaultColor, _isBold);
    // COLORREF rgbColor{ 0 };
    // if (_fUseRgbColor)
    // {
    //     rgbColor = _rgbForeground;
    // }
    // else
    // {
    //     const byte iColorTableIndex = (LOBYTE(GetLegacyAttributes()) & FG_ATTRS);

    //     FAIL_FAST_IF(!(iColorTableIndex >= 0));
    //     FAIL_FAST_IF(!(iColorTableIndex < colorTable.size()));

    //     rgbColor = colorTable[iColorTableIndex];
    // }
    // return rgbColor;
}

// Routine Description:
// - gets rgb background color, possibly based off of current color table. Does not take active modification
// attributes into account
// Arguments:
// - None
// Return Value:
// - color that is stored as the background color
COLORREF TextAttribute::GetRgbBackground(std::basic_string_view<COLORREF> colorTable,
                                         COLORREF defaultColor) const
{
    return _background.GetColor(colorTable, defaultColor, false);
    // COLORREF rgbColor{ 0 };
    // if (_fUseRgbColor)
    // {
    //     rgbColor = _rgbBackground;
    // }
    // else
    // {
    //     const byte iColorTableIndex = (LOBYTE(_wAttrLegacy) & BG_ATTRS) >> 4;

    //     FAIL_FAST_IF(!(iColorTableIndex >= 0));
    //     FAIL_FAST_IF(!(iColorTableIndex < colorTable.size()));

    //     rgbColor = colorTable[iColorTableIndex];
    // }
    // return rgbColor;
}


void TextAttribute::SetMetaAttributes(const WORD wMeta) noexcept
{
    WI_UpdateFlagsInMask(_wAttrLegacy, META_ATTRS, wMeta);
}

void TextAttribute::SetForeground(const COLORREF rgbForeground)
{
    _foreground = TextColor(rgbForeground);
    // _rgbForeground = rgbForeground;
    // if (!_fUseRgbColor)
    // {
    //     // TODO: The other color should not be affected by a change in this onw
    //     // Leave the other color unchanged.
    //     // _rgbBackground = GetRgbBackground();
    // }
    // _fUseRgbColor = true;
}

void TextAttribute::SetBackground(const COLORREF rgbBackground)
{
    _background = TextColor(rgbBackground);
    // _rgbBackground = rgbBackground;
    // if (!_fUseRgbColor)
    // {
    //     // TODO: The other color should not be affected by a change in this onw
    //     // Leave the other color unchanged.
    //     // _rgbForeground = GetRgbForeground();
    // }
    // _fUseRgbColor = true;
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
    return WI_IsFlagSet(_wAttrLegacy, COMMON_LVB_REVERSE_VIDEO);
}

bool TextAttribute::IsLeadingByte() const noexcept
{
    return WI_IsFlagSet(_wAttrLegacy, COMMON_LVB_LEADING_BYTE);
}

bool TextAttribute::IsTrailingByte() const noexcept
{
    return WI_IsFlagSet(_wAttrLegacy, COMMON_LVB_LEADING_BYTE);
}

bool TextAttribute::IsTopHorizontalDisplayed() const noexcept
{
    return WI_IsFlagSet(_wAttrLegacy, COMMON_LVB_GRID_HORIZONTAL);
}

bool TextAttribute::IsBottomHorizontalDisplayed() const noexcept
{
    return WI_IsFlagSet(_wAttrLegacy, COMMON_LVB_UNDERSCORE);
}

bool TextAttribute::IsLeftVerticalDisplayed() const noexcept
{
    return WI_IsFlagSet(_wAttrLegacy, COMMON_LVB_GRID_LVERTICAL);
}

bool TextAttribute::IsRightVerticalDisplayed() const noexcept
{
    return WI_IsFlagSet(_wAttrLegacy, COMMON_LVB_GRID_RVERTICAL);
}

void TextAttribute::SetLeftVerticalDisplayed(const bool isDisplayed) noexcept
{
    WI_UpdateFlag(_wAttrLegacy, COMMON_LVB_GRID_LVERTICAL, isDisplayed);
}

void TextAttribute::SetRightVerticalDisplayed(const bool isDisplayed) noexcept
{
    WI_UpdateFlag(_wAttrLegacy, COMMON_LVB_GRID_RVERTICAL, isDisplayed);
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
    // TODO: We should mark the attributes as bold, but GetColor should be affected by our boldness.
    // const CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();

    // // If we're changing our boldness, and we're an RGB attr, check if our color
    // //      is a darkened/brightened version of a color table entry. If it is,
    // //      then we'll instead use the bright/dark version of that color table
    // //      value as our new RGB color.
    // if ((_isBold != isBold) && _fUseRgbColor)
    // {
    //     const auto table = gsl::make_span(gci.GetColorTable(), gci.GetColorTableSize());
    //     const size_t start = isBold ? 0 : gci.GetColorTableSize()/2;
    //     const size_t end = isBold ? gci.GetColorTableSize()/2 : gci.GetColorTableSize();
    //     const auto shift = FOREGROUND_INTENSITY * (isBold ? 1 : -1);
    //     for (size_t i = start; i < end; i++)
    //     {
    //         if (table[i] == _rgbForeground)
    //         {
    //             _rgbForeground = table[i + shift];
    //             break;
    //         }
    //     }
    // }
    _isBold = isBold;
}

void TextAttribute::SetDefaultForeground(const COLORREF /*rgbForeground*/, const WORD /*wAttrDefault*/) noexcept
{
    // TODO remove params from signature
    _foreground = TextColor();
    // if(rgbForeground == INVALID_COLOR)
    // {
    //     return;
    // }
    // WI_UpdateFlagsInMask(_wAttrLegacy, FG_ATTRS, wAttrDefault);
    // SetForeground(rgbForeground);
    // _defaultFg = true;
}

void TextAttribute::SetDefaultBackground(const COLORREF /*rgbBackground*/, const WORD /*wAttrDefault*/) noexcept
{
    // TODO remove params from signature
    _background = TextColor();
    // if(rgbBackground == INVALID_COLOR)
    // {
    //     return;
    // }
    // WI_UpdateFlagsInMask(_wAttrLegacy, BG_ATTRS, wAttrDefault);
    // SetBackground(rgbBackground);
    // _defaultBg = true;
}

// Method Description:
// - Returns true if this attribute indicates it's foreground is the "default"
//      foreground. It's _rgbForeground will contain the actual value of the
//      default foreground. If the default colors are ever changed, this method
//      should be used to identify attributes with the default fg value, and
//      update them accordingly.
// Arguments:
// - <none>
// Return Value:
// - true iff this attribute indicates it's the "default" foreground color.
bool TextAttribute::ForegroundIsDefault() const noexcept
{
    return _foreground.IsDefault();
}

// Method Description:
// - Returns true if this attribute indicates it's background is the "default"
//      background. It's _rgbBackground will contain the actual value of the
//      default background. If the default colors are ever changed, this method
//      should be used to identify attributes with the default bg value, and
//      update them accordingly.
// Arguments:
// - <none>
// Return Value:
// - true iff this attribute indicates it's the "default" background color.
bool TextAttribute::BackgroundIsDefault() const noexcept
{
    return _background.IsDefault();
}
