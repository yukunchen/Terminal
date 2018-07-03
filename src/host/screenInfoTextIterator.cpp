/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "screenInfoTextIterator.hpp"

#include "misc.h"

#include "../buffer/out/CharRow.hpp"

#pragma hdrstop

ScreenInfoTextIterator::ScreenInfoTextIterator(const SCREEN_INFORMATION& si, COORD pos) :
    ScreenInfoCellIterator(si, pos)
{
}

const CharRow::reference ScreenInfoTextIterator::operator*() const
{
    return _pRow->GetCharRow().GlyphAt(_pos.X);
}
