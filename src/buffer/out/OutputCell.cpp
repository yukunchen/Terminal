/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "OutputCell.hpp"

#pragma warning(push)
#pragma warning(disable: ALL_CPPCORECHECK_WARNINGS)
#include "../interactivity/inc/ServiceLocator.hpp"
#pragma warning(pop)

#include "../../types/inc/convert.hpp"

OutputCell::OutputCell(const wchar_t charData,
                       const DbcsAttribute dbcsAttribute,
                       const TextAttribute textAttribute) noexcept :
    _charData{ charData },
    _dbcsAttribute{ dbcsAttribute },
    _textAttribute{ textAttribute }
{}

OutputCell::OutputCell(const std::vector<wchar_t>& charData,
                       const DbcsAttribute dbcsAttribute,
                       const TextAttribute textAttribute) :
    _charData{ charData },
    _dbcsAttribute{ dbcsAttribute },
    _textAttribute{ textAttribute }
{
    THROW_HR_IF(E_INVALIDARG, charData.empty());
}

void OutputCell::swap(_Inout_ OutputCell& other) noexcept
{
    using std::swap;
    swap(_charData, other._charData);
    swap(_dbcsAttribute, other._dbcsAttribute);
    swap(_textAttribute, other._textAttribute);
}

CHAR_INFO OutputCell::ToCharInfo()
{
    const CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    CHAR_INFO charInfo;
    charInfo.Char.UnicodeChar = Utf16ToUcs2(_charData);
    charInfo.Attributes = _dbcsAttribute.GeneratePublicApiAttributeFormat();
    charInfo.Attributes |= gci.GenerateLegacyAttributes(_textAttribute);
    return charInfo;
}

std::vector<wchar_t>& OutputCell::Chars() noexcept
{
    return _charData;
}

DbcsAttribute& OutputCell::DbcsAttr() noexcept
{
    return _dbcsAttribute;
}

TextAttribute& OutputCell::TextAttr() noexcept
{
    return _textAttribute;
}
