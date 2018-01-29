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
    _rowWidth{ static_cast<size_t>(rowWidth) },
    Left{ rowWidth },
    Right{ 0 },
    _attributes(rowWidth)
{
    Chars = std::make_unique<wchar_t[]>(rowWidth);
    THROW_IF_NULL_ALLOC(Chars.get());
    wmemset(Chars.get(), PADDING_CHAR, rowWidth);

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
    Left{ a.Left },
    Right{ a.Right },
    bRowFlags{ a.bRowFlags },
    _rowWidth{ a._rowWidth },
    _attributes{ a._attributes }
{
    Chars = std::make_unique<wchar_t[]>(_rowWidth);
    THROW_IF_NULL_ALLOC(Chars.get());
    std::copy(a.Chars.get(), a.Chars.get() + a._rowWidth, Chars.get());
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
    swap(Left, other.Left);
    swap(Right, other.Right);
    swap(Chars, other.Chars);
    swap(bRowFlags, other.bRowFlags);
    swap(_rowWidth, other._rowWidth);
    swap(_attributes, other._attributes);
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
    return _attributes.at(column);
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

// Routine Description:
// - returns an iterator to the data at specified column
// Arguments:
// - column - the column to start the iterator at (0-indexed)
// Return Value:
// - iterator starting at column
// Note: will throw exception if column is out of bounds
std::vector<DbcsAttribute>::iterator CHAR_ROW::GetAttributeIterator(_In_ const size_t column)
{
    THROW_HR_IF(E_INVALIDARG, column >= _attributes.size());
    return std::next(_attributes.begin(), column);
}

// Routine Description:
// - returns a const iterator to the data at specified column
// Arguments:
// - column - the column to start the const iterator at (0-indexed)
// Return Value:
// - const iterator starting at column
// Note: will throw exception if column is out of bounds
std::vector<DbcsAttribute>::const_iterator CHAR_ROW::GetAttributeIterator(_In_ const size_t column) const
{
    THROW_HR_IF(E_INVALIDARG, column >= _attributes.size());
    return std::next(_attributes.cbegin(), column);
}

std::vector<DbcsAttribute>::const_iterator CHAR_ROW::GetAttributeIteratorEnd() const noexcept
{
    return _attributes.cend();
}

// Routine Description:
// - gets the size of the char row, in text elements
// Arguments:
// - <none>
// Return Value:
// - the size of the wchar_t and attribute buffer, in their respective elements
size_t CHAR_ROW::GetWidth() const
{
    return _rowWidth;
}


// Routine Description:
// - Sets all properties of the CHAR_ROW to default values
// Arguments:
// - sRowWidth - The width of the row.
// Return Value:
// - <none>
void CHAR_ROW::Reset(_In_ short const sRowWidth)
{
    _rowWidth = static_cast<size_t>(sRowWidth);
    Left = sRowWidth;
    Right = 0;

    wmemset(Chars.get(), PADDING_CHAR, sRowWidth);
    for (DbcsAttribute& attr : _attributes)
    {
        attr.SetSingle();
    }

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
    std::unique_ptr<wchar_t[]> charBuffer;
    try
    {
        charBuffer = std::make_unique<wchar_t[]>(newSize);
    }
    CATCH_RETURN();
    RETURN_IF_NULL_ALLOC(charBuffer.get());

    std::unique_ptr<BYTE[]> attributesBuffer;
    try
    {
        attributesBuffer = std::make_unique<BYTE[]>(newSize);
    }
    CATCH_RETURN();
    RETURN_IF_NULL_ALLOC(attributesBuffer.get());

    const size_t copySize = min(newSize, _rowWidth);
    std::copy(Chars.get(), Chars.get() + copySize, charBuffer.get());

    if (newSize > _rowWidth)
    {
        std::fill(charBuffer.get() + copySize, charBuffer.get() + newSize, UNICODE_SPACE);
    }

    // last attribute in a row gets extended to the end
    _attributes.resize(newSize, _attributes.back());

    Chars.swap(charBuffer);
    Left = static_cast<short>(newSize);
    Right = 0;
    _rowWidth = newSize;

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
// - Inspects the current row contents and sets the Left/Right/OldLeft/OldRight boundary values as appropriate.
// Arguments:
// - sRowWidth - The width of the row.
// Return Value:
// - <none>
void CHAR_ROW::RemeasureBoundaryValues(_In_ short const sRowWidth)
{
    this->MeasureAndSaveLeft(sRowWidth);
    this->MeasureAndSaveRight(sRowWidth);
}

// Routine Description:
// - Inspects the current internal string to find the left edge of it
// Arguments:
// - sRowWidth - The width of the row.
// Return Value:
// - The calculated left boundary of the internal string.
short CHAR_ROW::MeasureLeft(_In_ short const sRowWidth) const
{
    wchar_t* pLastChar = &this->Chars[sRowWidth];
    wchar_t* pChar = this->Chars.get();

    for (; pChar < pLastChar && *pChar == PADDING_CHAR; pChar++)
    {
        /* do nothing */
    }

    return short(pChar - this->Chars.get());
}

// Routine Description:
// - Inspects the current internal string to find the right edge of it
// Arguments:
// - sRowWidth - The width of the row.
// Return Value:
// - The calculated right boundary of the internal string.
short CHAR_ROW::MeasureRight(_In_ short const sRowWidth) const
{
    wchar_t* pFirstChar = this->Chars.get();
    wchar_t* pChar = &this->Chars[sRowWidth - 1];

    for (; pChar >= pFirstChar && *pChar == PADDING_CHAR; pChar--)
    {
        /* do nothing */
    }

    return short((pChar - this->Chars.get()) + 1);
}

// Routine Description:
// - Updates the Left and OldLeft fields for this structure.
// Arguments:
// - sRowWidth - The width of the row.
// Return Value:
// - <none>
void CHAR_ROW::MeasureAndSaveLeft(_In_ short const sRowWidth)
{
    this->Left = MeasureLeft(sRowWidth);
}

// Routine Description:
// - Updates the Right and OldRight fields for this structure.
// Arguments:
// - sRowWidth - The width of the row.
// Return Value:
// - <none>
void CHAR_ROW::MeasureAndSaveRight(_In_ short const sRowWidth)
{
    this->Right = MeasureRight(sRowWidth);
}

// Routine Description:
// - Tells you whether or not this row contains any valid text.
// Arguments:
// - <none>
// Return Value:
// - True if there is valid text in this row. False otherwise.
bool CHAR_ROW::ContainsText() const
{
    return this->Right > this->Left;
}
