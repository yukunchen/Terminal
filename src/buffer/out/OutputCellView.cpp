/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "OutputCellView.hpp"

OutputCellView::OutputCellView(const std::wstring_view view,
                               const DbcsAttribute dbcsAttr,
                               const TextAttribute textAttr,
                               const OutputCell::TextAttributeBehavior behavior) :
    _view(view),
    _dbcsAttr(dbcsAttr),
    _textAttr(textAttr),
    _behavior(behavior)
{

}

std::wstring_view OutputCellView::Chars() const
{
    return _view;
}

DbcsAttribute OutputCellView::DbcsAttr() const
{
    return _dbcsAttr;
}

TextAttribute OutputCellView::TextAttr() const
{
    return _textAttr;
}

OutputCell::TextAttributeBehavior OutputCellView::TextAttrBehavior() const
{
    return _behavior;
}

bool OutputCellView::operator==(const OutputCellView& it) const noexcept
{
    return _view == it._view &&
        _dbcsAttr == it._dbcsAttr &&
        _textAttr == it._textAttr &&
        _behavior == it._behavior;
}
bool OutputCellView::operator!=(const OutputCellView& it) const noexcept
{
    return !(*this == it);
}
