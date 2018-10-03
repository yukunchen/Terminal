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
#include "../../host/dbcs.h"

using namespace Microsoft::Console::Interactivity;

bool operator==(const OutputCell& a, const OutputCell& b) noexcept
{
    return (a._charData == b._charData &&
            a._dbcsAttribute == b._dbcsAttribute &&
            a._textAttribute == b._textAttribute &&
            a._behavior == b._behavior);
}

static constexpr TextAttribute InvalidTextAttribute{ INVALID_COLOR, INVALID_COLOR };

std::vector<OutputCell> OutputCell::FromUtf16(const std::vector<std::vector<wchar_t>>& utf16Glyphs)
{
    return _fromUtf16(utf16Glyphs, { TextAttributeBehavior::Current });
}

std::vector<OutputCell> OutputCell::FromUtf16(const std::vector<std::vector<wchar_t>>& utf16Glyphs,
                                              const TextAttribute defaultTextAttribute)
{
    return _fromUtf16(utf16Glyphs, { defaultTextAttribute });
}

std::vector<OutputCell> OutputCell::_fromUtf16(const std::vector<std::vector<wchar_t>>& utf16Glyphs,
                                               const std::variant<TextAttribute, TextAttributeBehavior> textAttrVariant)
{
    std::vector<OutputCell> cells;

    auto constructorDispatch = [&](const std::wstring_view glyph, const DbcsAttribute dbcsAttr)
    {
        if (textAttrVariant.index() == 0)
        {
            cells.emplace_back(glyph, dbcsAttr, std::get<0>(textAttrVariant));
        }
        else
        {
            cells.emplace_back(glyph, dbcsAttr, std::get<1>(textAttrVariant));
        }
    };

    for (const auto glyph : utf16Glyphs)
    {
        DbcsAttribute dbcsAttr;
        const std::wstring_view glyphView{ glyph.data(), glyph.size() };
        if (IsGlyphFullWidth(glyphView))
        {
            dbcsAttr.SetLeading();
            constructorDispatch(glyphView, dbcsAttr);
            dbcsAttr.SetTrailing();
        }
        constructorDispatch(glyphView, dbcsAttr);
    }
    return cells;
}


OutputCell::OutputCell(const std::wstring_view charData,
                       const DbcsAttribute dbcsAttribute,
                       const TextAttributeBehavior behavior) :
    _charData{ charData.begin(), charData.end() },
    _dbcsAttribute{ dbcsAttribute },
    _textAttribute{ InvalidTextAttribute },
    _behavior{ behavior }
{
    THROW_HR_IF(E_INVALIDARG, charData.empty());
    _setFromBehavior(behavior);
}

OutputCell::OutputCell(const std::wstring_view charData,
                       const DbcsAttribute dbcsAttribute,
                       const TextAttribute textAttribute) :
    _charData{ charData.begin(), charData.end() },
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
    charInfo.Char.UnicodeChar = Utf16ToUcs2({ _charData.data(), _charData.size() });
    charInfo.Attributes = _dbcsAttribute.GeneratePublicApiAttributeFormat();
    charInfo.Attributes |= gci.GenerateLegacyAttributes(_textAttribute);
    return charInfo;
}

const std::wstring_view OutputCell::Chars() const noexcept
{
	return { _charData.data(), _charData.size() };
}

void OutputCell::SetChars(const std::wstring_view chars)
{
    _charData.assign(chars.cbegin(), chars.cend());
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

    if (WI_IsFlagSet(charInfo.Attributes, COMMON_LVB_LEADING_BYTE))
    {
        _dbcsAttribute.SetLeading();
    }
    else if (WI_IsFlagSet(charInfo.Attributes, COMMON_LVB_TRAILING_BYTE))
    {
        _dbcsAttribute.SetTrailing();
    }
    _textAttribute.SetFromLegacy(charInfo.Attributes);

    _behavior = TextAttributeBehavior::Stored;
}
