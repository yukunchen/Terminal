/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "screenInfoTextIterator.hpp"

#include "misc.h"

#include "../buffer/out/CharRow.hpp"

#pragma hdrstop

// Routine Description:
// - Creates a new read-only iterator to seek through text data stored within a screen buffer
// Arguments:
// - psi - Pointer to screen buffer to seek through
// - pos - Starting position to retrieve text data from (within screen buffer bounds)
ScreenInfoTextIterator::ScreenInfoTextIterator(const SCREEN_INFORMATION* const psi, COORD pos) :
    _psi(THROW_HR_IF_NULL(E_INVALIDARG, psi)),
    _pos(pos),
    _pRow(s_GetRow(psi, pos))
{

}

ScreenInfoTextIterator::operator bool() const
{
    if (nullptr == _psi)
    {
        return false;
    }

    const auto bufferSize = _psi->GetScreenBufferSize();

    return IsCoordInBounds(_pos, bufferSize);
}

bool ScreenInfoTextIterator::operator==(const ScreenInfoTextIterator& it) const
{
    return _pos == it._pos && _psi == it._psi;
}
bool ScreenInfoTextIterator::operator!=(const ScreenInfoTextIterator& it) const
{
    return !(*this == it);
}

ScreenInfoTextIterator& ScreenInfoTextIterator::operator+=(const ptrdiff_t& movement)
{
    ptrdiff_t move = movement;
    const auto bufferSize = _psi->GetScreenBufferSize();
    auto newPos = _pos;
    while (move > 0)
    {
        Utils::s_IncrementCoordinate(bufferSize, newPos);
        move--;
    }
    while (move < 0)
    {
        Utils::s_DecrementCoordinate(bufferSize, newPos);
        move++;
    }
    _SetPos(newPos);
    return (*this);
}

ScreenInfoTextIterator& ScreenInfoTextIterator::operator-=(const ptrdiff_t& movement)
{
    return this->operator+=(-movement);
}

ScreenInfoTextIterator& ScreenInfoTextIterator::operator++()
{
    return this->operator+=(1);
}

ScreenInfoTextIterator& ScreenInfoTextIterator::operator--()
{
    return this->operator-=(1);
}

ScreenInfoTextIterator ScreenInfoTextIterator::operator++(int)
{
    auto temp(*this);
    operator++();
    return temp;
}

ScreenInfoTextIterator ScreenInfoTextIterator::operator--(int)
{
    auto temp(*this);
    operator--();
    return temp;
}

ScreenInfoTextIterator ScreenInfoTextIterator::operator+(const ptrdiff_t& movement)
{
    auto temp(*this);
    temp += movement;
    return temp;
}

ScreenInfoTextIterator ScreenInfoTextIterator::operator-(const ptrdiff_t& movement)
{
    auto temp(*this);
    temp -= movement;
    return temp;
}

ptrdiff_t ScreenInfoTextIterator::operator-(const ScreenInfoTextIterator& it)
{
    return Utils::s_CompareCoords(_psi->GetScreenBufferSize(), _pos, it._pos);
}

const wchar_t& ScreenInfoTextIterator::operator*()
{
    return _pRow->GlyphAt(_pos.X);
}

const wchar_t& ScreenInfoTextIterator::operator*() const
{
    return _pRow->GlyphAt(_pos.X);
}

const wchar_t* ScreenInfoTextIterator::operator->()
{
    return &_pRow->GlyphAt(_pos.X);
}

const wchar_t* ScreenInfoTextIterator::getPtr() const
{
    return &_pRow->GlyphAt(_pos.X);
}

const wchar_t* ScreenInfoTextIterator::getConstPtr() const
{
    return &_pRow->GlyphAt(_pos.X);
}

void ScreenInfoTextIterator::_SetPos(const COORD newPos)
{
    if (newPos.Y != _pos.Y)
    {
        _pRow = s_GetRow(_psi, newPos);
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
const CharRow* ScreenInfoTextIterator::s_GetRow(const SCREEN_INFORMATION* const psi, const COORD pos)
{
    THROW_HR_IF_NULL(E_INVALIDARG, psi);
    return &static_cast<CharRow&>((psi->_textBuffer->GetRowByOffset(pos.Y).GetCharRow()));
}

