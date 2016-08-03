/*++
Copyright (c) Microsoft Corporation

Module Name:
- renderFontDefaults.hpp

Abstract:
- This provides the implementation of the interface that abstracts the lookup of default fonts from the actual rendering engine.

Author(s):
- Michael Niksa (miniksa) Mar 2016
--*/

#pragma once

#include "..\inc\render\IFontDefaultList.hpp"

using namespace Microsoft::Console::Render;

class RenderFontDefaults sealed : public IFontDefaultList
{
public:
    RenderFontDefaults();
    ~RenderFontDefaults();
    
    HRESULT RetrieveDefaultFontNameForCodepage(_In_ UINT const uiCodePage,
                                               _Out_writes_(cchFaceName) PWSTR pwszFaceName,
                                               _In_ size_t const cchFaceName);
};
