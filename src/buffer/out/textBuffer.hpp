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

#pragma warning(push)
#pragma warning(disable: ALL_CPPCORECHECK_WARNINGS)
#include "../host/cursor.h"
#include "../renderer/inc/FontInfo.hpp"
#include "../renderer/inc/FontInfoDesired.hpp"
#pragma warning(pop)

#include "Row.hpp"
#include "TextAttribute.hpp"

class TextBuffer final
{
public:
    TextBuffer(const FontInfo fontInfo,
               const COORD screenBufferSize,
               const CHAR_INFO fill,
               const UINT cursorSize);
    TextBuffer(const TextBuffer& a) = delete;

    ~TextBuffer() = default;

    // Used for duplicating properties to another text buffer
    void CopyProperties(const TextBuffer& OtherBuffer);

    // row manipulation
    const ROW& GetFirstRow() const;
    ROW& GetFirstRow();

    const ROW& GetRowByOffset(const size_t index) const;
    ROW& GetRowByOffset(const size_t index);

    const ROW& GetPrevRowNoWrap(const ROW& row) const;
    ROW& GetPrevRowNoWrap(const ROW& row);

    const ROW& GetNextRowNoWrap(const ROW& row) const;
    ROW& GetNextRowNoWrap(const ROW& row);

    const ROW& GetRowAtIndex(const UINT index) const;
    ROW& GetRowAtIndex(const UINT index);

    const ROW& GetPrevRow(const ROW& row) const noexcept;
    ROW& GetPrevRow(const ROW& row) noexcept;

    const ROW& GetNextRow(const ROW& row) const noexcept;
    ROW& GetNextRow(const ROW& row) noexcept;

    // Text insertion functions
    bool InsertCharacter(const wchar_t wch, const DbcsAttribute dbcsAttribute, const TextAttribute attr);
    bool IncrementCursor();
    bool NewlineCursor();

    // Cursor adjustment
    void DecrementCursor();

    // Scroll needs access to this to quickly rotate around the buffer.
    bool IncrementCircularBuffer();

    COORD GetLastNonSpaceCharacter() const;

    FontInfo& GetCurrentFont();
    const FontInfo& GetCurrentFont() const;

    FontInfoDesired& GetDesiredFont();
    const FontInfoDesired& GetDesiredFont() const;

    Cursor& GetCursor();
    const Cursor& GetCursor() const;

    const SHORT GetFirstRowIndex() const;
    const COORD GetCoordBufferSize() const;

    void SetFirstRowIndex(const SHORT FirstRowIndex);
    void SetCoordBufferSize(const COORD coordBufferSize);

    UINT TotalRowCount() const;

    CHAR_INFO GetFill() const;
    void SetFill(const CHAR_INFO ciFill);

    [[nodiscard]]
    HRESULT ResizeTraditional(const COORD currentScreenBufferSize,
                              const COORD newScreenBufferSize,
                              const TextAttribute attributes);
private:

    std::deque<ROW> _storage;
    Cursor _cursor;

    SHORT _FirstRow; // indexes top row (not necessarily 0)

    COORD _coordBufferSize; // TODO: can we just measure the number of rows and/or their allocated width for this?

    FontInfo _fiCurrentFont;
    FontInfoDesired _fiDesiredFont;


    COORD GetPreviousFromCursor() const;

    void SetWrapOnCurrentRow();
    void AdjustWrapOnCurrentRow(const bool fSet);

    // Assist with maintaining proper buffer state for Double Byte character sequences
    bool _PrepareForDoubleByteSequence(const DbcsAttribute dbcsAttribute);
    bool AssertValidDoubleByteSequence(const DbcsAttribute dbcsAttribute);

    CHAR_INFO _ciFill;

#ifdef UNIT_TESTING
    friend class TextBufferTests;
#endif
};
