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

class SCREEN_INFORMATION;

// Internal structures and definitions used by the conversion area.
class ConversionAreaBufferInfo
{
public:
    COORD coordCaBuffer;
    SMALL_RECT rcViewCaWindow;
    COORD coordConView;

    ConversionAreaBufferInfo(const COORD coordBufferSize);
};

class ConversionAreaInfo final
{
public:
    ConversionAreaInfo(const COORD bufferSize,
                       const COORD windowSize,
                       const CHAR_INFO fill,
                       const CHAR_INFO popupFill,
                       const FontInfo fontInfo);
    ~ConversionAreaInfo();
    ConversionAreaInfo(const ConversionAreaInfo&) = delete;
    ConversionAreaInfo(ConversionAreaInfo&& other);
    ConversionAreaInfo& operator=(const ConversionAreaInfo&) & = delete;
    ConversionAreaInfo& operator=(ConversionAreaInfo&&) & = delete;

    bool IsHidden() const;
    void SetHidden(const bool fIsHidden);
    void ClearArea();
    HRESULT Resize(const COORD newSize);

    void SetViewPos(const COORD pos);
    void SetWindowInfo(const SMALL_RECT view);
    void Paint() const;

    ConversionAreaBufferInfo CaInfo;
    SCREEN_INFORMATION* ScreenBuffer;

private:
    bool _fIsHidden;
};

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

    void RefreshAreaAttributes();
    void ClearAllAreas();
    HRESULT ResizeAllAreas(const COORD newSize);

    void WriteCompMessage(const LPCONIME_UICOMPMESSAGE msg);

    [[nodiscard]]
    HRESULT AddConversionArea();

private:
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
