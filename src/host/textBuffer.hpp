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
#include "../renderer/inc/FontInfo.hpp"
#include "../renderer/inc/FontInfoDesired.hpp"

#include "Row.hpp"
#include "TextAttribute.hpp"

#include <deque>
#include <memory>
#include <wil/resource.h>
#include <wil/wistd_memory.h>

class TEXT_BUFFER_INFO final
{
public:
    TEXT_BUFFER_INFO(_In_ const FontInfo* const pFontInfo,
                     _In_ const COORD screenBufferSize,
                     _In_ const CHAR_INFO fill,
                     _In_ const UINT cursorSize);
    TEXT_BUFFER_INFO(_In_ const TEXT_BUFFER_INFO& a) = delete;

    ~TEXT_BUFFER_INFO();

    // Used for duplicating properties to another text buffer
    void CopyProperties(_In_ TEXT_BUFFER_INFO* const pOtherBuffer);

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
    bool InsertCharacter(_In_ const wchar_t wch, _In_ const DbcsAttribute dbcsAttribute, _In_ const TextAttribute attr);
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

    [[nodiscard]]
    NTSTATUS ResizeTraditional(_In_ COORD const currentScreenBufferSize,
                               _In_ COORD const newScreenBufferSize,
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
    bool _PrepareForDoubleByteSequence(_In_ const DbcsAttribute dbcsAttribute);
    bool AssertValidDoubleByteSequence(_In_ const DbcsAttribute dbcsAttribute);

    CHAR_INFO _ciFill;

#ifdef UNIT_TESTING
    friend class TextBufferTests;
#endif
};
typedef TEXT_BUFFER_INFO *PTEXT_BUFFER_INFO;
typedef PTEXT_BUFFER_INFO *PPTEXT_BUFFER_INFO;
