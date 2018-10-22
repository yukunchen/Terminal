/*++
Copyright (c) Microsoft Corporation

Module Name:
- OutputCellIterator.hpp

Abstract:
- Read-only view into an entire batch of data to be written into the output buffer.
- This is done for performance reasons (avoid heap allocs and copies).

Author:
- Michael Niksa (MiNiksa) 06-Oct-2018

Revision History:
- Based on work from OutputCell.hpp/cpp by Austin Diviness (AustDi)

--*/

#pragma once

#include "TextAttribute.hpp"

#include "OutputCell.hpp"
#include "OutputCellView.hpp"

class OutputCellIterator final
{
public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = OutputCellView;
    using difference_type = ptrdiff_t;
    using pointer = OutputCellView*;
    using reference = OutputCellView&;

    OutputCellIterator(const std::wstring_view utf16Text, const TextAttribute attribute);
    OutputCellIterator(const std::basic_string_view<OutputCell> cells);
    ~OutputCellIterator() = default;

    OutputCellIterator& operator=(const OutputCellIterator& it) = default;

    operator bool() const noexcept;

    bool operator==(const OutputCellIterator& it) const noexcept;
    bool operator!=(const OutputCellIterator& it) const noexcept;

    OutputCellIterator& operator+=(const ptrdiff_t& movement);
    OutputCellIterator& operator++();
    OutputCellIterator operator++(int);
    OutputCellIterator operator+(const ptrdiff_t& movement);

    const OutputCellView& operator*() const;
    const OutputCellView* operator->() const;

private:
    
    enum class Mode 
    { 
        // Loose mode is where we're given text and attributes in a raw sort of form
        // like while data is being inserted from an API call.
        Loose, 

        // Cell mode is where we have an already fully structured cell data usually
        // from accessing/copying data already put into the OutputBuffer.
        Cell 
    };
    Mode _mode;

    std::wstring_view _utf16Run;
    TextAttribute _singleAttribute;
    std::basic_string_view<OutputCell> _cells;

    bool _TryMoveTrailing();

    static OutputCellView s_GenerateView(const std::wstring_view view,
                                  const TextAttribute attr);

    static OutputCellView s_GenerateView(const OutputCell& cell);

    OutputCellView _currentView;

    size_t _pos;
};
