/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"
#include "UnicodeStorage.hpp"
#include "CharRow.hpp"


void CharRowReference::operator=(const std::vector<wchar_t>& chars)
{
    THROW_HR_IF(E_INVALIDARG, chars.empty());
    if (chars.size() == 1)
    {
        _cellData().Char() = chars.front();
    }
    else
    {
        auto& storage = UnicodeStorage::GetInstance();
        auto key = storage.StoreText(chars);
        _cellData().Char() = key;
        _cellData().DbcsAttr().SetMapIndex(1);
    }
}

CharRowReference::operator std::vector<wchar_t>() const
{
    return _glyphData();
}

CharRowCell& CharRowReference::_cellData()
{
    return _parent._data.at(_index);
}

const CharRowCell& CharRowReference::_cellData() const
{
    return _parent._data.at(_index);
}

std::vector<wchar_t> CharRowReference::_glyphData() const
{
    if (_cellData().DbcsAttr().IsStored())
    {
        return UnicodeStorage::GetInstance().GetText(_cellData().Char());
    }
    else
    {
        return { _cellData().Char() };
    }
}

CharRowReference::const_iterator CharRowReference::begin() const
{
    if (_cellData().DbcsAttr().IsStored())
    {
        return UnicodeStorage::GetInstance().GetText(_cellData().Char()).data();
    }
    else
    {
        return &_cellData().Char();
    }
}

CharRowReference::const_iterator CharRowReference::end() const
{
    if (_cellData().DbcsAttr().IsStored())
    {

        const auto& chars = UnicodeStorage::GetInstance().GetText(_cellData().Char());
        return chars.data() + chars.size();
    }
    else
    {
        return &_cellData().Char() + 1;
    }
}

bool operator==(const CharRowReference& ref, const std::vector<wchar_t>& glyph)
{
    return ref._glyphData() == glyph;
}

bool operator==(const std::vector<wchar_t>& glyph, const CharRowReference& ref)
{
    return ref == glyph;
}
