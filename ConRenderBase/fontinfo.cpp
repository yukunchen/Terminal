/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "..\inc\render\FontInfo.hpp"

#define DEFAULT_TT_FONT_FACENAME L"__DefaultTTFont__"
#define DEFAULT_RASTER_FONT_FACENAME L"Terminal"

FontInfo::FontInfo(_In_ PCWSTR const pwszFaceName,
                   _In_ BYTE const bFamily,
                   _In_ LONG const lWeight,
                   _In_ COORD const coordSize,
                   _In_ UINT const uiCodePage) :
                   _bFamily(bFamily),
                   _lWeight(lWeight),
                   _uiCodePage(uiCodePage),
                   _coordSize(coordSize),
                   _coordSizeUnscaled(coordSize)
{
    wcscpy_s(_pwszFaceName, ARRAYSIZE(_pwszFaceName), pwszFaceName);

    ValidateFont();
}

FontInfo::~FontInfo()
{
}

BYTE FontInfo::GetFamily() const
{
    return _bFamily;
}

COORD FontInfo::GetUnscaledSize() const
{
    return _coordSizeUnscaled;
}

COORD FontInfo::GetEngineSize() const
{
    COORD coordSize = _coordSizeUnscaled;
    if (IsTrueTypeFont())
    {
        coordSize.X = 0; // Don't tell the engine about the width for a TrueType font. It makes a mess.
    }

    return coordSize;
}

COORD FontInfo::GetSize() const
{
    return _coordSize;
}

LONG FontInfo::GetWeight() const
{
    return _lWeight;
}

PCWCHAR FontInfo::GetFaceName() const
{
    return (PCWCHAR)_pwszFaceName;
}

UINT FontInfo::GetCodePage() const
{
    return _uiCodePage;
}

BYTE FontInfo::GetCharSet() const
{
    CHARSETINFO csi;

    if (!TranslateCharsetInfo((DWORD *)IntToPtr(_uiCodePage), &csi, TCI_SRCCODEPAGE))
    {
        // if we failed to translate from codepage to charset, choose our charset depending on what kind of font we're
        // dealing with. Raster Fonts need to be presented with the OEM charset, while TT fonts need to be ANSI.
        csi.ciCharset = (((_bFamily) & TMPF_TRUETYPE) == TMPF_TRUETYPE) ? ANSI_CHARSET : OEM_CHARSET;
    }

    return (BYTE)csi.ciCharset;
}

// NOTE: this method is intended to only be used from the engine itself to respond what font it has chosen.
void FontInfo::SetFromEngine(_In_ PCWSTR const pwszFaceName,
                             _In_ BYTE const bFamily,
                             _In_ LONG const lWeight,
                             _In_ COORD const coordSize,
                             _In_ COORD const coordSizeUnscaled)
{
    wcscpy_s(_pwszFaceName, ARRAYSIZE(_pwszFaceName), pwszFaceName);
    _bFamily = bFamily;
    _lWeight = lWeight;
    _coordSize = coordSize;
    _coordSizeUnscaled = coordSizeUnscaled;
}

void FontInfo::ValidateFont()
{
    // If we were given a blank name, it meant raster fonts, which to us is always Terminal.
    if (wcsnlen_s(_pwszFaceName, ARRAYSIZE(_pwszFaceName)) == 0)
    {
        wcscpy_s(_pwszFaceName, ARRAYSIZE(_pwszFaceName), DEFAULT_RASTER_FONT_FACENAME);
    }
    else if (s_pFontDefaultList != nullptr)
    {
        // If we have a list of default fonts and our current font is the placeholder for the defaults, substitute here.
        if (0 == wcsncmp(_pwszFaceName, DEFAULT_TT_FONT_FACENAME, ARRAYSIZE(DEFAULT_TT_FONT_FACENAME)))
        {
            WCHAR pwszDefaultFontFace[LF_FACESIZE];
            if (SUCCEEDED(s_pFontDefaultList->RetrieveDefaultFontNameForCodepage(GetCodePage(), pwszDefaultFontFace, ARRAYSIZE(pwszDefaultFontFace))))
            {
                wcscpy_s(_pwszFaceName, ARRAYSIZE(_pwszFaceName), pwszDefaultFontFace);
            }
        }
    }

    // If we have no font size, we want to use 8x12 by default
    if (_coordSize.Y == 0)
    {
        _coordSize.X = 8;
        _coordSize.Y = 12;

        _coordSizeUnscaled = _coordSize;
    }
}

bool FontInfo::IsTrueTypeFont() const
{
    return (_bFamily & TMPF_TRUETYPE) != 0;
}

Microsoft::Console::Render::IFontDefaultList* FontInfo::s_pFontDefaultList;

void FontInfo::s_SetFontDefaultList(_In_ Microsoft::Console::Render::IFontDefaultList* const pFontDefaultList)
{
    s_pFontDefaultList = pFontDefaultList;
}
