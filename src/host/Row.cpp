/********************************************************
 *                                                       *
 *   Copyright (C) Microsoft. All rights reserved.       *
 *                                                       *
 ********************************************************/

#include "precomp.h"
#include "Ucs2CharRow.hpp"

// Routine Description:
// - swaps two ROWs
// Arguments:
// - a - the first ROW to swap
// - b - the second ROW to swap
// Return Value:
// - <none>
void swap(ROW& a, ROW& b) noexcept
{
    a.swap(b);
}

// Routine Description:
// - constructor
// Arguments:
// - rowId - the row index in the text buffer
// - rowWidth - the width of the row, cell elements
// - fillAttribute - the default text attribute
// Return Value:
// - constructed object
ROW::ROW(_In_ const SHORT rowId, _In_ const short rowWidth, _In_ const TextAttribute fillAttribute) :
    _id{ rowId },
    _charRow{ std::make_unique<CHAR_ROW>(rowWidth) },
    _attrRow{ rowWidth, fillAttribute }
{
}

// Routine Description:
// - copy constructor
// Arguments:
// - a - the object to copy
// Return Value:
// - the copied object
ROW::ROW(const ROW& a) :
    _attrRow{ a._attrRow },
    _id{ a._id }
{
    if (a._charRow->GetSupportedEncoding() == ICharRow::SupportedEncoding::Ucs2)
    {
        CHAR_ROW charRow = *static_cast<const CHAR_ROW* const>(a._charRow.get());
        _charRow = std::make_unique<CHAR_ROW>(charRow);
    }
    else
    {
        THROW_HR_MSG(E_INVALIDARG, "ROW doesn't support copy constructor for non ucs2 ICharRow implementations");
    }
}

// Routine Description:
// - assignment operator overload
// Arguments:
// - a - the object to copy to this one
// Return Value:
// - a reference to this object
ROW& ROW::operator=(const ROW& a)
{
    ROW temp{ a };
    this->swap(temp);
    return *this;
}

// Routine Description:
// - move constructor
// Arguments:
// - a - the object to move
// Return Value:
// - the constructed object
ROW::ROW(ROW&& a) noexcept :
    _charRow{ std::move(a._charRow) },
    _attrRow{ std::move(a._attrRow) },
    _id{ std::move(a._id) }
{
}

// Routine Description:
// - swaps fields with another ROW
// Arguments:
// - other - the object to swap with
// Return Value:
// - <none>
void ROW::swap(ROW& other) noexcept
{
    using std::swap;
    swap(_charRow, other._charRow);
    swap(_attrRow, other._attrRow);
    swap(_id, other._id);
}

const ICharRow& ROW::GetCharRow() const
{
    return *_charRow;
}

ICharRow& ROW::GetCharRow()
{
    return const_cast<ICharRow&>(static_cast<const ROW* const>(this)->GetCharRow());
}

const ATTR_ROW& ROW::GetAttrRow() const
{
    return _attrRow;
}

ATTR_ROW& ROW::GetAttrRow()
{
    return const_cast<ATTR_ROW&>(static_cast<const ROW* const>(this)->GetAttrRow());
}

SHORT ROW::GetId() const noexcept
{
    return _id;
}

void ROW::SetId(_In_ const SHORT id)
{
    _id = id;
}

// Routine Description:
// - Sets all properties of the ROW to default values
// Arguments:
// - sRowWidth - The width of the row.
// - Attr - The default attribute (color) to fill
// Return Value:
// - <none>
bool ROW::Reset(_In_ short const sRowWidth, _In_ const TextAttribute Attr)
{
    _charRow->Reset(sRowWidth);
    return _attrRow.Reset(sRowWidth, Attr);
}

// Routine Description:
// - resizes ROW to new width
// Arguments:
// - width - the new width, in cells
// Return Value:
// - S_OK if successful, otherwise relevant error
HRESULT ROW::Resize(_In_ size_t const width)
{
    size_t oldWidth = _charRow->size();
    RETURN_IF_FAILED(_charRow->Resize(width));
    RETURN_IF_FAILED(_attrRow.Resize(static_cast<short>(oldWidth), static_cast<short>(width)));
    return S_OK;
}

// Routine Description:
// - clears char data in column in row
// Arguments:
// - column - 0-indexed column index
// Return Value:
// - <none>
void ROW::ClearColumn(_In_ const size_t column)
{
    THROW_HR_IF(E_INVALIDARG, column >= _charRow->size());
    _charRow->ClearCell(column);
}
