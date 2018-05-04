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

#include "..\..\host\screenInfo.hpp"

namespace Microsoft::Console::Interactivity::Win32
{
    class Clipboard
    {
    public:
        static Clipboard& Instance();

        void Copy();
        void StringPaste(_In_reads_(cchData) PCWCHAR pwchData,
                         const size_t cchData);
        void Paste();
    private:
        std::deque<std::unique_ptr<IInputEvent>> TextToKeyEvents(_In_reads_(cchData) const wchar_t* const pData,
                                                                 const size_t cchData);

        void StoreSelectionToClipboard();

        std::vector<std::wstring> RetrieveTextFromBuffer(const SCREEN_INFORMATION& screenInfo,
                                                         const bool lineSelection,
                                                         const std::vector<SMALL_RECT>& selectionRects);

        void CopyTextToSystemClipboard(const std::vector<std::wstring>& rows);

        bool FilterCharacterOnPaste(_Inout_ WCHAR * const pwch);

#ifdef UNIT_TESTING
        friend class ClipboardTests;
#endif
    };
}
