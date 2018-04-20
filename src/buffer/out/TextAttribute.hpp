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

class TextAttribute final
{
public:
    TextAttribute() noexcept;
    TextAttribute(const WORD wLegacyAttr) noexcept;
    TextAttribute(const COLORREF rgbForeground, const COLORREF rgbBackground) noexcept;

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

    void SetFromLegacy(const WORD wLegacy) noexcept;
    void SetMetaAttributes(const WORD wMeta) noexcept;

    void SetFrom(const TextAttribute& otherAttr) noexcept;
    bool IsEqual(const TextAttribute& otherAttr) const noexcept;
    bool IsEqualToLegacy(const WORD wLegacy) const noexcept;

    bool IsLegacy() const noexcept;

    void SetForeground(const COLORREF rgbForeground);
    void SetBackground(const COLORREF rgbBackground);
    void SetColor(const COLORREF rgbColor, const bool fIsForeground);

private:
    COLORREF _GetRgbForeground() const;
    COLORREF _GetRgbBackground() const;

    bool _IsReverseVideo() const noexcept;

    WORD _wAttrLegacy;
    bool _fUseRgbColor;
    COLORREF _rgbForeground;
    COLORREF _rgbBackground;

#ifdef UNIT_TESTING
    friend class TextBufferTests;
#endif
};
