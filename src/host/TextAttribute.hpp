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
    TextAttribute();
    TextAttribute(const WORD wLegacyAttr);
    TextAttribute(const COLORREF rgbForeground, const COLORREF rgbBackground);

    WORD GetLegacyAttributes() const;

    COLORREF CalculateRgbForeground() const;
    COLORREF CalculateRgbBackground() const;

    COLORREF GetRgbForeground() const;
    COLORREF GetRgbBackground() const;

    bool IsLeadingByte() const;
    bool IsTrailingByte() const;

    bool IsTopHorizontalDisplayed() const;
    bool IsBottomHorizontalDisplayed() const;
    bool IsLeftVerticalDisplayed() const;
    bool IsRightVerticalDisplayed() const;

    void SetFromLegacy(const WORD wLegacy);
    void SetMetaAttributes(const WORD wMeta);

    void SetFrom(const TextAttribute& otherAttr);
    bool IsEqual(const TextAttribute& otherAttr) const;
    bool IsEqualToLegacy(const WORD wLegacy) const;

    bool IsLegacy() const;

    void SetForeground(const COLORREF rgbForeground);
    void SetBackground(const COLORREF rgbBackground);
    void SetColor(const COLORREF rgbColor, const bool fIsForeground);

private:
    COLORREF _GetRgbForeground() const;
    COLORREF _GetRgbBackground() const;

    bool _IsReverseVideo() const;

    WORD _wAttrLegacy;
    bool _fUseRgbColor;
    COLORREF _rgbForeground;
    COLORREF _rgbBackground;

#ifdef UNIT_TESTING
    friend class TextBufferTests;
#endif
};
