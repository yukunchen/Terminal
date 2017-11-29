/*++
Copyright (c) Microsoft Corporation

Module Name:
- textBuffer.hpp

Abstract:
- This module contains structures and functions for manipulating a text
  based buffer within the console host window.

Author(s):
- Michael Niksa (miniksa) 10-Apr-2014
- Paul Campbell (paulcam) 10-Apr-2014

Revision History:
- From components of output.h/.c
  by Therese Stowell (ThereseS) 1990-1991

Notes:
ScreenBuffer data structure overview:

each screen buffer has an array of ROW structures.  each ROW structure
contains the data for one row of text.  the data stored for one row of
text is a character array and an attribute array.  the character array
is allocated the full length of the row from the heap, regardless of the
non-space length. we also maintain the non-space length.  the character
array is initialized to spaces.  the attribute
array is run length encoded (i.e 5 BLUE, 3 RED). if there is only one
attribute for the whole row (the normal case), it is stored in the ATTR_ROW
structure.  otherwise the attr string is allocated from the heap.

ROW - CHAR_ROW - CHAR string
\          \ length of char string
\
ATTR_ROW - ATTR_PAIR string
\ length of attr pair string
ROW
ROW
ROW

ScreenInfo->Rows points to the ROW array. ScreenInfo->Rows[0] is not
necessarily the top row. ScreenInfo->BufferInfo.TextInfo->FirstRow contains the index of
the top row.  That means scrolling (if scrolling entire screen)
merely involves changing the FirstRow index,
filling in the last row, and updating the screen.

--*/

#pragma once

#include "cursor.h"
#include "..\renderer\inc\FontInfo.hpp"
#include "..\renderer\inc\FontInfoDesired.hpp"

#include <deque>
#include <memory>
#include <wil/resource.h>
#include <wil/wistd_memory.h>

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

// run-length encoded data structure for attributes

class TextAttribute final
{
public:
    TextAttribute();
    TextAttribute(_In_ const WORD wLegacyAttr);
    TextAttribute(_In_ const COLORREF rgbForeground, _In_ const COLORREF rgbBackground);

    WORD GetLegacyAttributes() const;

    COLORREF GetRgbForeground() const;
    COLORREF GetRgbBackground() const;

    bool IsLeadingByte() const;
    bool IsTrailingByte() const;

    bool IsTopHorizontalDisplayed() const;
    bool IsBottomHorizontalDisplayed() const;
    bool IsLeftVerticalDisplayed() const;
    bool IsRightVerticalDisplayed() const;

    void SetFromLegacy(_In_ const WORD wLegacy);
    void SetMetaAttributes(_In_ const WORD wMeta);

    void SetFrom(_In_ const TextAttribute& otherAttr);
    bool IsEqual(_In_ const TextAttribute& otherAttr) const;
    bool IsEqualToLegacy(_In_ const WORD wLegacy) const;

    bool IsLegacy() const;

    void SetForeground(_In_ const COLORREF rgbForeground);
    void SetBackground(_In_ const COLORREF rgbBackground);
    void SetColor(_In_ const COLORREF rgbColor, _In_ const bool fIsForeground);

private:
    COLORREF _GetRgbForeground() const;
    COLORREF _GetRgbBackground() const;

    bool _IsReverseVideo() const;

    WORD _wAttrLegacy;
    bool _fUseRgbColor;
    COLORREF _rgbForeground;
    COLORREF _rgbBackground;
};

class TextAttributeRun final
{
public:
    UINT GetLength() const;
    void SetLength(_In_ UINT const cchLength);

    const TextAttribute GetAttributes() const;
    void SetAttributes(_In_ const TextAttribute textAttribute);
    void SetAttributesFromLegacy(_In_ const WORD wNew);

private:
    UINT _cchLength;
    TextAttribute _attributes;

#ifdef UNIT_TESTING
    friend class AttrRowTests;
#endif
};

// the attributes of one row of screen buffer

class ATTR_ROW final
{
public:
    ATTR_ROW(_In_ const UINT cchRowWidth, _In_ const TextAttribute attr);
    ATTR_ROW(const ATTR_ROW& a);
    ATTR_ROW& operator=(const ATTR_ROW& a);
    ATTR_ROW(ATTR_ROW&& a) noexcept = default;

