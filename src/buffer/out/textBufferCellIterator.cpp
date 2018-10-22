/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "textBufferCellIterator.hpp"

#include "CharRow.hpp"
#include "textBuffer.hpp"
#include "../types/inc/convert.hpp"
#include "../types/inc/viewport.hpp"

#include "../interactivity/inc/ServiceLocator.hpp"

#pragma hdrstop

using namespace Microsoft::Console::Types;

// Routine Description:
// - Creates a new read-only iterator to seek through text data stored within a screen buffer
// Arguments:
// - buffer - Text buffer to seek throught
// - pos - Starting position to retrieve text data from (within screen buffer bounds)
TextBufferCellIterator::TextBufferCellIterator(const TextBuffer& buffer, COORD pos) :
    TextBufferCellIterator(buffer, pos, buffer.GetSize())
{
}

// Routine Description:
// - Creates a new read-only iterator to seek through text data stored within a screen buffer
// Arguments:
// - buffer - Pointer to screen buffer to seek through
// - pos - Starting position to retrieve text data from (within screen buffer bounds)
// - limits - Viewport limits to restrict the iterator within the buffer bounds (smaller than the buffer itself)
TextBufferCellIterator::TextBufferCellIterator(const TextBuffer& buffer, COORD pos, const Viewport limits) :
    _buffer(buffer),
    _pos(pos),
    _pRow(s_GetRow(buffer, pos)),
    _bounds(limits),
    _exceeded(false),
    _attrIter(s_GetRow(buffer, pos)->GetAttrRow().cbegin())
{
    // Throw if the bounds rectangle is not limited to the inside of the given buffer.
    THROW_HR_IF(E_INVALIDARG, !buffer.GetSize().IsInBounds(limits));

    // Throw if the coordinate is not limited to the inside of the given buffer.
    THROW_HR_IF(E_INVALIDARG, !limits.IsInBounds(pos));

    _attrIter += pos.X;
}

TextBufferCellIterator::operator bool() const noexcept
{
    return !_exceeded && _bounds.IsInBounds(_pos);
}

bool TextBufferCellIterator::operator==(const TextBufferCellIterator& it) const noexcept
{
    return _pos == it._pos && &_buffer == &it._buffer;
}
bool TextBufferCellIterator::operator!=(const TextBufferCellIterator& it) const noexcept
{
    return !(*this == it);
}

TextBufferCellIterator& TextBufferCellIterator::operator+=(const ptrdiff_t& movement)
{
    ptrdiff_t move = movement;
    auto newPos = _pos;
    while (move > 0 && !_exceeded)
    {
        _exceeded = !_bounds.IncrementInBounds(newPos);
        move--;
    }
    while (move < 0 && !_exceeded)
    {
        _exceeded = !_bounds.DecrementInBounds(newPos);
        move++;
    }
    _SetPos(newPos);
    return (*this);
}

TextBufferCellIterator& TextBufferCellIterator::operator-=(const ptrdiff_t& movement)
{
    return this->operator+=(-movement);
}

TextBufferCellIterator& TextBufferCellIterator::operator++()
{
    return this->operator+=(1);
}

TextBufferCellIterator& TextBufferCellIterator::operator--()
{
    return this->operator-=(1);
}

TextBufferCellIterator TextBufferCellIterator::operator++(int)
{
    auto temp(*this);
    operator++();
    return temp;
}

TextBufferCellIterator TextBufferCellIterator::operator--(int)
{
    auto temp(*this);
    operator--();
    return temp;
}

TextBufferCellIterator TextBufferCellIterator::operator+(const ptrdiff_t& movement)
{
    auto temp(*this);
    temp += movement;
    return temp;
}

TextBufferCellIterator TextBufferCellIterator::operator-(const ptrdiff_t& movement)
{
    auto temp(*this);
    temp -= movement;
    return temp;
}

ptrdiff_t TextBufferCellIterator::operator-(const TextBufferCellIterator& it)
{
    return _bounds.CompareInBounds(_pos, it._pos);
}

void TextBufferCellIterator::_SetPos(const COORD newPos)
{
    if (newPos.Y != _pos.Y)
    {
        _pRow = s_GetRow(_buffer, newPos);
        _attrIter = _pRow->GetAttrRow().cbegin();
        _pos.X = 0;
    }

    if (newPos.X != _pos.X)
    {
        const ptrdiff_t diff = newPos.X - _pos.X;
        _attrIter += diff;
    }

    _pos = newPos;
}

// Routine Description:
// - Shortcut for pulling the row out of the text buffer embedded in the screen information.
//   We'll hold and cache this to improve performance over looking it up every time.
// Arguments:
// - psi - Screen information pointer to pull text buffer data from
// - pos - Position inside screen buffer bounds to retrieve row
// Return Value:
// - Pointer to the underlying CharRow structure
const ROW* TextBufferCellIterator::s_GetRow(const TextBuffer& buffer, const COORD pos)
{
    return &buffer.GetRowByOffset(pos.Y);
}

// Routine Description:
// - Converts the cell data into the legacy CHAR_INFO format for API usage.
// Arguments:
// - <none> - Uses current position
// Return Value:
// - CHAR_INFO representation of the cell data. Is lossy versus actual stored data.
const CHAR_INFO TextBufferCellIterator::AsCharInfo() const
{
    CHAR_INFO ci;
    ci.Char.UnicodeChar = Utf16ToUcs2(_pRow->GetCharRow().GlyphAt(_pos.X));

    const CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    ci.Attributes = _pRow->GetCharRow().DbcsAttrAt(_pos.X).GeneratePublicApiAttributeFormat();
    ci.Attributes |= gci.GenerateLegacyAttributes(_pRow->GetAttrRow().GetAttrByColumn(_pos.X));
    return ci;
}

const OutputCellView TextBufferCellIterator::operator*() const
{
    return OutputCellView(_pRow->GetCharRow().GlyphAt(_pos.X),
                          _pRow->GetCharRow().DbcsAttrAt(_pos.X),
                          *_attrIter,
                          TextAttributeBehavior::Stored);
}
