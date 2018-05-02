/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "CharRowCell.hpp"
#include "unicode.hpp"
#include "UnicodeStorage.hpp"


// default glyph value, used for reseting the character data portion of a cell
static constexpr wchar_t DefaultValue = UNICODE_SPACE;

CharRowCell::CharRowCell() :
    _wch{ DefaultValue },
    _attr{}
{
}

CharRowCell::CharRowCell(const wchar_t wch, const DbcsAttribute attr) :
    _wch{ wch },
    _attr{ attr }
{
}

void CharRowCell::EraseChars()
{
    if (_attr.IsStored())
    {
        auto key = _attr.GetMapIndex();
        auto& storage = UnicodeStorage::GetInstance();
        storage.Erase(key);
        _attr.EraseMapIndex();
    }
    _wch = DefaultValue;
}

void CharRowCell::Reset()
{
    EraseChars();
    _attr.SetSingle();
}

bool CharRowCell::IsSpace() const noexcept
{
    return !_attr.IsStored() && _wch == UNICODE_SPACE;
}

DbcsAttribute& CharRowCell::DbcsAttr()
{
    return _attr;
}

const DbcsAttribute& CharRowCell::DbcsAttr() const
{
    return _attr;
}

wchar_t& CharRowCell::Char()
{
    return _wch;
}

const wchar_t& CharRowCell::Char() const
{
    return _wch;
}