    void swap(ATTR_ROW& other) noexcept;

    bool Reset(_In_ UINT const cchRowWidth, _In_ const TextAttribute attr);

    void FindAttrIndex(_In_ UINT const iIndex,
                       _Outptr_ TextAttributeRun** const ppIndexedAttr,
                       _Out_opt_ UINT* const pcAttrApplies) const;
    bool SetAttrToEnd(_In_ UINT const iStart, _In_ const TextAttribute attr);
    void ReplaceLegacyAttrs(_In_ const WORD wToBeReplacedAttr, _In_ const WORD wReplaceWith);
    HRESULT Resize(_In_ const short sOldWidth, _In_ const short sNewWidth);

    HRESULT InsertAttrRuns(_In_reads_(cAttrs) const TextAttributeRun* const prgAttrs,
                           _In_ const UINT cAttrs,
                           _In_ const UINT iStart,
                           _In_ const UINT iEnd,
                           _In_ const UINT cBufferWidth);

    NTSTATUS UnpackAttrs(_Out_writes_(cRowLength) TextAttribute* const rgAttrs, _In_ UINT const cRowLength) const;

    friend constexpr bool operator==(const ATTR_ROW& a, const ATTR_ROW& b) noexcept;

    UINT _cList;   // length of attr pair array
    wistd::unique_ptr<TextAttributeRun[]> _rgList;
private:

    UINT _cchRowWidth;


#ifdef UNIT_TESTING
    friend class AttrRowTests;
#endif

};

void swap(ATTR_ROW& a, ATTR_ROW& b) noexcept;
constexpr bool operator==(const ATTR_ROW& a, const ATTR_ROW& b) noexcept
{
    return (a._cList == b._cList &&
            a._rgList == b._rgList &&
            a._cchRowWidth == b._cchRowWidth);
}

// information associated with one row of screen buffer

class ROW final
{
public:
    ROW(_In_ const SHORT rowId, _In_ const short rowWidth, _In_ const TextAttribute fillAttribute);
    ROW(const ROW& a);
    ROW& operator=(const ROW& a);
    ROW(ROW&& a) noexcept;

    void swap(ROW& other) noexcept;

    CHAR_ROW CharRow;
    ATTR_ROW AttrRow;
    SHORT sRowId;

    bool Reset(_In_ short const sRowWidth, _In_ const TextAttribute Attr);
    HRESULT Resize(_In_ size_t const width);

    bool IsTrailingByteAtColumn(_In_ const size_t column) const;

    void ClearColumn(_In_ const size_t column);

    friend constexpr bool operator==(const ROW& a, const ROW& b) noexcept;

#ifdef UNIT_TESTING
    friend class RowTests;
#endif
};

void swap(ROW& a, ROW& b) noexcept;
constexpr bool operator==(const ROW& a, const ROW& b) noexcept
{
    return (a.CharRow == b.CharRow &&
            a.AttrRow == b.AttrRow &&
            a.sRowId == b.sRowId);
}

class TEXT_BUFFER_INFO final
{
private:
    // TEXT_BUFFER_INFO is supposed to be created by calling the static CreateInstance() function instead of the
    // constructor. But CreateInstance() uses std::make_unique() (which cannot be made a friend function) so the
    // constructor must be public. In order to enforce usage of CreateInstance() a dummy class is added to the
    // constructor which can only be created from within TEXT_BUFFER_INFO, effectively making the constructor
    // out of reach to the rest of the codebase (even if it is public). This should be removed if
    // CreateInstance() is ever removed.
    class ConstructorGuard final
    {
    public:
        explicit ConstructorGuard() {}
    };

public:
    TEXT_BUFFER_INFO(_In_ const FontInfo* const pfiFont, _In_ const ConstructorGuard guard);

    static NTSTATUS CreateInstance(_In_ const FontInfo* const pFontInfo,
                                   _In_ COORD coordScreenBufferSize,
                                   _In_ CHAR_INFO const Fill,
                                   _In_ UINT const uiCursorSize,
                                   _Outptr_ TEXT_BUFFER_INFO ** const ppTextBufferInfo);

    ~TEXT_BUFFER_INFO();

