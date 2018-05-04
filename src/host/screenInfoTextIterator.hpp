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
class SCREEN_INFORMATION;
class CharRow;

class ScreenInfoTextIterator : public std::iterator<std::random_access_iterator_tag, const wchar_t>
{
public:
    ScreenInfoTextIterator(const SCREEN_INFORMATION* const psi, COORD pos);

    ScreenInfoTextIterator(const ScreenInfoTextIterator& it) = default;
    ~ScreenInfoTextIterator() = default;

    ScreenInfoTextIterator& operator=(const ScreenInfoTextIterator& it) = default;

    operator bool() const;

    bool operator==(const ScreenInfoTextIterator& it) const;
    bool operator!=(const ScreenInfoTextIterator& it) const;

    ScreenInfoTextIterator& operator+=(const ptrdiff_t& movement);
    ScreenInfoTextIterator& operator-=(const ptrdiff_t& movement);
    ScreenInfoTextIterator& operator++();
    ScreenInfoTextIterator& operator--();
    ScreenInfoTextIterator operator++(int);
    ScreenInfoTextIterator operator--(int);
    ScreenInfoTextIterator operator+(const ptrdiff_t& movement);
    ScreenInfoTextIterator operator-(const ptrdiff_t& movement);

    ptrdiff_t operator-(const ScreenInfoTextIterator& it);

    const wchar_t& operator*();
    const wchar_t& operator*() const;
    const wchar_t* operator->();
    const wchar_t* getPtr() const;
    const wchar_t* getConstPtr() const;

protected:

    void _SetPos(const COORD newPos);
    static const CharRow* s_GetRow(const SCREEN_INFORMATION* const psi, const COORD pos);

    const CharRow* _pRow;
    const SCREEN_INFORMATION* const _psi;
    COORD _pos;

#if UNIT_TESTING
    friend class ScreenInfoTextIteratorTests;
#endif
};
