/*++
Copyright (c) Microsoft Corporation

Module Name:
- FontInfo.hpp

Abstract:
- This serves as the structure defining font information.  There are three
  relevant classes defined.

  1. FontInfoBase - the base class that holds the font's GDI's LOGFONT
     lfFaceName, lfWeight and lfPitchAndFamily, as well as the code page
     to use for WideCharToMultiByte and font name.

  2. FontInfo - derived from FontInfoBase.  It also has font size
     information - both the width and height of the requested font, as
     well as the measured height and width of L'0' from GDI.  All
     coordinates { X, Y } pair are non zero and always set to some
     reasonable value, even when GDI APIs fail.  This helps avoid
     divide by zero issues while performing various sizing
     calculations.

  3. FontInfoDesired - derived from FontInfoBase.  It also contains
     a desired size { X, Y }, to be supplied to the GDI's LOGFONT
     structure.  Unlike FontInfo, both desired X and Y can be zero.

Author(s):
- Michael Niksa (MiNiksa) 17-Nov-2015
--*/
#pragma once

#include "IFontDefaultList.hpp"

class FontInfoBase
{
public: 
    FontInfoBase(_In_ PCWSTR const pwszFaceName,
                 _In_ BYTE const bFamily,
                 _In_ LONG const lWeight,
                 _In_ bool const fSetDefaultRasterFont,
                 _In_ UINT const uiCodePage);

    FontInfoBase(_In_ const FontInfoBase &fibFont);

    ~FontInfoBase();

    BYTE GetFamily() const;
    LONG GetWeight() const;
    PCWCHAR GetFaceName() const;
    UINT GetCodePage() const;
    BYTE GetCharSet() const;
    bool IsTrueTypeFont() const;

    void SetFromEngine(_In_ PCWSTR const pwszFaceName,
                       _In_ BYTE const bFamily,
                       _In_ LONG const lWeight,
                       _In_ bool const fSetDefaultRasterFont);

    bool WasDefaultRasterSetFromEngine() const;
    void ValidateFont();

    static Microsoft::Console::Render::IFontDefaultList* s_pFontDefaultList;
    static void s_SetFontDefaultList(_In_ Microsoft::Console::Render::IFontDefaultList* const pFontDefaultList);

protected:
    bool IsDefaultRasterFontNoSize() const;

private:
    WCHAR _wszFaceName[LF_FACESIZE];
    LONG _lWeight;
    BYTE _bFamily;
    UINT _uiCodePage;
    bool _fDefaultRasterSetFromEngine;
};

class FontInfo : public FontInfoBase
{
public:
    FontInfo(_In_ PCWSTR const pwszFaceName,
             _In_ BYTE const bFamily,
             _In_ LONG const lWeight,
             _In_ COORD const coordSize,
             _In_ UINT const uiCodePage,
             _In_ bool const fSetDefaultRasterFont = false);

    FontInfo(_In_ const FontInfo &fiFont);

    COORD GetSize() const;
    COORD GetUnscaledSize() const;

    void SetFromEngine(_In_ PCWSTR const pwszFaceName,
                       _In_ BYTE const bFamily,
                       _In_ LONG const lWeight,
                       _In_ bool const fSetDefaultRasterFont,
                       _In_ COORD const coordSize,
                       _In_ COORD const coordSizeUnscaled);

    void ValidateFont();
    
    static void s_SetFontDefaultList(_In_ Microsoft::Console::Render::IFontDefaultList* const pFontDefaultList);

private:
    void _ValidateCoordSize();

    COORD _coordSize;
    COORD _coordSizeUnscaled;
};

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

private:
    COORD _coordSizeDesired;
};

// SET AND UNSET CONSOLE_OEMFONT_DISPLAY unless we can get rid of the stupid recoding in the conhost side.
