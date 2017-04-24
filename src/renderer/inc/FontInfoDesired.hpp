/*++
Copyright (c) Microsoft Corporation

Module Name:
- FontInfo.hpp

Abstract:
- This serves as the structure defining font information.

- FontInfoDesired - derived from FontInfoBase.  It also contains
  a desired size { X, Y }, to be supplied to the GDI's LOGFONT
  structure.  Unlike FontInfo, both desired X and Y can be zero.

Author(s):
- Michael Niksa (MiNiksa) 17-Nov-2015
--*/

#pragma once

#include "FontInfoBase.hpp"
#include "FontInfo.hpp"


class FontInfoDesired : public FontInfoBase
{
public:
    FontInfoDesired(_In_ PCWSTR const pwszFaceName,
                    _In_ BYTE const bFamily,
                    _In_ LONG const lWeight,
                    _In_ COORD const coordSizeDesired,
                    _In_ UINT const uiCodePage);

    FontInfoDesired(_In_ const FontInfo &fiFont);

    COORD FontInfoDesired::GetEngineSize() const;
    bool IsDefaultRasterFont() const;

    friend bool operator==(const FontInfoDesired& a, const FontInfoDesired& b);

private:
    COORD _coordSizeDesired;
};

bool operator==(const FontInfoDesired& a, const FontInfoDesired& b);