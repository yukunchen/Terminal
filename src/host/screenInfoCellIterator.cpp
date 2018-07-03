/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "screenInfoCellIterator.hpp"

#include "misc.h"

#include "../buffer/out/CharRow.hpp"
#include "../interactivity/inc/ServiceLocator.hpp"
#include "../types/inc/convert.hpp"

#pragma hdrstop

using namespace Microsoft::Console::Interactivity;

// Routine Description:
// - Creates a new read-only iterator to seek through text data stored within a screen buffer
// Arguments:
// - psi - Pointer to screen buffer to seek through
// - pos - Starting position to retrieve text data from (within screen buffer bounds)
ScreenInfoCellIterator::ScreenInfoCellIterator(const SCREEN_INFORMATION& si, COORD pos) :
    ScreenInfoCellIterator(si, 
                           pos, 
                           si.GetScreenEdges())
{
}

// Routine Description:
// - Creates a new read-only iterator to seek through text data stored within a screen buffer
// Arguments:
// - psi - Pointer to screen buffer to seek through
// - pos - Starting position to retrieve text data from (within screen buffer bounds)
ScreenInfoCellIterator::ScreenInfoCellIterator(const SCREEN_INFORMATION& si, COORD pos, SMALL_RECT limits) :
    _si(si),
    _pos(pos),
    _pRow(s_GetRow(si, pos)),
    _bounds(limits),
    _exceeded(false)
{
    // Throw if the bounds rectangle is not limited to the inside of the given buffer.
    THROW_HR_IF(E_INVALIDARG, !IsRectInBoundsInclusive(limits, si.GetScreenEdges()));
}

ScreenInfoCellIterator::operator bool() const noexcept
{
    return !_exceeded && IsCoordInBoundsInclusive(_pos, _bounds);
}

bool ScreenInfoCellIterator::operator==(const ScreenInfoCellIterator& it) const noexcept
{
    return _pos == it._pos && &_si == &it._si;
}
bool ScreenInfoCellIterator::operator!=(const ScreenInfoCellIterator& it) const noexcept
{
    return !(*this == it);
}

ScreenInfoCellIterator& ScreenInfoCellIterator::operator+=(const ptrdiff_t& movement)
{
    ptrdiff_t move = movement;
    auto newPos = _pos;
    while (move > 0 && !_exceeded)
    {
        _exceeded = !Utils::s_DoIncrementScreenCoordinate(_bounds, &newPos);
        move--;
    }
    while (move < 0 && !_exceeded)
    {
        _exceeded = !Utils::s_DoDecrementScreenCoordinate(_bounds, &newPos);
        move++;
    }
    _SetPos(newPos);
    return (*this);
}

ScreenInfoCellIterator& ScreenInfoCellIterator::operator-=(const ptrdiff_t& movement)
{
    return this->operator+=(-movement);
}

ScreenInfoCellIterator& ScreenInfoCellIterator::operator++()
{
    return this->operator+=(1);
}

ScreenInfoCellIterator& ScreenInfoCellIterator::operator--()
{
    return this->operator-=(1);
}

ScreenInfoCellIterator ScreenInfoCellIterator::operator++(int)
{
    auto temp(*this);
    operator++();
    return temp;
}

ScreenInfoCellIterator ScreenInfoCellIterator::operator--(int)
{
    auto temp(*this);
    operator--();
    return temp;
}

ScreenInfoCellIterator ScreenInfoCellIterator::operator+(const ptrdiff_t& movement)
{
    auto temp(*this);
    temp += movement;
    return temp;
}

ScreenInfoCellIterator ScreenInfoCellIterator::operator-(const ptrdiff_t& movement)
{
    auto temp(*this);
    temp -= movement;
    return temp;
}

ptrdiff_t ScreenInfoCellIterator::operator-(const ScreenInfoCellIterator& it)
{
    return Utils::s_CompareCoords(_si.GetScreenBufferSize(), _pos, it._pos);
}

void ScreenInfoCellIterator::_SetPos(const COORD newPos)
{
    if (newPos.Y != _pos.Y)
    {
        _pRow = s_GetRow(_si, newPos);
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
const ROW* ScreenInfoCellIterator::s_GetRow(const SCREEN_INFORMATION& si, const COORD pos)
{
    return &static_cast<ROW&>((si._textBuffer->GetRowByOffset(pos.Y)));
}

const CHAR_INFO ScreenInfoCellIterator::operator*() const
{
    CHAR_INFO ci;
    ci.Char.UnicodeChar = Utf16ToUcs2(_pRow->GetCharRow().GlyphAt(_pos.X));
    
    const CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    ci.Attributes = _pRow->GetCharRow().DbcsAttrAt(_pos.X).GeneratePublicApiAttributeFormat();
    ci.Attributes |= gci.GenerateLegacyAttributes(_pRow->GetAttrRow().GetAttrByColumn(_pos.X));
    return ci;
}
