/********************************************************
 *                                                       *
 *   Copyright (C) Microsoft. All rights reserved.       *
 *                                                       *
 ********************************************************/

#include "precomp.h"
#include "CharRow.hpp"

// Routine Description:
// - swaps two CHAR_ROWs
// Arguments:
// - a - the first CHAR_ROW to swap
// - b - the second CHAR_ROW to swap
// Return Value:
// - <none>
void swap(CHAR_ROW& a, CHAR_ROW& b) noexcept
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
CHAR_ROW::CHAR_ROW(short rowWidth) :
    _data(rowWidth, std::pair<wchar_t, DbcsAttribute>(UNICODE_SPACE, DbcsAttribute{}))
{
    SetWrapStatus(false);
    SetDoubleBytePadded(false);
}

// Routine Description:
// - copy constructor
// Arguments:
// - CHAR_ROW to copy
// Return Value:
// - instantiated object
// Note: will through if unable to allocate char/attribute buffers
CHAR_ROW::CHAR_ROW(const CHAR_ROW& a) :
    bRowFlags{ a.bRowFlags },
    _data{ a._data }
{
}

// Routine Description:
// - assignment operator
// Arguments:
// - CHAR_ROW to copy
// Return Value:
// - reference to this object
CHAR_ROW& CHAR_ROW::operator=(const CHAR_ROW& a)
{
    CHAR_ROW temp(a);
    this->swap(temp);
    return *this;
}

// Routine Description:
// - move constructor
// Arguments:
// - CHAR_ROW to move
// Return Value:
// - instantiated object
CHAR_ROW::CHAR_ROW(CHAR_ROW&& a) noexcept
{
    this->swap(a);
}

CHAR_ROW::~CHAR_ROW()
{
}

// Routine Description:
// - swaps values with another CHAR_ROW
// Arguments:
// - other - the CHAR_ROW to swap with
// Return Value:
// - <none>
void CHAR_ROW::swap(CHAR_ROW& other) noexcept
{
    // this looks kinda weird, but we want the compiler to be able to choose between std::swap and a
    // specialized swap, so we include both in the same namespace and let it sort it out.
    using std::swap;
    swap(bRowFlags, other.bRowFlags);
    swap(_data, other._data);
}

// Routine Description:
// - gets the attribute at the specified column
// Arguments:
// - column - the column to get the attribute for
// Return Value:
// - the attribute
// Note: will throw exception if column is out of bounds
const DbcsAttribute& CHAR_ROW::GetAttribute(_In_ const size_t column) const
{
    return _data.at(column).second;
}

// Routine Description:
// - gets the attribute at the specified column
// Arguments:
// - column - the column to get the attribute for
// Return Value:
// - the attribute
// Note: will throw exception if column is out of bounds
DbcsAttribute& CHAR_ROW::GetAttribute(_In_ const size_t column)
{
    return const_cast<DbcsAttribute&>(static_cast<const CHAR_ROW* const>(this)->GetAttribute(column));
}

CHAR_ROW::iterator CHAR_ROW::begin() noexcept
{
    return _data.begin();
}

CHAR_ROW::const_iterator CHAR_ROW::cbegin() const noexcept
{
    return _data.cbegin();
}

CHAR_ROW::iterator CHAR_ROW::end() noexcept
{
    return _data.end();
}

CHAR_ROW::const_iterator CHAR_ROW::cend() const noexcept
{
    return _data.cend();
}

size_t CHAR_ROW::size() const noexcept
{
    return _data.size();
}

// Routine Description:
// - resets text data at column
// Arguments:
// - column - column index to clear text data from
// Return Value:
// - <none>
// Note: will throw exception if column is out of bounds
void CHAR_ROW::ClearGlyph(const size_t column)
{
    _data.at(column).first = UNICODE_SPACE;
}

// Routine Description:
// - returns text data at column as a const reference.
// Arguments:
// - column - column to get text data for
// Return Value:
// - text data at column
// - Note: will throw exception if column is out of bounds
const wchar_t& CHAR_ROW::GetGlyphAt(const size_t column) const
{
    return _data.at(column).first;
}

// Routine Description:
// - returns text data at column as a reference.
// Arguments:
// - column - column to get text data for
// Return Value:
// - text data at column
// - Note: will throw exception if column is out of bounds
wchar_t& CHAR_ROW::GetGlyphAt(const size_t column)
{
    return const_cast<wchar_t&>(static_cast<const CHAR_ROW* const>(this)->GetGlyphAt(column));
}

