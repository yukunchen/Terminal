/*++
Copyright (c) Microsoft Corporation

Module Name:
- conimeinfo.h

Abstract:
- This module contains the structures for the console IME

Author:
- KazuM

Revision History:
--*/

#pragma once

#include "../inc/conime.h"
#include "../buffer/out/OutputCell.hpp"
#include "../buffer/out/TextAttribute.hpp"
#include "../renderer/inc/FontInfo.hpp"

#include "conareainfo.h"

class SCREEN_INFORMATION;

class ConsoleImeInfo
{
public:
    // Composition String information
    LPCONIME_UICOMPMESSAGE CompStrData;
    bool SavedCursorVisible; // whether cursor is visible (set by user)

    // IME compositon string information
    // There is one "composition string" per line that must be rendered on the screen
    std::vector<ConversionAreaInfo> ConvAreaCompStr;

    ConsoleImeInfo();
    ~ConsoleImeInfo();
    ConsoleImeInfo(const ConsoleImeInfo&) = delete;
    ConsoleImeInfo(ConsoleImeInfo&&) = delete;
    ConsoleImeInfo& operator=(const ConsoleImeInfo&) & = delete;
    ConsoleImeInfo& operator=(ConsoleImeInfo&&) & = delete;

    void RefreshAreaAttributes();
    void ClearAllAreas();
    HRESULT ResizeAllAreas(const COORD newSize);

    void WriteCompMessage(const LPCONIME_UICOMPMESSAGE msg);

private:
    [[nodiscard]]
    HRESULT _AddConversionArea();

    void _WriteUndeterminedChars(const std::wstring_view text,
                                 const std::basic_string_view<BYTE> attributes,
                                 const std::basic_string_view<WORD> colorArray);

    static TextAttribute s_RetrieveAttributeAt(const size_t pos,
                                               const std::basic_string_view<BYTE> attributes,
                                               const std::basic_string_view<WORD> colorArray);

    static std::vector<OutputCell> s_ConvertToCells(const std::wstring_view text,
                                                    const std::basic_string_view<BYTE> attributes,
                                                    const std::basic_string_view<WORD> colorArray);

    std::vector<OutputCell>::const_iterator ConsoleImeInfo::_WriteConversionArea(const std::vector<OutputCell>::const_iterator begin,
                                                                                 const std::vector<OutputCell>::const_iterator end,
                                                                                 COORD& pos,
                                                                                 const SMALL_RECT view,
                                                                                 SCREEN_INFORMATION& screenInfo);
};
