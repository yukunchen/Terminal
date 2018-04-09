/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "OutputCell.hpp"
#include "..\interactivity\inc\ServiceLocator.hpp"


OutputCell::OutputCell(const wchar_t charData,
                       const DbcsAttribute dbcsAttribute,
                       const TextAttribute textAttribute) noexcept :
    _charData{ charData },
    _dbcsAttribute{ dbcsAttribute },
    _textAttribute{ textAttribute }
{}

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
    charInfo.Char.UnicodeChar = _charData;
    charInfo.Attributes = _dbcsAttribute.GeneratePublicApiAttributeFormat();
    charInfo.Attributes |= gci.GenerateLegacyAttributes(_textAttribute);
    return charInfo;
}

wchar_t& OutputCell::GetCharData() noexcept
{
    return _charData;
}

DbcsAttribute& OutputCell::GetDbcsAttribute() noexcept
{
    return _dbcsAttribute;
}

TextAttribute& OutputCell::GetTextAttribute() noexcept
{
    return _textAttribute;
}
