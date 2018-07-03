/*++
Copyright (c) Microsoft Corporation

Module Name:
- screenInfoCellIterator.hpp

Abstract:
- This module abstracts walking through text on the screen
- It is currently intended for read-only operations

Author(s):
- Michael Niksa (MiNiksa) 29-Jun-2018
--*/

#pragma once

#include "../buffer/out/CharRow.hpp"

class SCREEN_INFORMATION;

class ScreenInfoCellIterator : public std::iterator<std::random_access_iterator_tag, const wchar_t>
{
public:
    ScreenInfoCellIterator(const SCREEN_INFORMATION& si, COORD pos);
    ScreenInfoCellIterator(const SCREEN_INFORMATION& si, COORD pos, SMALL_RECT limits);

    ScreenInfoCellIterator(const ScreenInfoCellIterator& it) = default;
    ~ScreenInfoCellIterator() = default;

    ScreenInfoCellIterator& operator=(const ScreenInfoCellIterator& it) = default;

    operator bool() const noexcept;

    bool operator==(const ScreenInfoCellIterator& it) const noexcept;
    bool operator!=(const ScreenInfoCellIterator& it) const noexcept;

    ScreenInfoCellIterator& operator+=(const ptrdiff_t& movement);
    ScreenInfoCellIterator& operator-=(const ptrdiff_t& movement);
    ScreenInfoCellIterator& operator++();
    ScreenInfoCellIterator& operator--();
    ScreenInfoCellIterator operator++(int);
    ScreenInfoCellIterator operator--(int);
    ScreenInfoCellIterator operator+(const ptrdiff_t& movement);
    ScreenInfoCellIterator operator-(const ptrdiff_t& movement);

    ptrdiff_t operator-(const ScreenInfoCellIterator& it);

    const CHAR_INFO operator*() const;

protected:

    void _SetPos(const COORD newPos);
    static const ROW* s_GetRow(const SCREEN_INFORMATION& si, const COORD pos);

    const ROW* _pRow;
    const SCREEN_INFORMATION& _si;
    SMALL_RECT _bounds;
    bool _exceeded;
    COORD _pos;

#if UNIT_TESTING
    friend class ScreenInfoIteratorTests;
#endif
};

