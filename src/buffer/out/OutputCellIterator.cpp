/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "OutputCellIterator.hpp"

#include "../../types/inc/convert.hpp"
#include "../../types/inc/Utf16Parser.hpp"
#include "../../host/dbcs.h"

OutputCellIterator::OutputCellIterator(const std::wstring_view utf16Text, const TextAttribute attribute) :
    _utf16Run(utf16Text),
    _singleAttribute(attribute),
    _pos(0),
    _currentView(s_GenerateView(utf16Text, attribute))
{

}

OutputCellIterator::operator bool() const noexcept
{
    // In lieu of using start and end, this custom iterator type simply becomes bool false
    // when we run out of items to iterate over.
    return _pos < _utf16Run.length();
}

bool OutputCellIterator::operator==(const OutputCellIterator& it) const noexcept
{
    return _utf16Run == it._utf16Run &&
        _singleAttribute == it._singleAttribute &&
        _pos == it._pos &&
        _currentView == it._currentView;
}
bool OutputCellIterator::operator!=(const OutputCellIterator& it) const noexcept
{
    return !(*this == it);
}

OutputCellIterator& OutputCellIterator::operator+=(const ptrdiff_t& movement)
{
    if (!_TryMoveTrailing())
    {
        _pos += movement;
        _RefreshView();
    }

    return (*this);
}

OutputCellIterator& OutputCellIterator::operator++()
{
    return this->operator+=(1);
}

OutputCellIterator OutputCellIterator::operator++(int)
{
    auto temp(*this);
    operator++();
    return temp;
}

OutputCellIterator OutputCellIterator::operator+(const ptrdiff_t& movement)
{
    auto temp(*this);
    temp += movement;
    return temp;
}

const OutputCellView& OutputCellIterator::operator*() const
{
    return _currentView;
}

const OutputCellView* OutputCellIterator::operator->() const
{
    return &_currentView;
}

// Routine Description:
// - Checks the current view. If it is a leading half, it updates the current
//   view to the trailing half of the same glyph. 
// - This helps us to draw glyphs that are two columns wide by "doubling"
//   the view that is returned so it will consume two cells.
// Arguments:
// - <none>
// Return Value:
// - True if we just turned a lead half into a trailing half (and caller doesn't
//   need to further update the view).
// - False if this wasn't applicable and the caller should update the view.
bool OutputCellIterator::_TryMoveTrailing()
{
    if (_currentView.DbcsAttr().IsLeading())
    {
        DbcsAttribute dbcsAttr;
        dbcsAttr.SetTrailing();
        _currentView = OutputCellView(_currentView.Chars(),
                                      dbcsAttr,
                                      _currentView.TextAttr(),
                                      _currentView.TextAttrBehavior());
        return true;
    }
    else
    {
        return false;
    }
}

// Routine Description:
// - Member function to update the view to the current position in the buffer with 
//   the data held on this object.
// Arguments:
// - <none>
// Return Value:
// - <none>
void OutputCellIterator::_RefreshView()
{
    _currentView = s_GenerateView(_utf16Run.substr(_pos), _singleAttribute);
}

// Routine Description:
// - Static function to create a view.
// - It's pulled out statically so it can be used during construction with just the given
//   variables (so OutputCellView doesn't need an empty default constructor)
// - This will infer the width of the glyph and apply the appropriate attributes to the view.
// Arguments:
// - view - View representing characters corresponding to a single glyph
// - attr - Color attributes to apply to the text
// Return Value:
// - Object representing the view into this cell
OutputCellView OutputCellIterator::s_GenerateView(const std::wstring_view view,
                                                  const TextAttribute attr)
{
    const auto glyph = Utf16Parser::ParseNext(view);
    DbcsAttribute dbcsAttr;
    if (!glyph.empty() && IsGlyphFullWidth(glyph))
    {
        dbcsAttr.SetLeading();
    }

    const auto textAttr = attr;
    const auto behavior = OutputCell::TextAttributeBehavior::Stored;

    return OutputCellView(glyph, dbcsAttr, textAttr, behavior);
}