    // Used for duplicating properties to another text buffer
    void CopyProperties(_In_ TEXT_BUFFER_INFO* const pOtherBuffer);

    short GetMinBufferWidthNeeded() const; // TODO: just store this when the row is manipulated

    // row manipulation
    const ROW& GetFirstRow() const;
    ROW& GetFirstRow();

    const ROW& GetRowByOffset(_In_ const UINT index) const;
    ROW& GetRowByOffset(_In_ const UINT index);

    const ROW& GetPrevRowNoWrap(_In_ const ROW& row) const;
    ROW& GetPrevRowNoWrap(_In_ const ROW& row);

    const ROW& GetNextRowNoWrap(_In_ const ROW& row) const;
    ROW& GetNextRowNoWrap(_In_ const ROW& row);

    const ROW& GetRowAtIndex(_In_ const UINT index) const;
    ROW& GetRowAtIndex(_In_ const UINT index);

    const ROW& GetPrevRow(_In_ const ROW& row) const noexcept;
    ROW& GetPrevRow(_In_ const ROW& row) noexcept;

    const ROW& GetNextRow(_In_ const ROW& row) const noexcept;
    ROW& GetNextRow(_In_ const ROW& row) noexcept;

    // Text insertion functions
    bool InsertCharacter(_In_ WCHAR const wch, _In_ BYTE const bKAttr, _In_ const TextAttribute attr);
    bool IncrementCursor();
    bool NewlineCursor();

    // Cursor adjustment
    void DecrementCursor();

    // Scroll needs access to this to quickly rotate around the buffer.
    bool IncrementCircularBuffer();

    COORD GetLastNonSpaceCharacter() const;

    void SetCurrentFont(_In_ const FontInfo* const pfiNewFont);
    FontInfo* GetCurrentFont();

    void SetDesiredFont(_In_ const FontInfoDesired* const pfiNewFont);
    FontInfoDesired* GetDesiredFont();

    Cursor* const GetCursor() const;

    const SHORT GetFirstRowIndex() const;
    const COORD GetCoordBufferSize() const;

    void SetFirstRowIndex(_In_ SHORT const FirstRowIndex);
    void SetCoordBufferSize(_In_ COORD const coordBufferSize);

    UINT TotalRowCount() const;

    CHAR_INFO GetFill() const;
    void SetFill(_In_ const CHAR_INFO ciFill);

    NTSTATUS ResizeTraditional(_In_ COORD const currentSCreenBufferSize,
                               _In_ COORD const coordNewScreenSize,
                               _In_ TextAttribute const attributes);
private:

    std::deque<ROW> _storage;
    Cursor* _pCursor;

    SHORT _FirstRow; // indexes top row (not necessarily 0)

    COORD _coordBufferSize; // TODO: can we just measure the number of rows and/or their allocated width for this?

    FontInfo _fiCurrentFont;
    FontInfoDesired _fiDesiredFont;


    COORD GetPreviousFromCursor() const;

    void SetWrapOnCurrentRow();
    void AdjustWrapOnCurrentRow(_In_ bool const fSet);

    // Assist with maintaining proper buffer state for Double Byte character sequences
    bool _PrepareForDoubleByteSequence(_In_ BYTE const bKAttr);
    bool AssertValidDoubleByteSequence(_In_ BYTE const bKAttr);

    CHAR_INFO _ciFill;

#ifdef UNIT_TESTING
    friend class TextBufferTests;
#endif
};
typedef TEXT_BUFFER_INFO *PTEXT_BUFFER_INFO;
typedef PTEXT_BUFFER_INFO *PPTEXT_BUFFER_INFO;

// this sticks specializations of swap() into the std::namespace for our classes, so that callers that use
// std::swap explicitly over calling the global swap can still get the performance benefit.
namespace std
{
    template<>
    inline void swap<CHAR_ROW>(CHAR_ROW& a, CHAR_ROW& b) noexcept
    {
        a.swap(b);
    }

    template<>
    inline void swap<ATTR_ROW>(ATTR_ROW& a, ATTR_ROW& b) noexcept
    {
        a.swap(b);
    }

    template<>
    inline void swap<ROW>(ROW& a, ROW& b) noexcept
    {
        a.swap(b);
    }
}
