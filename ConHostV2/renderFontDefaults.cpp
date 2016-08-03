/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "renderFontDefaults.hpp"

#pragma hdrstop

RenderFontDefaults::RenderFontDefaults()
{
    TrueTypeFontList::s_Initialize();
}

RenderFontDefaults::~RenderFontDefaults()
{
    TrueTypeFontList::s_Destroy();
}

HRESULT RenderFontDefaults::RetrieveDefaultFontNameForCodepage(_In_ UINT const uiCodePage,
                                                               _Out_writes_(cchFaceName) PWSTR pwszFaceName,
                                                               _In_ size_t const cchFaceName)
{
    NTSTATUS status = TrueTypeFontList::s_SearchByCodePage(uiCodePage, pwszFaceName, cchFaceName);
    return HRESULT_FROM_NT(status);
}
