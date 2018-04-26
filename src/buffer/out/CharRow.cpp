/********************************************************
 *                                                       *
 *   Copyright (C) Microsoft. All rights reserved.       *
 *                                                       *
 ********************************************************/

#include "precomp.h"
#include "CharRow.hpp"
#include "unicode.hpp"

// default glyph value, used for reseting the character data portion of a cell
static constexpr CharRow::glyph_type DefaultValue = UNICODE_SPACE;

// Routine Description:
// - swaps two CharRows
// Arguments:
// - a - the first CharRow to swap
// - b - the second CharRow to swap
// Return Value:
// - <none>
void swap(CharRow& a, CharRow& b) noexcept
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
CharRow::CharRow(size_t rowWidth) :
    _wrapForced{ false },
    _doubleBytePadded{ false },
    _data(rowWidth, value_type(DefaultValue, DbcsAttribute{}))
{
}

// Routine Description:
// - assignment operator
// Arguments:
// - CharRow to copy
// Return Value:
// - reference to this object
CharRow& CharRow::operator=(const CharRow& a)
{
    CharRow temp(a);
    this->swap(temp);
    return *this;
}

// Routine Description:
// - swaps values with another CharRow
// Arguments:
// - other - the CharRow to swap with
// Return Value:
// - <none>
void CharRow::swap(CharRow& other) noexcept
{
    // this looks kinda weird, but we want the compiler to be able to choose between std::swap and a
    // specialized swap, so we include both in the same namespace and let it sort it out.
    using std::swap;
    swap(_wrapForced, other._wrapForced);
    swap(_doubleBytePadded, other._doubleBytePadded);
    swap(_data, other._data);
}

ICharRow::SupportedEncoding CharRow::GetSupportedEncoding() const noexcept
{
    return ICharRow::SupportedEncoding::Ucs2;
}

// Routine Description:
// - Sets the wrap status for the current row
// Arguments:
// - wrapForced - True if the row ran out of space and we forced to wrap to the next row. False otherwise.
// Return Value:
// - <none>
void CharRow::SetWrapForced(const bool wrapForced) noexcept
{
    _wrapForced = wrapForced;
}

// Routine Description:
// - Gets the wrap status for the current row
// Arguments:
// - <none>
// Return Value:
// - True if the row ran out of space and we were forced to wrap to the next row. False otherwise.
bool CharRow::WasWrapForced() const noexcept
{
    return _wrapForced;
}

// Routine Description:
// - Sets the double byte padding for the current row
// Arguments:
// - fWrapWasForced - True if the row ran out of space for a double byte character and we padded out the row. False otherwise.
// Return Value:
// - <none>
void CharRow::SetDoubleBytePadded(const bool doubleBytePadded) noexcept
{
    _doubleBytePadded = doubleBytePadded;
}

// Routine Description:
// - Gets the double byte padding status for the current row.
// Arguments:
// - <none>
// Return Value:
// - True if the row didn't have space for a double byte character and we were padded out the row. False otherwise.
bool CharRow::WasDoubleBytePadded() const noexcept
{
    return _doubleBytePadded;
}

// Routine Description:
// - gets the size of the row, in glyph cells
// Arguments:
// - <none>
// Return Value:
// - the size of the row
size_t CharRow::size() const noexcept
{
    return _data.size();
}

// Routine Description:
// - Sets all properties of the CharRowBase to default values
// Arguments:
// - sRowWidth - The width of the row.
// Return Value:
// - <none>
void CharRow::Reset()
{
    const value_type insertVals{ DefaultValue, DbcsAttribute{} };
    std::fill(_data.begin(), _data.end(), insertVals);

    _wrapForced = false;
    _doubleBytePadded = false;
}

