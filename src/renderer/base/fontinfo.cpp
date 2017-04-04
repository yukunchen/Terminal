/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "..\inc\FontInfo.hpp"

#define DEFAULT_TT_FONT_FACENAME L"__DefaultTTFont__"
#define DEFAULT_RASTER_FONT_FACENAME L"Terminal"

FontInfoBase::FontInfoBase(_In_ PCWSTR const pwszFaceName,
                           _In_ BYTE const bFamily,
                           _In_ LONG const lWeight,
                           _In_ bool const fSetDefaultRasterFont,
                           _In_ UINT const uiCodePage) :
                           _bFamily(bFamily),
                           _lWeight(lWeight),
                           _fDefaultRasterSetFromEngine(fSetDefaultRasterFont),
                           _uiCodePage(uiCodePage)
{
    if (nullptr != pwszFaceName)
    {
        wcscpy_s(_wszFaceName, ARRAYSIZE(_wszFaceName), pwszFaceName);
    }

    ValidateFont();
}

FontInfoBase::FontInfoBase(_In_ const FontInfoBase &fibFont) :
    FontInfoBase(fibFont.GetFaceName(),
                 fibFont.GetFamily(),
                 fibFont.GetWeight(),
                 fibFont.WasDefaultRasterSetFromEngine(),
                 fibFont.GetCodePage())
{
}

FontInfo::FontInfo(_In_ PCWSTR const pwszFaceName,
                   _In_ BYTE const bFamily,
                   _In_ LONG const lWeight,
                   _In_ COORD const coordSize,
                   _In_ UINT const uiCodePage,
                   _In_ bool const fSetDefaultRasterFont /*= false*/) :
                   FontInfoBase(pwszFaceName, bFamily, lWeight, fSetDefaultRasterFont, uiCodePage),
                   _coordSize(coordSize),
                   _coordSizeUnscaled(coordSize)
{
    ValidateFont();
}

FontInfo::FontInfo(_In_ const FontInfo& fiFont) :
                   FontInfoBase(fiFont),
                   _coordSize(fiFont.GetSize()),
                   _coordSizeUnscaled(fiFont.GetUnscaledSize())
{

}

FontInfoBase::~FontInfoBase()
{
}

BYTE FontInfoBase::GetFamily() const
{
    return _bFamily;
}

// When the default raster font is forced set from the engine, this is how we differentiate it from a simple apply.
// Default raster font is internally represented as a blank face name and zeros for weight, family, and size. This is
// the hint for the engine to use whatever comes back from GetStockObject(OEM_FIXED_FONT) (at least in the GDI world).
bool FontInfoBase::WasDefaultRasterSetFromEngine() const
{
    return _fDefaultRasterSetFromEngine;
}

COORD FontInfo::GetUnscaledSize() const
{
    return _coordSizeUnscaled;
}

