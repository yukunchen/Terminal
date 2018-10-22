/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "OutputCellRect.hpp"

OutputCellRect::OutputCellRect(const size_t rows, const size_t cols) :
    _rows(rows),
    _cols(cols)
{
    size_t totalCells;
    THROW_IF_FAILED(SizeTMult(rows, cols, &totalCells));

    _storage = std::make_unique<byte[]>(sizeof(OutputCell) * totalCells);
    THROW_IF_NULL_ALLOC(_storage.get());
}

gsl::span<OutputCell> OutputCellRect::GetRow(const size_t row)
{
    return gsl::span<OutputCell>(_FindRowOffset(row), _cols);
}

OutputCellIterator OutputCellRect::GetRowIter(const size_t row) const
{
    const std::basic_string_view<OutputCell> view(_FindRowOffset(row), _cols);

    return OutputCellIterator(view);
}

OutputCell* OutputCellRect::_FindRowOffset(const size_t row) const
{
    return (reinterpret_cast<OutputCell*>(_storage.get()) + (row * _cols));
}

size_t OutputCellRect::Height() const noexcept
{
    return _rows;
}

size_t OutputCellRect::Width() const noexcept
{
    return _cols;
}
