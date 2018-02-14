/********************************************************
 *                                                       *
 *   Copyright (C) Microsoft. All rights reserved.       *
 *                                                       *
 ********************************************************/

#include "precomp.h"
#include "CharRowBase.hpp"


// Routine Description:
// - swaps two CharRowBases
// Arguments:
// - a - the first CharRowBase to swap
// - b - the second CharRowBase to swap
// Return Value:
// - <none>
void swap(CharRowBase& a, CharRowBase& b) noexcept
{
    a.swap(b);
}

CharRowBase::CharRowBase() :
    _wrapForced{ false },
    _doubleBytePadded{ false }
{
}

// Routine Description:
// - swaps values with another CharRowBase
// Arguments:
// - other - the CharRowBase to swap with
// Return Value:
// - <none>
void CharRowBase::swap(CharRowBase& other) noexcept
{
    using std::swap;
    swap(_wrapForced, other._wrapForced);
    swap(_doubleBytePadded, other._doubleBytePadded);
}

// Routine Description:
// - Sets the wrap status for the current row
// Arguments:
// - wrapForced - True if the row ran out of space and we forced to wrap to the next row. False otherwise.
// Return Value:
// - <none>
void CharRowBase::SetWrapForced(_In_ bool const wrapForced) noexcept
{
    _wrapForced = wrapForced;
}

// Routine Description:
// - Gets the wrap status for the current row
// Arguments:
// - <none>
// Return Value:
// - True if the row ran out of space and we were forced to wrap to the next row. False otherwise.
bool CharRowBase::WasWrapForced() const noexcept
{
    return _wrapForced;
}

// Routine Description:
// - Sets the double byte padding for the current row
// Arguments:
// - fWrapWasForced - True if the row ran out of space for a double byte character and we padded out the row. False otherwise.
// Return Value:
// - <none>
void CharRowBase::SetDoubleBytePadded(_In_ bool const doubleBytePadded) noexcept
{
    _doubleBytePadded = doubleBytePadded;
}

// Routine Description:
// - Gets the double byte padding status for the current row.
// Arguments:
// - <none>
// Return Value:
// - True if the row didn't have space for a double byte character and we were padded out the row. False otherwise.
bool CharRowBase::WasDoubleBytePadded() const noexcept
{
    return _doubleBytePadded;
}