// Routine Description:
// - resizes the width of the CharRowBase
// Arguments:
// - newSize - the new width of the character and attributes rows
// Return Value:
// - S_OK on success, otherwise relevant error code
[[nodiscard]]
HRESULT CharRow::Resize(const size_t newSize) noexcept
{
    try
    {
        const value_type insertVals{ DefaultValue, DbcsAttribute{} };
        _data.resize(newSize, insertVals);
    }
    CATCH_RETURN();

    return S_OK;
}

typename CharRow::iterator CharRow::begin() noexcept
{
    return _data.begin();
}

typename CharRow::const_iterator CharRow::cbegin() const noexcept
{
    return _data.cbegin();
}

typename CharRow::iterator CharRow::end() noexcept
{
    return _data.end();
}

typename CharRow::const_iterator CharRow::cend() const noexcept
{
    return _data.cend();
}

// Routine Description:
// - Inspects the current internal string to find the left edge of it
// Arguments:
// - <none>
// Return Value:
// - The calculated left boundary of the internal string.
size_t CharRow::MeasureLeft() const
{
    std::vector<value_type>::const_iterator it = _data.cbegin();
    while (it != _data.cend() && it->first == DefaultValue)
    {
        ++it;
    }
    return it - _data.cbegin();
}

// Routine Description:
// - Inspects the current internal string to find the right edge of it
// Arguments:
// - <none>
// Return Value:
// - The calculated right boundary of the internal string.
size_t CharRow::MeasureRight() const noexcept
{
    std::vector<value_type>::const_reverse_iterator it = _data.crbegin();
    while (it != _data.crend() && it->first == DefaultValue)
    {
        ++it;
    }
    return _data.crend() - it;
}

void CharRow::ClearCell(const size_t column)
{
    _data.at(column) = { DefaultValue, DbcsAttribute() };
}

// Routine Description:
// - Tells you whether or not this row contains any valid text.
// Arguments:
// - <none>
// Return Value:
// - True if there is valid text in this row. False otherwise.
bool CharRow::ContainsText() const noexcept
{
    for (const value_type& vals : _data)
    {
        if (vals.first != DefaultValue)
        {
            return true;
        }
    }
    return false;
}

// Routine Description:
// - gets the attribute at the specified column
// Arguments:
// - column - the column to get the attribute for
// Return Value:
// - the attribute
// Note: will throw exception if column is out of bounds
const DbcsAttribute& CharRow::GetAttribute(const size_t column) const
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
DbcsAttribute& CharRow::GetAttribute(const size_t column)
{
    return const_cast<DbcsAttribute&>(static_cast<const CharRow* const>(this)->GetAttribute(column));
}

// Routine Description:
// - resets text data at column
// Arguments:
// - column - column index to clear text data from
// Return Value:
// - <none>
// Note: will throw exception if column is out of bounds
void CharRow::ClearGlyph(const size_t column)
{
    _data.at(column).first = DefaultValue;
}

// Routine Description:
// - returns text data at column as a const reference.
// Arguments:
// - column - column to get text data for
// Return Value:
// - text data at column
// - Note: will throw exception if column is out of bounds
const CharRow::glyph_type& CharRow::GetGlyphAt(const size_t column) const
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
CharRow::glyph_type& CharRow::GetGlyphAt(const size_t column)
{
    return const_cast<glyph_type&>(static_cast<const CharRow* const>(this)->GetGlyphAt(column));
}

// Routine Description:
// - returns string containing text data exactly how it's stored internally, including doubling of
// leading/trailing cells.
// Arguments:
// - none
// Return Value:
// - text stored in char row
// - Note: will throw exception if out of memory
std::wstring CharRow::GetTextRaw() const
{
    std::wstring str;
    str.reserve(_data.size());
    for (auto& cell : _data)
    {
        str.push_back(cell.first);
    }
    return str;
}

std::wstring CharRow::GetText() const
{
    std::wstring wstr;
    wstr.reserve(_data.size());
    for (auto& cell : _data)
    {
        if (!cell.second.IsTrailing())
        {
            wstr.push_back(cell.first);
        }
    }
    return wstr;
}
