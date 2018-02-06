/*++
Copyright (c) Microsoft Corporation

Module Name:
- CharRow.hpp

Abstract:
- contains data structure for character data of a row

Author(s):
- Michael Niksa (miniksa) 10-Apr-2014
- Paul Campbell (paulcam) 10-Apr-2014

Revision History:
- From components of output.h/.c
  by Therese Stowell (ThereseS) 1990-1991
- Pulled into its own file from textBuffer.hpp/cpp (AustDi, 2017)
--*/

#pragma once

#include <memory>
#include <vector>

#include "DbcsAttribute.hpp"

// Characters used for padding out the buffer with invalid/empty space
#define PADDING_CHAR UNICODE_SPACE



// the characters of one row of screen buffer
// we keep the following values so that we don't write
// more pixels to the screen than we have to:
// left is initialized to screenbuffer width.  right is
// initialized to zero.
//
//      [     foo.bar    12-12-61                       ]
//       ^    ^                  ^                     ^
//       |    |                  |                     |
//     Chars Left               Right                end of Chars buffer
class CHAR_ROW final
{
public:
    using value_type = std::pair<wchar_t, DbcsAttribute>;
    using iterator = std::vector<value_type>::iterator;
    using const_iterator = std::vector<value_type>::const_iterator;

    CHAR_ROW(short rowWidth);
    CHAR_ROW(const CHAR_ROW& a);
    CHAR_ROW& operator=(const CHAR_ROW& a);
    CHAR_ROW(CHAR_ROW&& a) noexcept;
    ~CHAR_ROW();

    void swap(CHAR_ROW& other) noexcept;

    iterator begin() noexcept;
    const_iterator cbegin() const noexcept;

    iterator end() noexcept;
    const_iterator cend() const noexcept;

    size_t size() const noexcept;

    const DbcsAttribute& GetAttribute(_In_ const size_t column) const;
    DbcsAttribute& GetAttribute(_In_ const size_t column);

    std::wstring GetText() const;

    void ClearGlyph(const size_t column);

    const wchar_t& GetGlyphAt(const size_t column) const;
    wchar_t& GetGlyphAt(const size_t column);

    void Reset(_In_ short const sRowWidth);

    HRESULT Resize(_In_ size_t const newSize);

    void SetWrapStatus(_In_ bool const fWrapWasForced);
    bool WasWrapForced() const;

    void SetDoubleBytePadded(_In_ bool const fDoubleBytePadded);
    bool WasDoubleBytePadded() const;

    bool ContainsText() const;

    size_t MeasureLeft() const;
    size_t MeasureRight() const;

    friend constexpr bool operator==(const CHAR_ROW& a, const CHAR_ROW& b) noexcept;

    enum class RowFlags
    {
        NoFlags = 0x0,
        WrapForced = 0x1, // Occurs when the user runs out of text in a given row and we're forced to wrap the cursor to the next line
        DoubleBytePadded = 0x2, // Occurs when the user runs out of text to support a double byte character and we're forced to the next line
    };

private:
    RowFlags bRowFlags;
    std::vector<value_type> _data;

#ifdef UNIT_TESTING
    friend class CharRowTests;
#endif

};

DEFINE_ENUM_FLAG_OPERATORS(CHAR_ROW::RowFlags);
void swap(CHAR_ROW& a, CHAR_ROW& b) noexcept;
constexpr bool operator==(const CHAR_ROW& a, const CHAR_ROW& b) noexcept
{
    return (a.bRowFlags == b.bRowFlags &&
            a._data == b._data);
}

// this sticks specialization of swap() into the std::namespace for CHAR_ROW, so that callers that use
// std::swap explicitly over calling the global swap can still get the performance benefit.
namespace std
{
    template<>
    inline void swap<CHAR_ROW>(CHAR_ROW& a, CHAR_ROW& b) noexcept
    {
        a.swap(b);
    }
}
