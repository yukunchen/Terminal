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
#include "../../inc/conattrs.hpp"

using namespace Microsoft::Console::Interactivity;

static constexpr TextAttribute InvalidTextAttribute{ INVALID_COLOR, INVALID_COLOR };

OutputCell::OutputCell(const std::vector<wchar_t>& charData,
                       const DbcsAttribute dbcsAttribute,
                       const TextAttributeBehavior behavior) :
    _charData{ charData },
    _dbcsAttribute{ dbcsAttribute },
    _textAttribute{ InvalidTextAttribute },
    _behavior{ behavior }
{
    THROW_HR_IF(E_INVALIDARG, charData.empty());
    _setFromBehavior(behavior);
}

OutputCell::OutputCell(const std::vector<wchar_t>& charData,
                       const DbcsAttribute dbcsAttribute,
                       const TextAttribute textAttribute) :
    _charData{ charData },
    _dbcsAttribute{ dbcsAttribute },
    _textAttribute{ textAttribute },
    _behavior{ TextAttributeBehavior::Stored }
{
    THROW_HR_IF(E_INVALIDARG, charData.empty());
}

OutputCell::OutputCell(const CHAR_INFO& charInfo) :
    _dbcsAttribute{},
    _textAttribute{ InvalidTextAttribute }
{
    _setFromCharInfo(charInfo);
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
    if (_behavior == TextAttributeBehavior::Current)
    {
        throw InvalidCharInfoConversionException();
    }
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

TextAttribute& OutputCell::TextAttr()
{
    THROW_HR_IF(E_INVALIDARG, _behavior == TextAttributeBehavior::Current);
    return _textAttribute;
}

void OutputCell::_setFromBehavior(const TextAttributeBehavior behavior)
{
    THROW_HR_IF(E_INVALIDARG, behavior == TextAttributeBehavior::Stored);
    if (behavior == TextAttributeBehavior::Default)
    {
        const auto& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
        _textAttribute = gci.GetActiveOutputBuffer().GetAttributes();
    }
}

void OutputCell::_setFromCharInfo(const CHAR_INFO& charInfo)
{
    _charData = { charInfo.Char.UnicodeChar };

    if (IsFlagSet(charInfo.Attributes, COMMON_LVB_LEADING_BYTE))
    {
        _dbcsAttribute.SetLeading();
    }
    else if (IsFlagSet(charInfo.Attributes, COMMON_LVB_TRAILING_BYTE))
    {
        _dbcsAttribute.SetTrailing();
    }
    _textAttribute.SetFromLegacy(charInfo.Attributes);

}
