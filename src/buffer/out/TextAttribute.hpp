/*++
Copyright (c) Microsoft Corporation

Module Name:
- TextAttribute.hpp

Abstract:
- contains data structure for run-length-encoding of text attribute data

Author(s):
- Michael Niksa (miniksa) 10-Apr-2014
- Paul Campbell (paulcam) 10-Apr-2014

Revision History:
- From components of output.h/.c
  by Therese Stowell (ThereseS) 1990-1991
- Pulled into its own file from textBuffer.hpp/cpp (AustDi, 2017)
--*/

#pragma once

#ifdef UNIT_TESTING
#include "WexTestClass.h"
#endif

class TextAttribute final
{
public:
    constexpr TextAttribute() noexcept :
        _wAttrLegacy{ 0 },
        _fUseRgbColor{ false },
        _rgbForeground{ RGB(0, 0, 0) },
        _rgbBackground{ RGB(0, 0, 0) },
        _isBold{ false },
        _defaultFg{ false },
        _defaultBg{ false }
    {
    }

    constexpr TextAttribute(const WORD wLegacyAttr) noexcept :
        _wAttrLegacy{ wLegacyAttr },
        _fUseRgbColor{ false },
        _rgbForeground{ RGB(0, 0, 0) },
        _rgbBackground{ RGB(0, 0, 0) },
        _isBold{ false },
        _defaultFg{ false },
        _defaultBg{ false }
    {
        // If we're given lead/trailing byte information with the legacy color, strip it.
        WI_ClearAllFlags(_wAttrLegacy, COMMON_LVB_SBCSDBCS);
    }

    constexpr TextAttribute(const COLORREF rgbForeground, const COLORREF rgbBackground) noexcept :
        _wAttrLegacy{ 0 },
        _fUseRgbColor{ true },
        _rgbForeground{ rgbForeground },
        _rgbBackground{ rgbBackground },
        _isBold{ false },
        _defaultFg{ false },
        _defaultBg{ false }
    {
    }

    WORD GetLegacyAttributes() const noexcept;

    COLORREF CalculateRgbForeground() const;
    COLORREF CalculateRgbBackground() const;

    COLORREF GetRgbForeground() const;
    COLORREF GetRgbBackground() const;

    bool IsLeadingByte() const noexcept;
    bool IsTrailingByte() const noexcept;

    bool IsTopHorizontalDisplayed() const noexcept;
    bool IsBottomHorizontalDisplayed() const noexcept;
    bool IsLeftVerticalDisplayed() const noexcept;
    bool IsRightVerticalDisplayed() const noexcept;

    void SetLeftVerticalDisplayed(const bool isDisplayed) noexcept;
    void SetRightVerticalDisplayed(const bool isDisplayed) noexcept;

    void SetFromLegacy(const WORD wLegacy) noexcept;
    void SetMetaAttributes(const WORD wMeta) noexcept;

    void Embolden() noexcept;
    void Debolden() noexcept;

    void Invert() noexcept;

    friend constexpr bool operator==(const TextAttribute& a, const TextAttribute& b) noexcept;
    friend constexpr bool operator!=(const TextAttribute& a, const TextAttribute& b) noexcept;
    friend constexpr bool operator==(const TextAttribute& attr, const WORD& legacyAttr) noexcept;
    friend constexpr bool operator!=(const TextAttribute& attr, const WORD& legacyAttr) noexcept;
    friend constexpr bool operator==(const WORD& legacyAttr, const TextAttribute& attr) noexcept;
    friend constexpr bool operator!=(const WORD& legacyAttr, const TextAttribute& attr) noexcept;

    bool IsLegacy() const noexcept;
    bool IsBold() const noexcept;

    void SetForeground(const COLORREF rgbForeground);
    void SetBackground(const COLORREF rgbBackground);
    void SetColor(const COLORREF rgbColor, const bool fIsForeground);

    void SetDefaultForeground(const COLORREF rgbForeground, const WORD wAttrDefault) noexcept;
    void SetDefaultBackground(const COLORREF rgbBackground, const WORD wAttrDefault) noexcept;

    bool ForegroundIsDefault() const noexcept;
    bool BackgroundIsDefault() const noexcept;

private:
    COLORREF _GetRgbForeground() const;
    COLORREF _GetRgbBackground() const;

    bool _IsReverseVideo() const noexcept;

    void _SetBoldness(const bool isBold) noexcept;

    WORD _wAttrLegacy;
    bool _fUseRgbColor;
    COLORREF _rgbForeground;
    COLORREF _rgbBackground;
    bool _isBold;
    bool _defaultFg;
    bool _defaultBg;

#ifdef UNIT_TESTING
    friend class TextBufferTests;
#endif
};

enum class TextAttributeBehavior
{
    Stored, // use contained text attribute
    Default, // use default text attribute at time of object instantiation
    Current, // use text attribute of cell being written to
    StoredOnly, // only use the contained text attribute and skip the insertion of anything else
};


bool constexpr operator==(const TextAttribute& a, const TextAttribute& b) noexcept
{
    return a._wAttrLegacy == b._wAttrLegacy &&
           a._fUseRgbColor == b._fUseRgbColor &&
           a._rgbForeground == b._rgbForeground &&
           a._rgbBackground == b._rgbBackground &&
           a._defaultFg == b._defaultFg &&
           a._defaultBg == b._defaultBg &&
           a._isBold == b._isBold;
}

bool constexpr operator!=(const TextAttribute& a, const TextAttribute& b) noexcept
{
    return !(a == b);
}

bool constexpr operator==(const TextAttribute& attr, const WORD& legacyAttr) noexcept
{
    return attr._wAttrLegacy == legacyAttr && !attr._fUseRgbColor;
}

bool constexpr operator!=(const TextAttribute& attr, const WORD& legacyAttr) noexcept
{
    return !(attr == legacyAttr);
}

bool constexpr operator==(const WORD& legacyAttr, const TextAttribute& attr) noexcept
{
    return attr == legacyAttr;
}

bool constexpr operator!=(const WORD& legacyAttr, const TextAttribute& attr) noexcept
{
    return !(attr == legacyAttr);
}

#ifdef UNIT_TESTING

namespace WEX {
    namespace TestExecution {
        template<>
        class VerifyOutputTraits < TextAttribute >
        {
        public:
            static WEX::Common::NoThrowString ToString(const TextAttribute& attr)
            {
                return WEX::Common::NoThrowString().Format(
                    L"{IsLegacy:%d,GetLegacyAttributes:0x%02x,FG:0x%06x,BG:0x%06x,bold:%d,default:(%d,%d)}",
                    attr.IsLegacy(),
                    attr.GetLegacyAttributes(),
                    attr.CalculateRgbForeground(),
                    attr.CalculateRgbBackground(),
                    attr.IsBold(),
                    attr.ForegroundIsDefault(),
                    attr.BackgroundIsDefault()
                );
            }
        };
    }
}
#endif
