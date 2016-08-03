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
#include "cursor.h"
#include "..\inc\render\FontInfo.hpp"

typedef struct _CHAR_ROW
{
    static const SHORT INVALID_OLD_LENGTH = -1;

    // for use with pbKAttrs
    static const BYTE ATTR_SINGLE_BYTE = 0x00;
    static const BYTE ATTR_LEADING_BYTE = 0x01;
    static const BYTE ATTR_TRAILING_BYTE = 0x02;
    static const BYTE ATTR_DBCSSBCS_BYTE = 0x03;
    static const BYTE ATTR_SEPARATE_BYTE = 0x10;
    static const BYTE ATTR_EUDCFLAG_BYTE = 0x20;

    SHORT Right;    // one past rightmost bound of chars in Chars array (array will be full width)
    SHORT Left; // leftmost bound of chars in Chars array (array will be full width)
    PWCHAR Chars;   // all chars in row up to last non-space char
    PBYTE KAttrs;   // all DBCS lead & trail bit in row

    void Initialize(_In_ short const sRowWidth);

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

private:
    RowFlags bRowFlags;

#ifdef UNIT_TESTING
    friend class CharRowTests;
#endif

} CHAR_ROW, *PCHAR_ROW;

DEFINE_ENUM_FLAG_OPERATORS(_CHAR_ROW::RowFlags);

// run-length encoded data structure for attributes

typedef struct _ATTR_PAIR
{
    SHORT Length;   // number of times attribute appears
    WORD Attr;  // attribute
} ATTR_PAIR, *PATTR_PAIR, **PPATTR_PAIR;

// the attributes of one row of screen buffer

typedef struct _ATTR_ROW
{
    SHORT Length;   // length of attr pair array
    ATTR_PAIR AttrPair; // use this if only one pair
    PATTR_PAIR Attrs;   // attr pair array

    void Initialize(_In_ short const sRowWidth, _In_ WORD const wAttr);

    void FindAttrIndex(_In_ UINT const uiIndex, _Outptr_ ATTR_PAIR** const ppIndexedAttr, _Out_ UINT* const cAttrApplies) const;
    NTSTATUS PackAttrs(_In_reads_(cRowLength) const WORD* const rgAttrs, _In_ int const cRowLength);
    NTSTATUS UnpackAttrs(_Out_writes_(cRowLength) WORD* const rgAttrs, _In_ int const cRowLength) const;
    void SetAttrToEnd(_In_ short const sStartIndex, _In_ WORD const wAttr);

private:
    UINT TotalLength() const;

#ifdef UNIT_TESTING
    friend class AttrRowTests;
#endif

} ATTR_ROW, *PATTR_ROW;

// information associated with one row of screen buffer

typedef struct _ROW
{
    CHAR_ROW CharRow;
    ATTR_ROW AttrRow;
    SHORT sRowId;

    void Initialize(_In_ short const sRowWidth, _In_ WORD const wAttr);

#ifdef UNIT_TESTING
    friend class RowTests;
#endif
} ROW, *PROW;

class DBCS_SCREEN_BUFFER
{
public:
    static NTSTATUS CreateInstance(_In_ COORD const dwScreenBufferSize, _Outptr_ DBCS_SCREEN_BUFFER ** const ppDbcsScreenBuffer);

    ~DBCS_SCREEN_BUFFER();

    // all DBCS lead & trail bit buffer
    PBYTE KAttrRows;

private:
    DBCS_SCREEN_BUFFER();
};
typedef DBCS_SCREEN_BUFFER *PDBCS_SCREEN_BUFFER;

class TEXT_BUFFER_INFO
{
public:
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
    PROW GetFirstRow() const;
    PROW GetRowByOffset(_In_ UINT const rowIndex) const;
    PROW GetPrevRowNoWrap(_In_ PROW const pRow) const;
    PROW GetNextRowNoWrap(_In_ PROW const pRow) const;

    // Text insertion functions
    void InsertCharacter(_In_ WCHAR const wch, _In_ BYTE const bKAttr, _In_ WORD const wAttr);
    void InsertNewline();
    void IncrementCursor();
    void NewlineCursor();

    // Cursor adjustment
    void DecrementCursor();

    // Scroll needs access to this to quickly rotate around the buffer.
    void IncrementCircularBuffer();

    COORD GetLastNonSpaceCharacter() const;

    void SetCurrentFont(_In_ const FontInfo* const pfiNewFont);
    FontInfo* GetCurrentFont();

    Cursor* const GetCursor() const;

    const SHORT GetFirstRowIndex() const;
    const COORD GetCoordBufferSize() const;

    void SetFirstRowIndex(_In_ SHORT const FirstRowIndex);
    void SetCoordBufferSize(_In_ COORD const coordBufferSize);

    CHAR_INFO GetFill() const;
    void SetFill(_In_ const CHAR_INFO ciFill);

    PROW Rows;
    PWCHAR TextRows;
    PDBCS_SCREEN_BUFFER pDbcsScreenBuffer;
private:

    Cursor* _pCursor;

    SHORT _FirstRow; // indexes top row (not necessarily 0)

    COORD _coordBufferSize; // TODO: can we just measure the number of rows and/or their allocated width for this?

    FontInfo _fiCurrentFont;

    TEXT_BUFFER_INFO(_In_ const FontInfo* const pfiFont);
    UINT _TotalRowCount() const;

    COORD GetPreviousFromCursor() const;

    void SetWrapOnCurrentRow();
    void AdjustWrapOnCurrentRow(_In_ bool const fSet);

    // Assist with maintaining proper buffer state for Double Byte character sequences
    void PrepareForDoubleByteSequence(_In_ BYTE const bKAttr);
    bool AssertValidDoubleByteSequence(_In_ BYTE const bKAttr);

    CHAR_INFO _ciFill;

#ifdef UNIT_TESTING
friend class TextBufferTests;
#endif
};
typedef TEXT_BUFFER_INFO *PTEXT_BUFFER_INFO;
typedef PTEXT_BUFFER_INFO *PPTEXT_BUFFER_INFO;
