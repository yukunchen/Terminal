/*++
Copyright (c) Microsoft Corporation

Module Name:
- CharRowBase.hpp

Abstract:
- base class for output buffer characters rows

Author(s):
- Austin Diviness (AustDi) 13-Feb-2018

Revision History:
--*/

#pragma once

#include "ICharRow.hpp"

template <typename T>
class CharRowBase : public ICharRow
{
public:
    using value_type = typename std::pair<T, DbcsAttribute>;
    using iterator = typename std::vector<value_type>::iterator;
    using const_iterator = typename std::vector<value_type>::const_iterator;


    CharRowBase(_In_ const size_t rowWidth, _In_ const T defaultValue);
    virtual ~CharRowBase() = default;

    void swap(CharRowBase& other) noexcept;

    // ICharRow methods
    void SetWrapForced(_In_ bool const wrap) noexcept;
    bool WasWrapForced() const noexcept;
    void SetDoubleBytePadded(_In_ bool const doubleBytePadded) noexcept;
    bool WasDoubleBytePadded() const noexcept;
    size_t size() const noexcept override;
    // TODO does Reset also resize the row?
    void Reset(_In_ short const sRowWidth);
    HRESULT Resize(_In_ const size_t newSize) noexcept;
    size_t MeasureLeft() const override;
    size_t MeasureRight() const override;

    // iterators
    iterator begin() noexcept;
    const_iterator cbegin() const noexcept;

    iterator end() noexcept;
    const_iterator cend() const noexcept;

    friend constexpr bool operator==(const CharRowBase& a, const CharRowBase& b) noexcept;

protected:
    // Occurs when the user runs out of text in a given row and we're forced to wrap the cursor to the next line
    bool _wrapForced;

    // Occurs when the user runs out of text to support a double byte character and we're forced to the next line
    bool _doubleBytePadded;

    const T _defaultValue;

    std::vector<value_type> _data;
};

template <typename T>
void swap(CharRowBase<T>& a, CharRowBase<T>& b) noexcept;

template <typename T>
constexpr bool operator==(const CharRowBase<T>& a, const CharRowBase<T>& b) noexcept
{
    return (a._wrapForced == b._wrapForced &&
            a._doubleBytePadded == b._doubleBytePadded &&
            a._defaultValue == b._defaultValue &&
            a._data == b._data);
}

// this sticks specialization of swap() into the std::namespace for CharRowBase, so that callers that use
// std::swap explicitly over calling the global swap can still get the performance benefit.
namespace std
{
    /*
      // TODO
    template<>
    inline void swap<CharRowBase<T>>(CharRowBase<T>& a, CharRowBase<T>& b) noexcept
    {
        a.swap(b);
    }
    */
}

// Routine Description:
// - swaps two CharRowBases
// Arguments:
// - a - the first CharRowBase to swap
// - b - the second CharRowBase to swap
// Return Value:
// - <none>
template<typename T>
void swap(CharRowBase<T>& a, CharRowBase<T>& b) noexcept
{
    a.swap(b);
}

#pragma warning(push)
#pragma warning(disable:4505)
template<typename T>
CharRowBase<T>::CharRowBase(_In_ const size_t rowWidth, _In_ const T defaultValue) :
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
template<typename T>
void CharRowBase<T>::swap(_In_ CharRowBase<T>& other) noexcept
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
template<typename T>
void CharRowBase<T>::SetWrapForced(_In_ bool const wrapForced) noexcept
{
    _wrapForced = wrapForced;
}

// Routine Description:
// - Gets the wrap status for the current row
// Arguments:
// - <none>
// Return Value:
// - True if the row ran out of space and we were forced to wrap to the next row. False otherwise.
template<typename T>
bool CharRowBase<T>::WasWrapForced() const noexcept
{
    return _wrapForced;
}

// Routine Description:
// - Sets the double byte padding for the current row
// Arguments:
// - fWrapWasForced - True if the row ran out of space for a double byte character and we padded out the row. False otherwise.
// Return Value:
// - <none>
template<typename T>
void CharRowBase<T>::SetDoubleBytePadded(_In_ bool const doubleBytePadded) noexcept
{
    _doubleBytePadded = doubleBytePadded;
}

// Routine Description:
// - Gets the double byte padding status for the current row.
// Arguments:
// - <none>
// Return Value:
// - True if the row didn't have space for a double byte character and we were padded out the row. False otherwise.
template<typename T>
bool CharRowBase<T>::WasDoubleBytePadded() const noexcept
{
    return _doubleBytePadded;
}

template<typename T>
size_t CharRowBase<T>::size() const noexcept
{
    return _data.size();
}

// Routine Description:
// - Sets all properties of the CharRowBase to default values
// Arguments:
// - sRowWidth - The width of the row.
// Return Value:
// - <none>
template<typename T>
void CharRowBase<T>::Reset(_In_ const short sRowWidth)
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
template<typename T>
HRESULT CharRowBase<T>::Resize(_In_ const size_t newSize) noexcept
{
    try
    {
        const value_type insertVals{ _defaultValue, DbcsAttribute{} };
        _data.resize(newSize, insertVals);
    }
    CATCH_RETURN();

    return S_OK;
}

template<typename T>
typename CharRowBase<T>::iterator CharRowBase<T>::begin() noexcept
{
    return _data.begin();
}

template<typename T>
typename CharRowBase<T>::const_iterator CharRowBase<T>::cbegin() const noexcept
{
    return _data.cbegin();
}

template<typename T>
typename CharRowBase<T>::iterator CharRowBase<T>::end() noexcept
{
    return _data.end();
}

template<typename T>
typename CharRowBase<T>::const_iterator CharRowBase<T>::cend() const noexcept
{
    return _data.cend();
}

// Routine Description:
// - Inspects the current internal string to find the left edge of it
// Arguments:
// - <none>
// Return Value:
// - The calculated left boundary of the internal string.
template<typename T>
size_t CharRowBase<T>::MeasureLeft() const
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
template<typename T>
size_t CharRowBase<T>::MeasureRight() const
{
    std::vector<value_type>::const_reverse_iterator it = _data.crbegin();
    while (it != _data.crend() && it->first == _defaultValue)
    {
        ++it;
    }
    return _data.crend() - it;
}

#pragma warning(pop)