COORD FontInfoDesired::GetEngineSize() const
{
    COORD coordSize = _coordSizeDesired;
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

LONG FontInfoBase::GetWeight() const
{
    return _lWeight;
}

PCWCHAR FontInfoBase::GetFaceName() const
{
    return (PCWCHAR)_wszFaceName;
}

UINT FontInfoBase::GetCodePage() const
{
    return _uiCodePage;
}

BYTE FontInfoBase::GetCharSet() const
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
void FontInfoBase::SetFromEngine(_In_ PCWSTR const pwszFaceName,
                                 _In_ BYTE const bFamily,
                                 _In_ LONG const lWeight,
                                 _In_ bool const fSetDefaultRasterFont)
{
    wcscpy_s(_wszFaceName, ARRAYSIZE(_wszFaceName), pwszFaceName);
    _bFamily = bFamily;
    _lWeight = lWeight;
    _fDefaultRasterSetFromEngine = fSetDefaultRasterFont;
}

void FontInfo::SetFromEngine(_In_ PCWSTR const pwszFaceName,
                             _In_ BYTE const bFamily,
                             _In_ LONG const lWeight,
                             _In_ bool const fSetDefaultRasterFont,
                             _In_ COORD const coordSize,
                             _In_ COORD const coordSizeUnscaled)
{
    FontInfoBase::SetFromEngine(pwszFaceName,
                                bFamily,
                                lWeight,
                                fSetDefaultRasterFont);

    _coordSize = coordSize;
    _coordSizeUnscaled = coordSizeUnscaled;

    _ValidateCoordSize();
}

// Internally, default raster font is represented by empty facename, and zeros for weight, family, and size. Since
// FontInfoBase doesn't have sizing information, this helper checks everything else.
bool FontInfoBase::IsDefaultRasterFontNoSize() const
{
    return (_lWeight == 0 && _bFamily == 0 && wcsnlen_s(_wszFaceName, ARRAYSIZE(_wszFaceName)) == 0);
}

void FontInfoBase::ValidateFont()
{
    // If we were given a blank name, it meant raster fonts, which to us is always Terminal.
    if (!IsDefaultRasterFontNoSize() && s_pFontDefaultList != nullptr)
    {
        // If we have a list of default fonts and our current font is the placeholder for the defaults, substitute here.
        if (0 == wcsncmp(_wszFaceName, DEFAULT_TT_FONT_FACENAME, ARRAYSIZE(DEFAULT_TT_FONT_FACENAME)))
        {
            WCHAR pwszDefaultFontFace[LF_FACESIZE];
            if (SUCCEEDED(s_pFontDefaultList->RetrieveDefaultFontNameForCodepage(GetCodePage(),
                                                                                 pwszDefaultFontFace,
                                                                                 ARRAYSIZE(pwszDefaultFontFace))))
            {
                wcscpy_s(_wszFaceName, ARRAYSIZE(_wszFaceName), pwszDefaultFontFace);
            }
        }
    }
}

void FontInfo::ValidateFont()
{
    _ValidateCoordSize();
}

void FontInfo::_ValidateCoordSize()
{
    // a (0,0) font is okay for the default raster font, as we will eventually set the dimensions based on the font GDI
    // passes back to us.
    if (!IsDefaultRasterFontNoSize())
    {
        // Initialize X to 1 so we don't divide by 0
        if (_coordSize.X == 0)
        {
            _coordSize.X = 1;
        }

        // If we have no font size, we want to use 8x12 by default
        if (_coordSize.Y == 0)
        {
            _coordSize.X = 8;
            _coordSize.Y = 12;

            _coordSizeUnscaled = _coordSize;
        }
    }
}

bool FontInfoBase::IsTrueTypeFont() const
{
    return (_bFamily & TMPF_TRUETYPE) != 0;
}

#pragma warning(suppress:4356)
Microsoft::Console::Render::IFontDefaultList* FontInfo::s_pFontDefaultList;

void FontInfoBase::s_SetFontDefaultList(_In_ Microsoft::Console::Render::IFontDefaultList* const pFontDefaultList)
{
    s_pFontDefaultList = pFontDefaultList;
}

void FontInfo::s_SetFontDefaultList(_In_ Microsoft::Console::Render::IFontDefaultList* const pFontDefaultList)
{
    FontInfoBase::s_SetFontDefaultList(pFontDefaultList);
}

FontInfoDesired::FontInfoDesired(_In_ PCWSTR const pwszFaceName,
                                 _In_ BYTE const bFamily,
                                 _In_ LONG const lWeight,
                                 _In_ COORD const coordSizeDesired,
                                 _In_ UINT const uiCodePage) :
                                 FontInfoBase(pwszFaceName, bFamily, lWeight, false, uiCodePage),
                                 _coordSizeDesired(coordSizeDesired)
{
}

FontInfoDesired::FontInfoDesired(_In_ const FontInfo& fiFont) :
                                 FontInfoBase(fiFont),
                                 _coordSizeDesired(fiFont.GetUnscaledSize())
{
}

// This helper determines if this object represents the default raster font. This can either be because internally we're
// using the empty facename and zeros for size, weight, and family, or it can be because we were given explicit
// dimensions from the engine that were the result of loading the default raster font. See GdiEngine::_GetProposedFont().
bool FontInfoDesired::IsDefaultRasterFont() const
{
    // Default raster font means no face name, no weight, no family, and (0,0) for both sizes
    return WasDefaultRasterSetFromEngine() || (IsDefaultRasterFontNoSize() &&
                                               _coordSizeDesired.X == 0 &&
                                               _coordSizeDesired.Y == 0);
}

