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
#include "FontInfo.hpp"

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

class TextAttribute sealed
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
    void SetFrom(_In_ const TextAttribute* const pOtherAttr);
    bool IsEqual(_In_ const TextAttribute* const pOtherAttr) const;
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

class TextAttributeRun sealed
{
public:
    UINT GetLength();
    void SetLength(_In_ UINT const cchLength);

    const TextAttribute* GetAttributes() const;
    void SetAttributes(_In_ const TextAttribute* const pNew);
    void SetAttributesFromLegacy(_In_ const WORD wNew);

private:
    UINT _cchLength;
    TextAttribute _attributes;
};

// the attributes of one row of screen buffer

class ATTR_ROW sealed
{
public:

    bool Initialize(_In_ UINT const cchRowWidth, _In_ const TextAttribute* const pAttr);

    void FindAttrIndex(_In_ UINT const iIndex, _Outptr_ TextAttributeRun** const ppIndexedAttr, _Out_ UINT* const cAttrApplies) const;
    TextAttributeRun* GetHead() const;
    
    NTSTATUS PackAttrs(_In_reads_(cRowLength) const TextAttribute* const rgAttrs, _In_ UINT const cRowLength);
    NTSTATUS UnpackAttrs(_Out_writes_(cRowLength) TextAttribute* const rgAttrs, _In_ UINT const cRowLength) const;
    
    bool SetAttrToEnd(_In_ UINT const iStart, _In_ const TextAttribute* const pAttr);
    void ReplaceLegacyAttrs(_In_ const WORD wToBeReplacedAttr, _In_ const WORD wReplaceWith);
    bool Resize(_In_ const short sOldWidth, _In_ const short sNewWidth);

    NTSTATUS InsertAttrRuns(_In_reads_(cMergeAttrPairs) TextAttributeRun* prgMergeAttrPairs,
                            _In_ const short cMergeAttrPairs,
                            _In_ const short sStartIndex,
                            _In_ const short sEndIndex,
                            _In_ const short BufferWidth);

    SHORT Length;   // length of attr pair array
private:

    UINT _TotalLength() const;
    TextAttributeRun* _pAttribRunListHead = nullptr;

#ifdef UNIT_TESTING
    friend class AttrRowTests;
#endif

};

// information associated with one row of screen buffer

class ROW
{
public:
    CHAR_ROW CharRow;
    ATTR_ROW AttrRow;
    SHORT sRowId;

    bool Initialize(_In_ short const sRowWidth, _In_ const TextAttribute* const pAttr);
#ifdef UNIT_TESTING
    friend class RowTests;
#endif
};

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
    ROW* GetFirstRow() const;
    ROW* GetRowByOffset(_In_ UINT const rowIndex) const;
    ROW* GetPrevRowNoWrap(_In_ ROW* const pRow) const;
    ROW* GetNextRowNoWrap(_In_ ROW* const pRow) const;

    // Text insertion functions
    bool InsertCharacter(_In_ WCHAR const wch, _In_ BYTE const bKAttr, _In_ const TextAttribute* const pAttr);
    bool IncrementCursor();
    bool NewlineCursor();

    // Cursor adjustment
    void DecrementCursor();

    // Scroll needs access to this to quickly rotate around the buffer.
    bool IncrementCircularBuffer();

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

    void FreeExtraAttributeRows(_In_ const short sTopRowIndex, _In_ const short sOldHeight, _In_ const short sNewHeight);

    ROW* Rows;
    PWCHAR TextRows;
    // all DBCS lead & trail bit buffer
    PBYTE KAttrRows;
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
    bool _PrepareForDoubleByteSequence(_In_ BYTE const bKAttr);
    bool AssertValidDoubleByteSequence(_In_ BYTE const bKAttr);

    CHAR_INFO _ciFill;

#ifdef UNIT_TESTING
friend class TextBufferTests;
#endif
};
typedef TEXT_BUFFER_INFO *PTEXT_BUFFER_INFO;
typedef PTEXT_BUFFER_INFO *PPTEXT_BUFFER_INFO;
