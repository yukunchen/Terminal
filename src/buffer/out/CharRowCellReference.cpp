/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"
#include "UnicodeStorage.hpp"
#include "CharRow.hpp"


// Routine Description:
// - assignment operator. will store extended glyph data in a separate storage location
// Arguments:
// - chars - the glyph data to store
void CharRowCellReference::operator=(const std::vector<wchar_t>& chars)
{
    THROW_HR_IF(E_INVALIDARG, chars.empty());
    if (chars.size() == 1)
    {
        _cellData().Char() = chars.front();
        _cellData().DbcsAttr().SetGlyphStored(false);
    }
    else
    {
        auto& storage = _parent.GetUnicodeStorage();
        const auto key = _parent.GetStorageKey(_index);
        storage.StoreGlyph(key, chars);
        _cellData().DbcsAttr().SetGlyphStored(true);
    }
}

// Routine Description:
// - implicit conversion to vector<wchar_t> operator.
// Return Value:
// - std::vector<wchar_t> of the glyph data in the referenced cell
CharRowCellReference::operator std::vector<wchar_t>() const
{
    return _glyphData();
}

// Routine Description:
// - The CharRowCell this object "references"
// Return Value:
// - ref to the CharRowCell
CharRowCell& CharRowCellReference::_cellData()
{
    return _parent._data.at(_index);
}

// Routine Description:
// - The CharRowCell this object "references"
// Return Value:
// - ref to the CharRowCell
const CharRowCell& CharRowCellReference::_cellData() const
{
    return _parent._data.at(_index);
}

// Routine Description:
// - the glyph data of the referenced cell
// Return Value:
// - the glyph data
std::vector<wchar_t> CharRowCellReference::_glyphData() const
{
    if (_cellData().DbcsAttr().IsGlyphStored())
    {
        return _parent.GetUnicodeStorage().GetText(_parent.GetStorageKey(_index));
    }
    else
    {
        return { _cellData().Char() };
    }
}

// Routine Description:
// - gets read-only iterator to the beginning of the glyph data
// Return Value:
// - iterator of the glyph data
CharRowCellReference::const_iterator CharRowCellReference::begin() const
{
    if (_cellData().DbcsAttr().IsGlyphStored())
    {
        return _parent.GetUnicodeStorage().GetText(_parent.GetStorageKey(_index)).data();
    }
    else
    {
        return &_cellData().Char();
    }
}

// Routine Description:
// - get read-only iterator to the end of the glyph data
// Return Value:
// - end iterator of the glyph data
CharRowCellReference::const_iterator CharRowCellReference::end() const
{
    if (_cellData().DbcsAttr().IsGlyphStored())
    {

        const auto& chars = _parent.GetUnicodeStorage().GetText(_parent.GetStorageKey(_index));
        return chars.data() + chars.size();
    }
    else
    {
        return &_cellData().Char() + 1;
    }
}

bool operator==(const CharRowCellReference& ref, const std::vector<wchar_t>& glyph)
{
    const DbcsAttribute& dbcsAttr = ref._cellData().DbcsAttr();
    if (glyph.size() == 1 && dbcsAttr.IsGlyphStored())
    {
        return false;
    }
    else if (glyph.size() > 1 && !dbcsAttr.IsGlyphStored())
    {
        return false;
    }
    else if (glyph.size() == 1 && !dbcsAttr.IsGlyphStored())
    {
        return ref._cellData().Char() == glyph.front();
    }
    else
    {
        const auto& chars = ref._parent.GetUnicodeStorage().GetText(ref._parent.GetStorageKey(ref._index));
        return chars == glyph;
    }
}

bool operator==(const std::vector<wchar_t>& glyph, const CharRowCellReference& ref)
{
    return ref == glyph;
}