// Routine Description:
// - returns the all of the text in a row, including leading and trailing whitespace.
// Arguments:
// - <none>
// Return Value:
// - all text data in the row
std::wstring CHAR_ROW::GetText() const
{
    std::wstring temp(_data.size(), UNICODE_SPACE);
    for (size_t i = 0; i < _data.size(); ++i)
    {
        temp[i] = _data[i].first;
    }
    return temp;
}

// Routine Description:
// - Sets all properties of the CHAR_ROW to default values
// Arguments:
// - sRowWidth - The width of the row.
// Return Value:
// - <none>
void CHAR_ROW::Reset(_In_ short const sRowWidth)
{
    const value_type insertVals{ UNICODE_SPACE, DbcsAttribute() };

    std::fill(_data.begin(), _data.end(), insertVals);
    _data.resize(sRowWidth, insertVals);

    SetWrapStatus(false);
    SetDoubleBytePadded(false);
}

// Routine Description:
// - resizes the width of the CHAR_ROW
// Arguments:
// - newSize - the new width of the character and attributes rows
// Return Value:
// - S_OK on success, otherwise relevant error code
HRESULT CHAR_ROW::Resize(_In_ size_t const newSize)
{
    // last attribute in a row gets extended to the end
    const value_type insertVals{ UNICODE_SPACE, _data.back().second };
    _data.resize(newSize, insertVals);

    return S_OK;
}

// Routine Description:
// - Sets the wrap status for the current row
// Arguments:
// - fWrapWasForced - True if the row ran out of space and we forced to wrap to the next row. False otherwise.
// Return Value:
// - <none>
void CHAR_ROW::SetWrapStatus(_In_ bool const fWrapWasForced)
{
    if (fWrapWasForced)
    {
        this->bRowFlags |= RowFlags::WrapForced;
    }
    else
    {
        this->bRowFlags &= ~RowFlags::WrapForced;
    }
}

// Routine Description:
// - Gets the wrap status for the current row
// Arguments:
// - <none>
// Return Value:
// - True if the row ran out of space and we were forced to wrap to the next row. False otherwise.
bool CHAR_ROW::WasWrapForced() const
{
    return IsFlagSet((DWORD)this->bRowFlags, (DWORD)RowFlags::WrapForced);
}

// Routine Description:
// - Sets the double byte padding for the current row
// Arguments:
// - fWrapWasForced - True if the row ran out of space for a double byte character and we padded out the row. False otherwise.
// Return Value:
// - <none>
void CHAR_ROW::SetDoubleBytePadded(_In_ bool const fDoubleBytePadded)
{
    if (fDoubleBytePadded)
    {
        this->bRowFlags |= RowFlags::DoubleBytePadded;
    }
    else
    {
        this->bRowFlags &= ~RowFlags::DoubleBytePadded;
    }
}

// Routine Description:
// - Gets the double byte padding status for the current row.
// Arguments:
// - <none>
// Return Value:
// - True if the row didn't have space for a double byte character and we were padded out the row. False otherwise.
bool CHAR_ROW::WasDoubleBytePadded() const
{
    return IsFlagSet((DWORD)this->bRowFlags, (DWORD)RowFlags::DoubleBytePadded);
}

// Routine Description:
// - Inspects the current internal string to find the left edge of it
// Arguments:
// - <none>
// Return Value:
// - The calculated left boundary of the internal string.
size_t CHAR_ROW::MeasureLeft() const
{
    std::vector<value_type>::const_iterator it = _data.cbegin();
    while (it != _data.cend() && it->first == UNICODE_SPACE)
    {
        ++it;
    }
    return it - _data.begin();
}

// Routine Description:
// - Inspects the current internal string to find the right edge of it
// Arguments:
// - <none>
// Return Value:
// - The calculated right boundary of the internal string.
size_t CHAR_ROW::MeasureRight() const
{
    std::vector<value_type>::const_reverse_iterator it = _data.crbegin();
    while (it != _data.crend() && it->first == UNICODE_SPACE)
    {
        ++it;
    }
    return _data.crend() - it;
}

// Routine Description:
// - Tells you whether or not this row contains any valid text.
// Arguments:
// - <none>
// Return Value:
// - True if there is valid text in this row. False otherwise.
bool CHAR_ROW::ContainsText() const
{
    for (const value_type& vals : _data)
    {
        if (vals.first != UNICODE_SPACE)
        {
            return true;
        }
    }
    return false;
}

ICharRow::SupportedEncoding CHAR_ROW::GetSupportedEncoding() const noexcept
{
    return ICharRow::SupportedEncoding::Ucs2;
}

void CHAR_ROW::ClearCell(_In_ const size_t column)
{
    _data.at(column) = { UNICODE_SPACE, DbcsAttribute() };
}
