/*++
Copyright (c) Microsoft Corporation

Module Name:
- screenInfoTextIterator.hpp

Abstract:
- This module abstracts walking through text on the screen
- It is currently intended for read-only operations

Author(s):
- Michael Niksa (MiNiksa) 01-May-2018
--*/

#pragma once

#include "../buffer/out/CharRow.hpp"
#include "screenInfoCellIterator.hpp"

class SCREEN_INFORMATION;

class ScreenInfoTextIterator : public ScreenInfoCellIterator
{
public:
    ScreenInfoTextIterator(const SCREEN_INFORMATION& si, COORD pos);

    const CharRow::reference operator*() const;

protected:

#if UNIT_TESTING
    friend class ScreenInfoIteratorTests;
#endif
};
