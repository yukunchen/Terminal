/********************************************************
 *                                                       *
 *   Copyright (C) Microsoft. All rights reserved.       *
 *                                                       *
 ********************************************************/

#include "precomp.h"
#include "Ucs2CharRow.hpp"

// Routine Description:
// - swaps two Ucs2CharRows
// Arguments:
// - a - the first Ucs2CharRow to swap
// - b - the second Ucs2CharRow to swap
// Return Value:
// - <none>
void swap(Ucs2CharRow& a, Ucs2CharRow& b) noexcept
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
Ucs2CharRow::Ucs2CharRow(short rowWidth) :
    CharRowBase(static_cast<size_t>(rowWidth), UNICODE_SPACE)
{
}

// Routine Description:
// - assignment operator
// Arguments:
// - Ucs2CharRow to copy
// Return Value:
// - reference to this object
Ucs2CharRow& Ucs2CharRow::operator=(const Ucs2CharRow& a)
{
    Ucs2CharRow temp(a);
    this->swap(temp);
    return *this;
}

// Routine Description:
// - swaps values with another Ucs2CharRow
// Arguments:
// - other - the Ucs2CharRow to swap with
// Return Value:
// - <none>
void Ucs2CharRow::swap(Ucs2CharRow& other) noexcept
{
    // this looks kinda weird, but we want the compiler to be able to choose between std::swap and a
    // specialized swap, so we include both in the same namespace and let it sort it out.
    using std::swap;
    swap(static_cast<CharRowBase&>(*this), static_cast<CharRowBase&>(other));
}

// Routine Description:
// - returns the all of the text in a row, including leading and trailing whitespace.
// Arguments:
// - <none>
// Return Value:
// - all text data in the row
Ucs2CharRow::string_type Ucs2CharRow::GetText() const
{
    std::wstring temp(_data.size(), UNICODE_SPACE);
    for (size_t i = 0; i < _data.size(); ++i)
    {
        temp[i] = _data[i].first;
    }
    return temp;
}
ICharRow::SupportedEncoding Ucs2CharRow::GetSupportedEncoding() const noexcept
{
    return ICharRow::SupportedEncoding::Ucs2;
}
