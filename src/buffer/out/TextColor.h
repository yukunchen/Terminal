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
#include <bitset>
#ifdef UNIT_TESTING
#include "WexTestClass.h"
#endif

class TextColor final
{
public:
    static const unsigned int IS_INDEX = 0x0;
    static const unsigned int IS_DEFAULT = 0x1;
    static const unsigned int IS_RGB = 0x2;

    constexpr TextColor() noexcept :
        _meta{ IS_DEFAULT },
        _red{ 0 },
        _green{ 0 },
        _blue{ 0 }
    {
    }

    constexpr TextColor(const BYTE wLegacyAttr) noexcept :
        _meta{ IS_INDEX },
        _red{ wLegacyAttr },
        _green{ 0 },
        _blue{ 0 }
    {
    }

    constexpr TextColor(const COLORREF rgb) noexcept :
        _meta{ IS_RGB },
        _red{ GetRValue(rgb) },
        _green{ GetGValue(rgb) },
        _blue{ GetBValue(rgb) }
    {
    }

    friend constexpr bool operator==(const TextColor& a, const TextColor& b) noexcept;
    friend constexpr bool operator!=(const TextColor& a, const TextColor& b) noexcept;
    bool IsLegacy() const noexcept;
    bool IsDefault() const noexcept;
    bool IsRgb() const noexcept;
    void SetColor(const COLORREF rgbColor);
    void SetIndex(const BYTE index);
    void SetDefault();

    COLORREF GetColor(std::basic_string_view<COLORREF> colorTable, const COLORREF defaultColor, const bool isBold) const;

    // BYTE GetIndex() const;
    constexpr BYTE TextColor::GetIndex() const
    {
        // TODO: maybe take a defaultIndex param that we return if we're a default attr?
        // TODO: maybe make a version that does the HSL nearest color lookup?
        return _red;
    }


private:
    const static unsigned int IS_DEFAULT_BIT = 0;
    const static unsigned int IS_RGB_BIT = 1;
    std::bitset<2> _meta;
    BYTE _red;
    BYTE _green;
    BYTE _blue;
    COLORREF _GetRGB() const;

#ifdef UNIT_TESTING
    friend class TextBufferTests;
    template<typename TextColor> friend class WEX::TestExecution::VerifyOutputTraits;
#endif
};


bool constexpr operator==(const TextColor& a, const TextColor& b) noexcept
{
    return a._meta[0] == b._meta[0] &&
           a._meta[1] == b._meta[1] &&
           a._red == b._red &&
           a._green == b._green &&
           a._blue == b._blue;
}

bool constexpr operator!=(const TextColor& a, const TextColor& b) noexcept
{
    return !(a == b);
}

#ifdef UNIT_TESTING

namespace WEX {
    namespace TestExecution {
        template<>
        class VerifyOutputTraits < TextColor >
        {
        public:
            static WEX::Common::NoThrowString ToString(const TextColor& color)
            {
                if (color.IsDefault())
                {
                    return L"<default>";
                }
                else if (color.IsRgb())
                {
                    return WEX::Common::NoThrowString().Format(L"RGB:0x%06x", color._GetRGB());
                }
                else
                {
                    return WEX::Common::NoThrowString().Format(L"index:0x%04x", color._red);
                }
            }
        };
    }
}
#endif
