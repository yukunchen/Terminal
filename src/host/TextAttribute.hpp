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
    TextAttribute(_In_ const WORD wLegacyAttr);
    TextAttribute(_In_ const COLORREF rgbForeground, _In_ const COLORREF rgbBackground);

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

    void SetFromLegacy(_In_ const WORD wLegacy);
    void SetMetaAttributes(_In_ const WORD wMeta);

    void SetFrom(_In_ const TextAttribute& otherAttr);
    bool IsEqual(_In_ const TextAttribute& otherAttr) const;
    bool IsEqualToLegacy(_In_ const WORD wLegacy) const;

    bool IsLegacy() const;

    void SetForeground(_In_ const COLORREF rgbForeground);
    void SetBackground(_In_ const COLORREF rgbBackground);
    void SetColor(_In_ const COLORREF rgbColor, _In_ const bool fIsForeground);

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
