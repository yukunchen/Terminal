/*++
Copyright (c) Microsoft Corporation

Module Name:
- FontInfo.hpp

Abstract:
- This serves as the structure defining font information.

Author(s):
- Michael Niksa (MiNiksa) 17-Nov-2015
--*/
#pragma once

#include "IFontDefaultList.hpp"

class FontInfo sealed
{
public: 
    FontInfo(_In_ PCWSTR const pwszFaceName,
             _In_ BYTE const bFamily,
             _In_ LONG const lWeight,
             _In_ COORD const coordSize,
             _In_ UINT const uiCodePage);

    ~FontInfo();

    BYTE GetFamily() const;
    COORD GetUnscaledSize() const;
    COORD GetSize() const;
    LONG GetWeight() const;
    PCWCHAR GetFaceName() const;
    UINT GetCodePage() const;
    BYTE GetCharSet() const;
    bool IsTrueTypeFont() const;

    COORD GetEngineSize() const; 

    void SetFromEngine(_In_ PCWSTR const pwszFaceName,
                       _In_ BYTE const bFamily,
                       _In_ LONG const lWeight,
                       _In_ COORD const coordSize,
                       _In_ COORD const coordSizeUnscaled);

    void ValidateFont();

    static Microsoft::Console::Render::IFontDefaultList* s_pFontDefaultList;
    static void s_SetFontDefaultList(_In_ Microsoft::Console::Render::IFontDefaultList* const pFontDefaultList);

private:   
    void _ValidateCoordSize();

    WCHAR _pwszFaceName[LF_FACESIZE];
    LONG _lWeight;
    BYTE _bFamily;
    UINT _uiCodePage;
    COORD _coordSize;
    COORD _coordSizeUnscaled;
};

// SET AND UNSET CONSOLE_OEMFONT_DISPLAY unless we can get rid of the stupid recoding in the conhost side.