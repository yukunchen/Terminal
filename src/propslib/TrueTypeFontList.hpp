/*++
Copyright (c) Microsoft Corporation

Module Name:
- TrueTypeFontList.hpp

Abstract:
- This module is used for managing the list of preferred TrueType fonts in the registry

Author(s):
- Michael Niksa (MiNiksa) 14-Mar-2016

--*/

#pragma once

#ifdef __cplusplus

class TrueTypeFontList
{
public:
    static SINGLE_LIST_ENTRY s_ttFontList;

    static NTSTATUS s_Initialize();
    static NTSTATUS s_Destroy();

    static LPTTFONTLIST s_SearchByName(_In_opt_ LPCWSTR pwszFace,
                                       _In_ BOOL fCodePage,
                                       _In_ UINT CodePage);
    
    static NTSTATUS s_SearchByCodePage(_In_ const UINT uiCodePage,
                                       _Out_writes_(cchFaceName) PWSTR pwszFaceName,
                                       _In_ const size_t cchFaceName);
};

#else // not __cplusplus

    // The following registry methods remain public for DBCS and EUDC lookups.

NTSTATUS TrueTypeFontListInitialize();
NTSTATUS TrueTypeFontListDestroy();
LPTTFONTLIST TrueTypeFontListSearchByName(_In_opt_ LPCWSTR pwszFace,
                                      _In_ BOOL fCodePage,
                                      _In_ UINT CodePage);
NTSTATUS TrueTypeFontListSearchByCodePage(_In_ const UINT uiCodePage,
                                          _Out_writes_(cchFaceName) PWSTR pwszFaceName,
                                          _In_ const size_t cchFaceName);
    
#endif
