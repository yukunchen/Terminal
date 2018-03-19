/*++
Copyright (c) Microsoft Corporation

Module Name:
- IFontDefaultList.hpp

Abstract:
- This serves as an abstraction to retrieve a list of default preferred fonts that we should use if the user hasn't chosen one.

Author(s):
- Michael Niksa (MiNiksa) 14-Mar-2016
--*/
#pragma once

namespace Microsoft::Console::Render
{
    class IFontDefaultList
    {
    public:
        [[nodiscard]]
        virtual HRESULT RetrieveDefaultFontNameForCodepage(_In_ UINT const uiCodePage,
                                                           _Out_writes_(cchFaceName) PWSTR pwszFaceName,
                                                           _In_ size_t const cchFaceName) = 0;
    };
}
