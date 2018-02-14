/********************************************************
 *                                                       *
 *   Copyright (C) Microsoft. All rights reserved.       *
 *                                                       *
 ********************************************************/

#include "precomp.h"
#include "Utf8CharRow.hpp"

// Routine Description:
// - swaps two Utf8CharRows
// Arguments:
// - a - the first Utf8CharRow to swap
// - b - the second Utf8CharRow to swap
// Return Value:
// - <none>
void swap(Utf8CharRow& a, Utf8CharRow& b) noexcept
{
    a.swap(b);
}

// Routine Description:
// - constructor
// Arguments:
// - rowWidth - the size (in wchar_t) of the char and attribute rows
// Return Value:
// - instantiated object
// Note: will through if unable to allocate char/attribute buffers
Utf8CharRow::Utf8CharRow(short rowWidth) :
    CharRowBase(static_cast<size_t>(rowWidth), { UNICODE_SPACE })
{
}

// Routine Description:
// - copy constructor
// Arguments:
// - Utf8CharRow to copy
// Return Value:
// - instantiated object
// Note: will through if unable to allocate char/attribute buffers
Utf8CharRow::Utf8CharRow(const Utf8CharRow& a) :
    CharRowBase(a)
{
}

// Routine Description:
// - assignment operator
// Arguments:
// - Utf8CharRow to copy
// Return Value:
// - reference to this object
Utf8CharRow& Utf8CharRow::operator=(const Utf8CharRow& a)
{
    Utf8CharRow temp(a);
    this->swap(temp);
    return *this;
}

// Routine Description:
// - swaps values with another Utf8CharRow
// Arguments:
// - other - the Utf8CharRow to swap with
// Return Value:
// - <none>
void Utf8CharRow::swap(Utf8CharRow& other) noexcept
{
    // this looks kinda weird, but we want the compiler to be able to choose between std::swap and a
    // specialized swap, so we include both in the same namespace and let it sort it out.
    using std::swap;
    swap(static_cast<CharRowBase&>(*this), static_cast<CharRowBase&>(other));
    swap(_data, other._data);
}
