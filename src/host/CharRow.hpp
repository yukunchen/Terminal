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

// Characters used for padding out the buffer with invalid/empty space
#define PADDING_CHAR UNICODE_SPACE
#define PADDING_KATTR '\0'


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
    static const SHORT INVALID_OLD_LENGTH = -1;

    // for use with pbKAttrs
    static const BYTE ATTR_SINGLE_BYTE = 0x00;
    static const BYTE ATTR_LEADING_BYTE = 0x01;
    static const BYTE ATTR_TRAILING_BYTE = 0x02;
    static const BYTE ATTR_DBCSSBCS_BYTE = 0x03;
    static const BYTE ATTR_SEPARATE_BYTE = 0x10;
    static const BYTE ATTR_EUDCFLAG_BYTE = 0x20;

    CHAR_ROW(short rowWidth);
    CHAR_ROW(const CHAR_ROW& a);
    CHAR_ROW& operator=(const CHAR_ROW& a);
    CHAR_ROW(CHAR_ROW&& a) noexcept;
    ~CHAR_ROW();

    void swap(CHAR_ROW& other) noexcept;

    SHORT Right;    // one past rightmost bound of chars in Chars array (array will be full width)
    SHORT Left; // leftmost bound of chars in Chars array (array will be full width)
    std::unique_ptr<wchar_t[]> Chars; // all chars in row
    std::unique_ptr<BYTE[]> KAttrs; // all DBCS lead & trail bit in row

    void Reset(_In_ short const sRowWidth);

    HRESULT Resize(_In_ size_t const newSize);

    void SetWrapStatus(_In_ bool const fWrapWasForced);
    bool WasWrapForced() const;

    void SetDoubleBytePadded(_In_ bool const fDoubleBytePadded);
    bool WasDoubleBytePadded() const;

    enum class RowFlags
    {
        NoFlags = 0x0,
        WrapForced = 0x1, // Occurs when the user runs out of text in a given row and we're forced to wrap the cursor to the next line
        DoubleBytePadded = 0x2, // Occurs when the user runs out of text to support a double byte character and we're forced to the next line
    };

    // TODO: this class should know the width of itself and use that instead of the caller needing to know.
    // caller knowing more often than not means passing the SCREEN_INFO width... which is also silly.
    void RemeasureBoundaryValues(_In_ short const sRowWidth);
    void MeasureAndSaveLeft(_In_ short const sRowWidth);
    void MeasureAndSaveRight(_In_ short const sRowWidth);
    short MeasureLeft(_In_ short const sRowWidth) const;
    short MeasureRight(_In_ short const sRowWidth) const;

    static bool IsLeadingByte(_In_ BYTE const bKAttr);
    static bool IsTrailingByte(_In_ BYTE const bKAttr);
    static bool IsSingleByte(_In_ BYTE const bKAttr);

    bool ContainsText() const;

    size_t GetWidth() const;


    friend constexpr bool operator==(const CHAR_ROW& a, const CHAR_ROW& b) noexcept;

private:
    RowFlags bRowFlags;
    size_t _rowWidth;

#ifdef UNIT_TESTING
    friend class CharRowTests;
#endif

};

DEFINE_ENUM_FLAG_OPERATORS(CHAR_ROW::RowFlags);
void swap(CHAR_ROW& a, CHAR_ROW& b) noexcept;
constexpr bool operator==(const CHAR_ROW& a, const CHAR_ROW& b) noexcept
{
    return (a.bRowFlags == b.bRowFlags &&
            a._rowWidth == b._rowWidth &&
            a.Right == b.Right &&
            a.Left == b.Left &&
            a.Chars == b.Chars &&
            a.KAttrs == b.KAttrs);
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
