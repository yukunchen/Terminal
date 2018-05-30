/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"
#include "Row.hpp"
#include "CharRow.hpp"
#include "textBuffer.hpp"
#include "../types/inc/convert.hpp"

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
// - pParent - the text buffer that this row belongs to
// Return Value:
// - constructed object
ROW::ROW(const SHORT rowId, const short rowWidth, const TextAttribute fillAttribute, TextBuffer* const pParent) :
    _id{ rowId },
    _rowWidth{ gsl::narrow<size_t>(rowWidth) },
    _charRow{ gsl::narrow<size_t>(rowWidth), this },
    _attrRow{ rowWidth, fillAttribute },
    _pParent{ pParent }
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
    _rowWidth{ a._rowWidth },
    _id{ a._id },
    _charRow{ a._charRow },
    _pParent{ a._pParent }
{
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
    _id{ std::move(a._id) },
    _rowWidth{ a._rowWidth },
    _pParent{ a._pParent }
{
}

// Routine Description:
// - swaps fields with another ROW. does not swap parent text buffer
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
    swap(_rowWidth, other._rowWidth);
    swap(_pParent, other._pParent);
}
size_t ROW::size() const noexcept
{
    return _rowWidth;
}

const CharRow& ROW::GetCharRow() const
{
    return _charRow;
}

CharRow& ROW::GetCharRow()
{
    return const_cast<CharRow&>(static_cast<const ROW* const>(this)->GetCharRow());
}

const ATTR_ROW& ROW::GetAttrRow() const noexcept
{
    return _attrRow;
}

ATTR_ROW& ROW::GetAttrRow() noexcept
{
    return const_cast<ATTR_ROW&>(static_cast<const ROW* const>(this)->GetAttrRow());
}

SHORT ROW::GetId() const noexcept
{
    return _id;
}

void ROW::SetId(const SHORT id) noexcept
{
    _id = id;
}

// Routine Description:
// - Sets all properties of the ROW to default values
// Arguments:
// - Attr - The default attribute (color) to fill
// Return Value:
// - <none>
bool ROW::Reset(const TextAttribute Attr)
{
    _charRow.Reset();
    try
    {
        _attrRow.Reset(Attr);
    }
    catch (...)
    {
        LOG_CAUGHT_EXCEPTION();
        return false;
    }
    return true;
}

// Routine Description:
// - resizes ROW to new width
// Arguments:
// - width - the new width, in cells
// Return Value:
// - S_OK if successful, otherwise relevant error
[[nodiscard]]
HRESULT ROW::Resize(const size_t width)
{
    RETURN_IF_FAILED(_charRow.Resize(width));
    try
    {
        _attrRow.Resize(width);
    }
    CATCH_RETURN();

    _rowWidth = width;

    return S_OK;
}

// Routine Description:
// - clears char data in column in row
// Arguments:
// - column - 0-indexed column index
// Return Value:
// - <none>
void ROW::ClearColumn(const size_t column)
{
    THROW_HR_IF(E_INVALIDARG, column >= _charRow.size());
    _charRow.ClearCell(column);
}

// Routine Description:
// - gets the text of the row as it would be shown on the screen
// Return Value:
// - wstring containing text for the row
std::wstring ROW::GetText() const
{
    return _charRow.GetText();
}

// Routine Description:
// - gets the cell data for the row
// Return Value:
// - vector of cell data for row, one object per column
std::vector<OutputCell> ROW::AsCells() const
{
    return AsCells(0, size());
}

// Routine Description:
// - gets the cell data for the row
// Arguments:
// - startIndex - index to start fetching data from
// Return Value:
// - vector of cell data for row, one object per column
std::vector<OutputCell> ROW::AsCells(const size_t startIndex) const
{
    return AsCells(startIndex, size() - startIndex);
}

// Routine Description:
// - gets the cell data for the row
// Arguments:
// - startIndex - index to start fetching data from
// - count - the number of cells to grab
// Return Value:
// - vector of cell data for row, one object per column
std::vector<OutputCell> ROW::AsCells(const size_t startIndex, const size_t count) const
{
    std::vector<OutputCell> cells;
    cells.reserve(size());

    // Unpack the attributes into an array so we can iterate over them.
    const auto unpackedAttrs = _attrRow.UnpackAttrs();

    for (size_t i = 0; i < count; ++i)
    {
        const auto index = startIndex + i;
        cells.emplace_back(_charRow.GlyphAt(index), _charRow.DbcsAttrAt(index), unpackedAttrs[index]);
    }
    return cells;
}

const OutputCell ROW::at(const size_t column) const
{
    return { _charRow.GlyphAt(column), _charRow.DbcsAttrAt(column), _attrRow.GetAttrByColumn(column) };
}

UnicodeStorage& ROW::GetUnicodeStorage()
{
    return _pParent->GetUnicodeStorage();
}

const UnicodeStorage& ROW::GetUnicodeStorage() const
{
    return _pParent->GetUnicodeStorage();
}
