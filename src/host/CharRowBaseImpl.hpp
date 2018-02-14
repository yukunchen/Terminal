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

// Routine Description:
// - swaps two CharRowBases
// Arguments:
// - a - the first CharRowBase to swap
// - b - the second CharRowBase to swap
// Return Value:
// - <none>
template<typename T, typename StringType>
void swap(CharRowBase<T, StringType>& a, CharRowBase<T, StringType>& b) noexcept
{
    a.swap(b);
}

template<typename T, typename StringType>
inline CharRowBase<T, StringType>::CharRowBase(_In_ const size_t rowWidth, _In_ const T defaultValue) :
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
template<typename T, typename StringType>
inline void CharRowBase<T, StringType>::swap(_In_ CharRowBase<T, StringType>& other) noexcept
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
template<typename T, typename StringType>
inline void CharRowBase<T, StringType>::SetWrapForced(_In_ bool const wrapForced) noexcept
{
    _wrapForced = wrapForced;
}

// Routine Description:
// - Gets the wrap status for the current row
// Arguments:
// - <none>
// Return Value:
// - True if the row ran out of space and we were forced to wrap to the next row. False otherwise.
template<typename T, typename StringType>
inline bool CharRowBase<T, StringType>::WasWrapForced() const noexcept
{
    return _wrapForced;
}

// Routine Description:
// - Sets the double byte padding for the current row
// Arguments:
// - fWrapWasForced - True if the row ran out of space for a double byte character and we padded out the row. False otherwise.
// Return Value:
// - <none>
template<typename T, typename StringType>
inline void CharRowBase<T, StringType>::SetDoubleBytePadded(_In_ bool const doubleBytePadded) noexcept
{
    _doubleBytePadded = doubleBytePadded;
}

// Routine Description:
// - Gets the double byte padding status for the current row.
// Arguments:
// - <none>
// Return Value:
// - True if the row didn't have space for a double byte character and we were padded out the row. False otherwise.
template<typename T, typename StringType>
inline bool CharRowBase<T, StringType>::WasDoubleBytePadded() const noexcept
{
    return _doubleBytePadded;
}

template<typename T, typename StringType>
inline size_t CharRowBase<T, StringType>::size() const noexcept
{
    return _data.size();
}

// Routine Description:
// - Sets all properties of the CharRowBase to default values
// Arguments:
// - sRowWidth - The width of the row.
// Return Value:
// - <none>
template<typename T, typename StringType>
inline void CharRowBase<T, StringType>::Reset(_In_ const short sRowWidth)
{
    const value_type insertVals{ _defaultValue, DbcsAttribute{} };
    std::fill(_data.begin(), _data.end(), insertVals);
    _data.resize(sRowWidth, insertVals);
    _wrapForced = false;
    _doubleBytePadded = false;
}

// Routine Description:
// - resizes the width of the CharRowBase
// Arguments:
// - newSize - the new width of the character and attributes rows
// Return Value:
// - S_OK on success, otherwise relevant error code
template<typename T, typename StringType>
inline HRESULT CharRowBase<T, StringType>::Resize(_In_ const size_t newSize) noexcept
{
    try
    {
        const value_type insertVals{ _defaultValue, DbcsAttribute{} };
        _data.resize(newSize, insertVals);
    }
    CATCH_RETURN();

    return S_OK;
}

template<typename T, typename StringType>
inline typename CharRowBase<T, StringType>::iterator CharRowBase<T, StringType>::begin() noexcept
{
    return _data.begin();
}

template<typename T, typename StringType>
inline typename CharRowBase<T, StringType>::const_iterator CharRowBase<T, StringType>::cbegin() const noexcept
{
    return _data.cbegin();
}

template<typename T, typename StringType>
inline typename CharRowBase<T, StringType>::iterator CharRowBase<T, StringType>::end() noexcept
{
    return _data.end();
}

template<typename T, typename StringType>
inline typename CharRowBase<T, StringType>::const_iterator CharRowBase<T, StringType>::cend() const noexcept
{
    return _data.cend();
}

// Routine Description:
// - Inspects the current internal string to find the left edge of it
// Arguments:
// - <none>
// Return Value:
// - The calculated left boundary of the internal string.
template<typename T, typename StringType>
inline size_t CharRowBase<T, StringType>::MeasureLeft() const
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
template<typename T, typename StringType>
inline size_t CharRowBase<T, StringType>::MeasureRight() const
{
    std::vector<value_type>::const_reverse_iterator it = _data.crbegin();
    while (it != _data.crend() && it->first == _defaultValue)
    {
        ++it;
    }
    return _data.crend() - it;
}

template<typename T, typename StringType>
inline void CharRowBase<T, StringType>::ClearCell(_In_ const size_t column)
{
    _data.at(column) = { _defaultValue, DbcsAttribute() };
}

// Routine Description:
// - Tells you whether or not this row contains any valid text.
// Arguments:
// - <none>
// Return Value:
// - True if there is valid text in this row. False otherwise.
template<typename T, typename StringType>
inline bool CharRowBase<T, StringType>::ContainsText() const
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
template<typename T, typename StringType>
inline const DbcsAttribute& CharRowBase<T, StringType>::GetAttribute(_In_ const size_t column) const
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
template<typename T, typename StringType>
inline DbcsAttribute& CharRowBase<T, StringType>::GetAttribute(_In_ const size_t column)
{
    return const_cast<DbcsAttribute&>(static_cast<const CharRowBase<T, StringType>* const>(this)->GetAttribute(column));
}

// Routine Description:
// - resets text data at column
// Arguments:
// - column - column index to clear text data from
// Return Value:
// - <none>
// Note: will throw exception if column is out of bounds
template<typename T, typename StringType>
inline void CharRowBase<T, StringType>::ClearGlyph(const size_t column)
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
template<typename T, typename StringType>
inline const T& CharRowBase<T, StringType>::GetGlyphAt(const size_t column) const
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
template<typename T, typename StringType>
inline T& CharRowBase<T, StringType>::GetGlyphAt(const size_t column)
{
    return const_cast<T&>(static_cast<const CharRowBase<T, StringType>* const>(this)->GetGlyphAt(column));
}
