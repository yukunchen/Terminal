/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "textBufferTextIterator.hpp"

#include "CharRow.hpp"
#include "Row.hpp"

#pragma hdrstop

TextBufferTextIterator::TextBufferTextIterator(const TextBuffer& buffer, COORD pos) :
    TextBufferCellIterator(buffer, pos)
{
}

const CharRow::reference TextBufferTextIterator::operator*() const
{
    return _pRow->GetCharRow().GlyphAt(_pos.X);
}
