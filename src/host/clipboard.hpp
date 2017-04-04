/*++
Copyright (c) Microsoft Corporation

Module Name:
- clipboard.hpp

Abstract:
- This module is used for clipboard operations

Author(s):
- Michael Niksa (MiNiksa) 10-Apr-2014
- Paul Campbell (PaulCam) 10-Apr-2014

Revision History:
- From components of clipbrd.h/.c
--*/

#pragma once

#include "precomp.h"

// TODO: Static methods generally mean they're getting their state globally and not from this object _yet_
class Clipboard
{
public:
    static void s_StoreSelectionToClipboard();

    // legacy methods (not refactored yet)
    static void s_DoCopy();
    static void s_DoMark(); // TODO: could be eliminated, just calls a Selection method.
    static void s_DoStringPaste(_In_reads_(cchData) PCWCHAR pwchData, _In_ const size_t cchData);
    static void s_DoPaste();
    // end legacy methods

private:
    _Check_return_
    static NTSTATUS s_RetrieveTextFromBuffer(_In_ SCREEN_INFORMATION* const pScreenInfo,
                                             _In_ bool const fLineSelection,
                                             _In_ UINT const cRectsSelected,
                                             _In_reads_(cRectsSelected) const SMALL_RECT* const rgsrSelection,
                                             _Out_writes_(cRectsSelected) PWCHAR* const rgpwszTempText,
                                             _Out_writes_(cRectsSelected) size_t* const rgTempTextLengths);

    static NTSTATUS s_CopyTextToSystemClipboard(_In_ const UINT cTotalRows,
                                                _In_reads_(cTotalRows) const PWCHAR* const rgTempRows,
                                                _In_reads_(cTotalRows) const size_t* const rgTempRowLengths);

    static bool s_FilterCharacterOnPaste(_Inout_ WCHAR * const pwch);

#ifdef UNIT_TESTING
    friend class ClipboardTests;
#endif
};
