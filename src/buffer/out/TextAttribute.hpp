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
#include "TextColor.h"
#include "../../inc/conattrs.hpp"

#ifdef UNIT_TESTING
#include "WexTestClass.h"
#endif

class TextAttribute final
{
public:
    constexpr TextAttribute() noexcept :
        _wAttrLegacy{ 0 },
        _foreground{},
        _background{},
        _isBold{ false }
    {
    }

    constexpr TextAttribute(const WORD wLegacyAttr) noexcept :
        _wAttrLegacy{ (WORD)(wLegacyAttr & META_ATTRS) },
        _foreground{ (BYTE)(wLegacyAttr & FG_ATTRS) },
        _background{ (BYTE)((wLegacyAttr & BG_ATTRS)>>4) },
        _isBold{ false }
    {
    }

    constexpr TextAttribute(const COLORREF rgbForeground, const COLORREF rgbBackground) noexcept :
        _wAttrLegacy{ 0 },
        _foreground{ rgbForeground },
        _background{ rgbBackground },
        _isBold{ false }
    {
    }

    constexpr WORD TextAttribute::GetLegacyAttributes() const noexcept
    {
        // TODO: Should this take a defaultLegacy value, if the Default is set for either fg/bg?
        // Maybe
        BYTE fg = (_foreground.GetIndex() & FG_ATTRS);
        BYTE bg = (_background.GetIndex() & BG_ATTRS) << 4;
        WORD meta = (_wAttrLegacy & META_ATTRS);
        return (fg | bg | meta) | (_isBold ? FOREGROUND_INTENSITY : 0);
    }

    COLORREF CalculateRgbForeground(std::basic_string_view<COLORREF> colorTable, COLORREF defaultColor) const;
    COLORREF CalculateRgbBackground(std::basic_string_view<COLORREF> colorTable, COLORREF defaultColor) const;

    COLORREF _GetRgbForeground(std::basic_string_view<COLORREF> colorTable, COLORREF defaultColor) const;
    COLORREF _GetRgbBackground(std::basic_string_view<COLORREF> colorTable, COLORREF defaultColor) const;

    bool IsLeadingByte() const noexcept;
    bool IsTrailingByte() const noexcept;
    bool IsTopHorizontalDisplayed() const noexcept;
    bool IsBottomHorizontalDisplayed() const noexcept;
    bool IsLeftVerticalDisplayed() const noexcept;
    bool IsRightVerticalDisplayed() const noexcept;

    void SetLeftVerticalDisplayed(const bool isDisplayed) noexcept;
    void SetRightVerticalDisplayed(const bool isDisplayed) noexcept;

    void SetFromLegacy(const WORD wLegacy) noexcept;

    void SetLegacyAttributes(const WORD attrs,
                             const bool setForeground,
                             const bool setBackground,
                             const bool setMeta);

    void SetMetaAttributes(const WORD wMeta) noexcept;

    void Embolden() noexcept;
    void Debolden() noexcept;

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

    void SetDefaultForeground() noexcept;
    void SetDefaultBackground() noexcept;

    bool ForegroundIsDefault() const noexcept;
    bool BackgroundIsDefault() const noexcept;

private:
    bool _IsReverseVideo() const noexcept;
    void _SetBoldness(const bool isBold) noexcept;

    WORD _wAttrLegacy;
    TextColor _foreground;
    TextColor _background;
    bool _isBold;

#ifdef UNIT_TESTING
    friend class TextBufferTests;
    template<typename TextAttribute> friend class WEX::TestExecution::VerifyOutputTraits;
#endif
};

enum class TextAttributeBehavior
{
    Stored, // use contained text attribute
    Default, // use default text attribute at time of object instantiation
    Current, // use text attribute of cell being written to
    StoredOnly, // only use the contained text attribute and skip the insertion of anything else
};


constexpr bool operator==(const TextAttribute& a, const TextAttribute& b) noexcept
{
    return a._wAttrLegacy == b._wAttrLegacy &&
           a._foreground == b._foreground &&
           a._background == b._background &&
           a._isBold == b._isBold;
}

constexpr bool operator!=(const TextAttribute& a, const TextAttribute& b) noexcept
{
    return !(a == b);
}

constexpr bool operator==(const TextAttribute& attr, const WORD& legacyAttr) noexcept
{
    // TODO: Does this still make sense?
    return attr.GetLegacyAttributes() == legacyAttr;
}

constexpr bool operator!=(const TextAttribute& attr, const WORD& legacyAttr) noexcept
{
    return !(attr == legacyAttr);
}

constexpr bool operator==(const WORD& legacyAttr, const TextAttribute& attr) noexcept
{
    return attr == legacyAttr;
}

constexpr bool operator!=(const WORD& legacyAttr, const TextAttribute& attr) noexcept
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
                    // L"{IsLegacy:%d,GetLegacyAttributes:0x%02x,FG:0x%06x,BG:0x%06x,bold:%d,default:(%d,%d)}",
                    L"{FG:%s,BG:%s,bold:%d,wLegacy:(0x%04x)}",
                    VerifyOutputTraits<TextColor>::ToString(attr._foreground).GetBuffer(),
                    VerifyOutputTraits<TextColor>::ToString(attr._background).GetBuffer(),
                    attr.IsBold(),
                    attr._wAttrLegacy
                );
            }
        };
    }
}
#endif
