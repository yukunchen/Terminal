/*++
Copyright (c) Microsoft Corporation

Module Name:
- CharRowBaseImpl.hpp

Abstract:
- implementation of base class for output buffer characters rows

Author(s):
- Austin Diviness (AustDi) 13-Feb-2018

Revision History:
--*/

#pragma once

// don't warn when one of these functions isn't used by a file that includes this header
#pragma warning(push)
#pragma warning(disable:4505)

// Routine Description:
// - swaps two CharRowBases
// Arguments:
// - a - the first CharRowBase to swap
// - b - the second CharRowBase to swap
// Return Value:
// - <none>
template<typename GlyphType, typename StringType>
void swap(CharRowBase<GlyphType, StringType>& a, CharRowBase<GlyphType, StringType>& b) noexcept
{
    a.swap(b);
}

template<typename GlyphType, typename StringType>
CharRowBase<GlyphType, StringType>::CharRowBase(_In_ const size_t rowWidth, _In_ const GlyphType defaultValue) :
    _wrapForced{ false },
    _doubleBytePadded{ false },
    _defaultValue{ defaultValue },
    _data(rowWidth, value_type(defaultValue, DbcsAttribute{}))
{
}

// Routine Description:
// - swaps values with another CharRowBase
// Arguments:
// - other - the CharRowBase to swap with
// Return Value:
// - <none>
template<typename GlyphType, typename StringType>
void CharRowBase<GlyphType, StringType>::swap(_In_ CharRowBase<GlyphType, StringType>& other) noexcept
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
template<typename GlyphType, typename StringType>
void CharRowBase<GlyphType, StringType>::SetWrapForced(_In_ bool const wrapForced) noexcept
{
    _wrapForced = wrapForced;
}

// Routine Description:
// - Gets the wrap status for the current row
// Arguments:
// - <none>
// Return Value:
// - True if the row ran out of space and we were forced to wrap to the next row. False otherwise.
template<typename GlyphType, typename StringType>
bool CharRowBase<GlyphType, StringType>::WasWrapForced() const noexcept
{
    return _wrapForced;
}

// Routine Description:
// - Sets the double byte padding for the current row
// Arguments:
// - fWrapWasForced - True if the row ran out of space for a double byte character and we padded out the row. False otherwise.
// Return Value:
// - <none>
template<typename GlyphType, typename StringType>
void CharRowBase<GlyphType, StringType>::SetDoubleBytePadded(_In_ bool const doubleBytePadded) noexcept
{
    _doubleBytePadded = doubleBytePadded;
}

// Routine Description:
// - Gets the double byte padding status for the current row.
// Arguments:
// - <none>
// Return Value:
// - True if the row didn't have space for a double byte character and we were padded out the row. False otherwise.
template<typename GlyphType, typename StringType>
bool CharRowBase<GlyphType, StringType>::WasDoubleBytePadded() const noexcept
{
    return _doubleBytePadded;
}

// Routine Description:
// - gets the size of the row, in glyph cells
// Arguments:
// - <none>
// Return Value:
// - the size of the row
template<typename GlyphType, typename StringType>
size_t CharRowBase<GlyphType, StringType>::size() const noexcept
{
    return _data.size();
}

// Routine Description:
// - Sets all properties of the CharRowBase to default values
// Arguments:
// - sRowWidth - The width of the row.
// Return Value:
// - <none>
template<typename GlyphType, typename StringType>
void CharRowBase<GlyphType, StringType>::Reset()
{
    const value_type insertVals{ _defaultValue, DbcsAttribute{} };
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
template<typename GlyphType, typename StringType>
HRESULT CharRowBase<GlyphType, StringType>::Resize(_In_ const size_t newSize) noexcept
{
    try
    {
        const value_type insertVals{ _defaultValue, DbcsAttribute{} };
        _data.resize(newSize, insertVals);
    }
    CATCH_RETURN();

    return S_OK;
}

template<typename GlyphType, typename StringType>
typename CharRowBase<GlyphType, StringType>::iterator CharRowBase<GlyphType, StringType>::begin() noexcept
{
    return _data.begin();
}

template<typename GlyphType, typename StringType>
typename CharRowBase<GlyphType, StringType>::const_iterator CharRowBase<GlyphType, StringType>::cbegin() const noexcept
{
    return _data.cbegin();
}

template<typename GlyphType, typename StringType>
typename CharRowBase<GlyphType, StringType>::iterator CharRowBase<GlyphType, StringType>::end() noexcept
{
    return _data.end();
}

template<typename GlyphType, typename StringType>
typename CharRowBase<GlyphType, StringType>::const_iterator CharRowBase<GlyphType, StringType>::cend() const noexcept
{
    return _data.cend();
}

// Routine Description:
// - Inspects the current internal string to find the left edge of it
// Arguments:
// - <none>
// Return Value:
// - The calculated left boundary of the internal string.
template<typename GlyphType, typename StringType>
size_t CharRowBase<GlyphType, StringType>::MeasureLeft() const
{
    std::vector<value_type>::const_iterator it = _data.cbegin();
    while (it != _data.cend() && it->first == _defaultValue)
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
template<typename GlyphType, typename StringType>
size_t CharRowBase<GlyphType, StringType>::MeasureRight() const
{
    std::vector<value_type>::const_reverse_iterator it = _data.crbegin();
    while (it != _data.crend() && it->first == _defaultValue)
    {
        ++it;
    }
    return _data.crend() - it;
}

template<typename GlyphType, typename StringType>
void CharRowBase<GlyphType, StringType>::ClearCell(_In_ const size_t column)
{
    _data.at(column) = { _defaultValue, DbcsAttribute() };
}

// Routine Description:
// - Tells you whether or not this row contains any valid text.
// Arguments:
// - <none>
// Return Value:
// - True if there is valid text in this row. False otherwise.
template<typename GlyphType, typename StringType>
bool CharRowBase<GlyphType, StringType>::ContainsText() const
{
    for (const value_type& vals : _data)
    {
        if (vals.first != _defaultValue)
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
template<typename GlyphType, typename StringType>
const DbcsAttribute& CharRowBase<GlyphType, StringType>::GetAttribute(_In_ const size_t column) const
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
template<typename GlyphType, typename StringType>
DbcsAttribute& CharRowBase<GlyphType, StringType>::GetAttribute(_In_ const size_t column)
{
    return const_cast<DbcsAttribute&>(static_cast<const CharRowBase<GlyphType, StringType>* const>(this)->GetAttribute(column));
}

// Routine Description:
// - resets text data at column
// Arguments:
// - column - column index to clear text data from
// Return Value:
// - <none>
// Note: will throw exception if column is out of bounds
template<typename GlyphType, typename StringType>
void CharRowBase<GlyphType, StringType>::ClearGlyph(const size_t column)
{
    _data.at(column).first = _defaultValue;
}

// Routine Description:
// - returns text data at column as a const reference.
// Arguments:
// - column - column to get text data for
// Return Value:
// - text data at column
// - Note: will throw exception if column is out of bounds
template<typename GlyphType, typename StringType>
const GlyphType& CharRowBase<GlyphType, StringType>::GetGlyphAt(const size_t column) const
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
template<typename GlyphType, typename StringType>
GlyphType& CharRowBase<GlyphType, StringType>::GetGlyphAt(const size_t column)
{
    return const_cast<GlyphType&>(static_cast<const CharRowBase<GlyphType, StringType>* const>(this)->GetGlyphAt(column));
}

#pragma warning(pop)
