/********************************************************
 *                                                       *
 *   Copyright (C) Microsoft. All rights reserved.       *
 *                                                       *
 ********************************************************/

#include "precomp.h"

#include "textBuffer.hpp"

#include "..\interactivity\inc\ServiceLocator.hpp"

#pragma hdrstop


// Routine Description:
// - Creates a new instance of TEXT_BUFFER_INFO
// Arguments:
// - pFontInfo - The font to use for this text buffer as specified in the global font cache
// - screenBufferSize - The X by Y dimensions of the new screen buffer
// - fill - Uses the .Attributes property to decide which default color to apply to all text in this buffer
// - cursorSize - The height of the cursor within this buffer
// Return Value:
// - constructed object
// Note: may throw exception
TEXT_BUFFER_INFO::TEXT_BUFFER_INFO(_In_ const FontInfo* const pFontInfo,
                                   _In_ const COORD screenBufferSize,
                                   _In_ const CHAR_INFO fill,
                                   _In_ const UINT cursorSize) :
    _fiCurrentFont{ *pFontInfo },
    _fiDesiredFont{ *pFontInfo },
    _FirstRow{ 0 },
    _ciFill{ fill },
    _coordBufferSize{ screenBufferSize },
    _pCursor{ nullptr },
    _storage{}
{
    THROW_IF_FAILED(HRESULT_FROM_NT(Cursor::CreateInstance(static_cast<ULONG>(cursorSize), &_pCursor)));

    // initialize ROWs
    for (size_t i = 0; i < static_cast<size_t>(screenBufferSize.Y); ++i)
    {
        TextAttribute FillAttributes;
        FillAttributes.SetFromLegacy(_ciFill.Attributes);
        _storage.emplace_back(static_cast<SHORT>(i), screenBufferSize.X, FillAttributes);
    }
}

// Routine Description:
// - Destructor to free memory associated with TEXT_BUFFER_INFO
// - NOTE: This will release font structures which may not have been allocated at CreateInstance time.
#pragma prefast(push)
#pragma prefast(disable:6001, "Prefast fires that *this is not initialized, which is absurd since this is a destructor.")
TEXT_BUFFER_INFO::~TEXT_BUFFER_INFO()
{
    if (this->_pCursor != nullptr)
    {
        delete this->_pCursor;
    }
}
#pragma prefast(pop)

// Routine Description:
// - Copies properties from another text buffer into this one.
// - This is primarily to copy properties that would otherwise not be specified during CreateInstance
// Arguments:
// - pOtherBuffer - The text buffer to copy properties from
// Return Value:
// - <none>
void TEXT_BUFFER_INFO::CopyProperties(_In_ TEXT_BUFFER_INFO* const pOtherBuffer)
{
    SetCurrentFont(pOtherBuffer->GetCurrentFont());

    this->GetCursor()->CopyProperties(pOtherBuffer->GetCursor());
}

void TEXT_BUFFER_INFO::SetCurrentFont(_In_ const FontInfo* const pfiNewFont)
{
    _fiCurrentFont = *pfiNewFont;
}

FontInfo* TEXT_BUFFER_INFO::GetCurrentFont()
{
    return &_fiCurrentFont;
}

void TEXT_BUFFER_INFO::SetDesiredFont(_In_ const FontInfoDesired* const pfiNewFont)
{
    _fiDesiredFont = *pfiNewFont;
}

FontInfoDesired* TEXT_BUFFER_INFO::GetDesiredFont()
{
    return &_fiDesiredFont;
}

//Routine Description:
// - The minimum text buffer width needed to hold all rows and not lose information.
//Arguments:
// - <none>
//Return Value:
// - Width required in character count.
short TEXT_BUFFER_INFO::GetMinBufferWidthNeeded() const
{
    short sMaxRight = 0;

    for (const ROW& row : _storage)
    {
        // note that .Right is always one position past the final character.
        // therefore a row with characters in array positions 0-19 will have a .Right of 20
        const SHORT sRowRight = row.CharRow.Right;

        if (sRowRight > sMaxRight)
        {
            sMaxRight = sRowRight;
        }
    }

    return sMaxRight;
}

// Routine Description:
// - Gets the number of rows in the buffer
// Arguments:
// - <none>
// Return Value:
// - Total number of rows in the buffer
UINT TEXT_BUFFER_INFO::TotalRowCount() const
{
    return static_cast<UINT>(_storage.size());
}

// Routine Description:
// - Retrieves the first row from the underlying buffer.
// Arguments:
// - <none>
// Return Value:
//  - const reference to the first row.
const ROW& TEXT_BUFFER_INFO::GetFirstRow() const
{
    return GetRowByOffset(0);
}

// Routine Description:
// - Retrieves the first row from the underlying buffer.
// Arguments:
// - <none>
// Return Value:
//  - reference to the first row.
ROW& TEXT_BUFFER_INFO::GetFirstRow()
{
    return const_cast<ROW&>(static_cast<const TEXT_BUFFER_INFO*>(this)->GetFirstRow());
}


// Routine Description:
// - Retrieves a row from the buffer by its offset from the first row of the text buffer (what corresponds to
// the top row of the screen buffer)
// Arguments:
// - Number of rows down from the first row of the buffer.
// Return Value:
// - const reference to the requested row. Asserts if out of bounds.
const ROW& TEXT_BUFFER_INFO::GetRowByOffset(_In_ const UINT index) const
{
    UINT const totalRows = this->TotalRowCount();
    ASSERT(index < totalRows);

    // Rows are stored circularly, so the index you ask for is offset by the start position and mod the total of rows.
    UINT const offsetIndex = (this->_FirstRow + index) % totalRows;
    return _storage[offsetIndex];
}

// Routine Description:
// - Retrieves a row from the buffer by its offset from the first row of the text buffer (what corresponds to
// the top row of the screen buffer)
// Arguments:
// - Number of rows down from the first row of the buffer.
// Return Value:
// - reference to the requested row. Asserts if out of bounds.
ROW& TEXT_BUFFER_INFO::GetRowByOffset(_In_ const UINT index)
{
    return const_cast<ROW&>(static_cast<const TEXT_BUFFER_INFO*>(this)->GetRowByOffset(index));
}

// Routine Description:
// - Retrieves the row that comes before the given row.
// - Does not wrap around the screen buffer.
// Arguments:
// - The current row.
// Return Value:
// - const reference to the previous row
// Note:
// - will throw exception if called with the first row of the text buffer
const ROW& TEXT_BUFFER_INFO::GetPrevRowNoWrap(_In_ const ROW& Row) const
{
    int prevRowIndex = Row.sRowId - 1;
    if (prevRowIndex < 0)
    {
        prevRowIndex = TotalRowCount() - 1;
    }

    THROW_HR_IF(E_FAIL, Row.sRowId == _FirstRow);
    return _storage[prevRowIndex];
}

// Routine Description:
// - Retrieves the row that comes before the given row.
// - Does not wrap around the screen buffer.
// Arguments:
// - The current row.
// Return Value:
// - reference to the previous row
// Note:
// - will throw exception if called with the first row of the text buffer
ROW& TEXT_BUFFER_INFO::GetPrevRowNoWrap(_In_ const ROW& Row)
{
    return const_cast<ROW&>(static_cast<const TEXT_BUFFER_INFO*>(this)->GetPrevRowNoWrap(Row));
}

// Routine Description:
// - Retrieves the row that comes after the given row.
// - Does not wrap around the screen buffer.
// Arguments:
// - The current row.
// Return Value:
// - const reference to the next row
// Note:
// - will throw exception if the row passed in is the last row of the screen buffer.
const ROW& TEXT_BUFFER_INFO::GetNextRowNoWrap(_In_ const ROW& row) const
{
    UINT nextRowIndex = row.sRowId + 1;
    UINT totalRows = this->TotalRowCount();

    if (nextRowIndex >= totalRows)
    {
        nextRowIndex = 0;
    }

    THROW_HR_IF(E_FAIL, nextRowIndex == static_cast<UINT>(_FirstRow));
    return _storage[nextRowIndex];
}

// Routine Description:
// - Retrieves the row that comes after the given row.
// - Does not wrap around the screen buffer.
// Arguments:
// - The current row.
// Return Value:
// - const reference to the next row
// Note:
// - will throw exception if the row passed in is the last row of the screen buffer.
ROW& TEXT_BUFFER_INFO::GetNextRowNoWrap(_In_ const ROW& row)
{
    return const_cast<ROW&>(static_cast<const TEXT_BUFFER_INFO*>(this)->GetNextRowNoWrap(row));
}

// Routine Description:
// - Retrieves the row at the specified index of the text buffer, without referring to which row is the first
// row of the screen buffer
// Arguments:
// - the index to fetch the row for
// Return Value:
// - const reference to the row
// Note:
// - will throw exception if the index passed would overflow the row storage
const ROW& TEXT_BUFFER_INFO::GetRowAtIndex(_In_ const UINT index) const
{
    if (index >= TotalRowCount())
    {
        THROW_HR(E_INVALIDARG);
    }
    return _storage[index];
}

// Routine Description:
// - Retrieves the row at the specified index of the text buffer, without referring to which row is the first
// row of the screen buffer
// Arguments:
// - the index to fetch the row for
// Return Value:
// - reference to the row
// Note:
// - will throw exception if the index passed would overflow the row storage
ROW& TEXT_BUFFER_INFO::GetRowAtIndex(_In_ const UINT index)
{
    return const_cast<ROW&>(static_cast<const TEXT_BUFFER_INFO*>(this)->GetRowAtIndex(index));
}

// Routine Description:
// - Retrieves the row previous to the one passed in.
// - will wrap around the buffer, so don't use in a loop.
// Arguments:
// - the row to fetch the previous row for.
// Return Value:
// - const reference to the previous row.
const ROW& TEXT_BUFFER_INFO::GetPrevRow(_In_ const ROW& row) const noexcept
{
    const SHORT rowIndex = row.sRowId;
    if (rowIndex == 0)
    {
        return _storage[TotalRowCount() - 1];
    }
    return _storage[rowIndex - 1];
}

// Routine Description:
// - Retrieves the row previous to the one passed in.
// - will wrap around the buffer, so don't use in a loop.
// Arguments:
// - the row to fetch the previous row for.
// Return Value:
// - reference to the previous row.
ROW& TEXT_BUFFER_INFO::GetPrevRow(_In_ const ROW& row) noexcept
{
    return const_cast<ROW&>(static_cast<const TEXT_BUFFER_INFO*>(this)->GetPrevRow(row));
}

// Routine Description:
// - Retrieves the row after the one passed in.
// - will wrap around the buffer, so don't use in a loop.
// Arguments:
// - the row to fetch the next row for.
// Return Value:
// - const reference to the next row.
const ROW& TEXT_BUFFER_INFO::GetNextRow(_In_ const ROW& row) const noexcept
{
    const UINT rowIndex = static_cast<UINT>(row.sRowId);
    if (rowIndex == TotalRowCount() - 1)
    {
        return _storage[0];
    }
    return _storage[rowIndex + 1];
}

// Routine Description:
// - Retrieves the row after the one passed in.
// - will wrap around the buffer, so don't use in a loop.
// Arguments:
// - the row to fetch the next row for.
// Return Value:
// - reference to the next row.
ROW& TEXT_BUFFER_INFO::GetNextRow(_In_ const ROW& row) noexcept
{
    return const_cast<ROW&>(static_cast<const TEXT_BUFFER_INFO*>(this)->GetNextRow(row));
}

//Routine Description:
// - Corrects and enforces consistent double byte character state (KAttrs line) within a row of the text buffer.
// - This will take the given double byte information and check that it will be consistent when inserted into the buffer
//   at the current cursor position.
// - It will correct the buffer (by erasing the character prior to the cursor) if necessary to make a consistent state.
//Arguments:
// - dbcsAttribute - Double byte information associated with the character about to be inserted into the buffer
//Return Value:
// - True if it is valid to insert a character with the given double byte attributes. False otherwise.
bool TEXT_BUFFER_INFO::AssertValidDoubleByteSequence(_In_ const DbcsAttribute dbcsAttribute)
{
    // To figure out if the sequence is valid, we have to look at the character that comes before the current one
    const COORD coordPrevPosition = GetPreviousFromCursor();
    ROW& prevRow = GetRowByOffset(coordPrevPosition.Y);
    const DbcsAttribute prevDbcsAttr = prevRow.CharRow.GetAttribute(coordPrevPosition.X);

    bool fValidSequence = true; // Valid until proven otherwise
    bool fCorrectableByErase = false; // Can't be corrected until proven otherwise

    // Here's the matrix of valid items:
    // N = None (single byte)
    // L = Lead (leading byte of double byte sequence
    // T = Trail (trailing byte of double byte sequence
    // Prev Curr    Result
    // N    N       OK.
    // N    L       OK.
    // N    T       Fail, uncorrectable. Trailing byte must have had leading before it.
    // L    N       Fail, OK with erase. Lead needs trailing pair. Can erase lead to correct.
    // L    L       Fail, OK with erase. Lead needs trailing pair. Can erase prev lead to correct.
    // L    T       OK.
    // T    N       OK.
    // T    L       OK.
    // T    T       Fail, uncorrectable. New trailing byte must have had leading before it.

    // Check for only failing portions of the matrix:
    if (prevDbcsAttr.IsSingle() && dbcsAttribute.IsTrailing())
    {
        // N, T failing case (uncorrectable)
        fValidSequence = false;
    }
    else if (prevDbcsAttr.IsLeading())
    {
        if (dbcsAttribute.IsSingle() || dbcsAttribute.IsLeading())
        {
            // L, N and L, L failing cases (correctable)
            fValidSequence = false;
            fCorrectableByErase = true;
        }
    }
    else if (prevDbcsAttr.IsTrailing() && dbcsAttribute.IsTrailing())
    {
        // T, T failing case (uncorrectable)
        fValidSequence = false;
    }

    // If it's correctable by erase, erase the previous character
    if (fCorrectableByErase)
    {
        // Erase previous character into an N type.
        prevRow.CharRow.Chars[coordPrevPosition.X] = PADDING_CHAR;
        prevRow.CharRow.GetAttribute(coordPrevPosition.X).SetSingle();

        // Sequence is now N N or N L, which are both okay. Set sequence back to valid.
        fValidSequence = true;
    }

    return fValidSequence;
}

//Routine Description:
// - Call before inserting a character into the buffer.
// - This will ensure a consistent double byte state (KAttrs line) within the text buffer
// - It will attempt to correct the buffer if we're inserting an unexpected double byte character type
//   and it will pad out the buffer if we're going to split a double byte sequence across two rows.
//Arguments:
// - dbcsAttribute - Double byte information associated with the character about to be inserted into the buffer
//Return Value:
// - true if we successfully prepared the buffer and moved the cursor
// - false otherwise (out of memory)
bool TEXT_BUFFER_INFO::_PrepareForDoubleByteSequence(_In_ const DbcsAttribute dbcsAttribute)
{
    // Assert the buffer state is ready for this character
    // This function corrects most errors. If this is false, we had an uncorrectable one.
    bool const fValidSequence = AssertValidDoubleByteSequence(dbcsAttribute);
    ASSERT(fValidSequence); // Shouldn't be uncorrectable sequences unless something is very wrong.
    UNREFERENCED_PARAMETER(fValidSequence);

    bool fSuccess = true;
    // Now compensate if we don't have enough space for the upcoming double byte sequence
    // We only need to compensate for leading bytes
    if (dbcsAttribute.IsLeading())
    {
        short const sBufferWidth = this->_coordBufferSize.X;

        // If we're about to lead on the last column in the row, we need to add a padding space
        if (this->GetCursor()->GetPosition().X == sBufferWidth - 1)
        {
            // set that we're wrapping for double byte reasons
            GetRowByOffset(this->GetCursor()->GetPosition().Y).CharRow.SetDoubleBytePadded(true);

            // then move the cursor forward and onto the next row
            fSuccess = IncrementCursor();
        }
    }
    return fSuccess;
}

//Routine Description:
// - Inserts one character into the buffer at the current character position and advances the cursor as appropriate.
//Arguments:
// - wchChar - The character to insert
// - dbcsAttribute - Double byte information associated with the charadcter
// - bAttr - Color data associated with the character
//Return Value:
// - true if we successfully inserted the character
// - false otherwise (out of memory)
bool TEXT_BUFFER_INFO::InsertCharacter(_In_ const wchar_t wch,
                                       _In_ const DbcsAttribute dbcsAttribute,
                                       _In_ const TextAttribute attr)
{
    // Ensure consistent buffer state for double byte characters based on the character type we're about to insert
    bool fSuccess = _PrepareForDoubleByteSequence(dbcsAttribute);

    if (fSuccess)
    {
        // Get the current cursor position
        short const iRow = this->GetCursor()->GetPosition().Y; // row stored as logical position, not array position
        short const iCol = this->GetCursor()->GetPosition().X; // column logical and array positions are equal.

        // Get the row associated with the given logical position
        ROW& Row = GetRowByOffset(iRow);

        // Store character and double byte data
        CHAR_ROW* const pCharRow = &Row.CharRow;
        short const cBufferWidth = this->_coordBufferSize.X;

        pCharRow->Chars[iCol] = wch;
        pCharRow->GetAttribute(iCol) = dbcsAttribute;

        // Update positioning
        if (wch == PADDING_CHAR)
        {
            // If we inserted the padding char, remeasure everything.
            pCharRow->MeasureAndSaveLeft(cBufferWidth);
            pCharRow->MeasureAndSaveRight(cBufferWidth);
        }
        else
        {
            // Otherwise the new right is just one past the column we inserted into.
            pCharRow->Right = iCol + 1;
        }

        // Store color data
        fSuccess = Row.AttrRow.SetAttrToEnd(iCol, attr);
        if (fSuccess)
        {
            // Advance the cursor
            fSuccess = this->IncrementCursor();
        }
    }
    return fSuccess;
}

//Routine Description:
// - Finds the current row in the buffer (as indicated by the cursor position)
//   and specifies that we have forced a line wrap on that row
//Arguments:
// - <none> - Always sets to wrap
//Return Value:
// - <none>
void TEXT_BUFFER_INFO::SetWrapOnCurrentRow()
{
    AdjustWrapOnCurrentRow(true);
}

//Routine Description:
// - Finds the current row in the buffer (as indicated by the cursor position)
//   and specifies whether or not it should have a line wrap flag.
//Arguments:
// - fSet - True if this row has a wrap. False otherwise.
//Return Value:
// - <none>
void TEXT_BUFFER_INFO::AdjustWrapOnCurrentRow(_In_ bool const fSet)
{
    // The vertical position of the cursor represents the current row we're manipulating.
    const UINT uiCurrentRowOffset = this->GetCursor()->GetPosition().Y;

    // Set the wrap status as appropriate
    GetRowByOffset(uiCurrentRowOffset).CharRow.SetWrapStatus(fSet);
}

//Routine Description:
// - Increments the cursor one position in the buffer as if text is being typed into the buffer.
// - NOTE: Will introduce a wrap marker if we run off the end of the current row
//Arguments:
// - <none>
//Return Value:
// - true if we successfully moved the cursor.
// - false otherwise (out of memory)
bool TEXT_BUFFER_INFO::IncrementCursor()
{
    // Cursor position is stored as logical array indices (starts at 0) for the window
    // Buffer Size is specified as the "length" of the array. It would say 80 for valid values of 0-79.
    // So subtract 1 from buffer size in each direction to find the index of the final column in the buffer

    ASSERT(this->_coordBufferSize.X > 0);
    const short iFinalColumnIndex = this->_coordBufferSize.X - 1;

    // Move the cursor one position to the right
    this->GetCursor()->IncrementXPosition(1);

    bool fSuccess = true;
    // If we've passed the final valid column...
    if (this->GetCursor()->GetPosition().X > iFinalColumnIndex)
    {
        // Then mark that we've been forced to wrap
        this->SetWrapOnCurrentRow();

        // Then move the cursor to a new line
        fSuccess = this->NewlineCursor();
    }
    return fSuccess;
}

//Routine Description:
// - Decrements the cursor one position in the buffer as if text is being backspaced out of the buffer.
// - NOTE: Will remove a wrap marker if it goes around a row
//Arguments:
// - <none>
//Return Value:
// - <none>
void TEXT_BUFFER_INFO::DecrementCursor()
{
    // Cursor position is stored as logical array indices (starts at 0) for the window
    // Buffer Size is specified as the "length" of the array. It would say 80 for valid values of 0-79.
    // So subtract 1 from buffer size in each direction to find the index of the final column in the buffer

    ASSERT(this->_coordBufferSize.X > 0);
    const short iFinalColumnIndex = this->_coordBufferSize.X - 1;

    // Move the cursor one position to the left
    this->GetCursor()->DecrementXPosition(1);

    // If we've passed the beginning of the line...
    if (this->GetCursor()->GetPosition().X < 0)
    {
        // Move us up a line
        this->GetCursor()->DecrementYPosition(1);

        // If we've moved past the top, move back down one and set X to 0.
        if (this->GetCursor()->GetPosition().Y < 0)
        {
            this->GetCursor()->IncrementYPosition(1);
            this->GetCursor()->SetXPosition(0);
        }
        else
        {
            // Set the X position to the end of the line.
            this->GetCursor()->SetXPosition(iFinalColumnIndex);

            // Then mark that we've backed around the wrap onto this new line and it's no longer a wrap.
            this->AdjustWrapOnCurrentRow(false);
        }
    }
}

//Routine Description:
// - Increments the cursor one line down in the buffer and to the beginning of the line
//Arguments:
// - <none>
//Return Value:
// - true if we successfully moved the cursor.
bool TEXT_BUFFER_INFO::NewlineCursor()
{
    bool fSuccess = false;
    ASSERT(this->_coordBufferSize.Y > 0);
    short const iFinalRowIndex = this->_coordBufferSize.Y - 1;

    // Reset the cursor position to 0 and move down one line
    this->GetCursor()->SetXPosition(0);
    this->GetCursor()->IncrementYPosition(1);

    // If we've passed the final valid row...
    if (this->GetCursor()->GetPosition().Y > iFinalRowIndex)
    {
        // Stay on the final logical/offset row of the buffer.
        this->GetCursor()->SetYPosition(iFinalRowIndex);

        // Instead increment the circular buffer to move us into the "oldest" row of the backing buffer
        fSuccess = this->IncrementCircularBuffer();
    }
    else
    {
        fSuccess = true;
    }
    return fSuccess;
}

//Routine Description:
// - Increments the circular buffer by one. Circular buffer is represented by FirstRow variable.
//Arguments:
// - <none>
//Return Value:
// - true if we successfully incremented the buffer.
bool TEXT_BUFFER_INFO::IncrementCircularBuffer()
{
    // FirstRow is at any given point in time the array index in the circular buffer that corresponds
    // to the logical position 0 in the window (cursor coordinates and all other coordinates).
    auto& g = ServiceLocator::LocateGlobals();
    if (g.pRender)
    {
        g.pRender->TriggerCircling();
    }

    // First, clean out the old "first row" as it will become the "last row" of the buffer after the circle is performed.
    TextAttribute FillAttributes;
    FillAttributes.SetFromLegacy(_ciFill.Attributes);
    bool fSuccess = _storage[_FirstRow].Reset(_coordBufferSize.X, FillAttributes);
    if (fSuccess)
    {
        // Now proceed to increment.
        // Incrementing it will cause the next line down to become the new "top" of the window (the new "0" in logical coordinates)
        this->_FirstRow++;

        // If we pass up the height of the buffer, loop back to 0.
        if (this->_FirstRow >= this->_coordBufferSize.Y)
        {
            this->_FirstRow = 0;
        }
    }
    return fSuccess;
}

//Routine Description:
// - Retrieves the position of the last non-space character on the final line of the text buffer.
//Arguments:
// - <none>
//Return Value:
// - Coordinate position in screen coordinates (offset coordinates, not array index coordinates).
COORD TEXT_BUFFER_INFO::GetLastNonSpaceCharacter() const
{
    COORD coordEndOfText;
    // Always search the whole buffer, by starting at the bottom.
    coordEndOfText.Y = _coordBufferSize.Y - 1;

    const ROW* pCurrRow = &GetRowByOffset(coordEndOfText.Y);
    // The X position of the end of the valid text is the Right draw boundary (which is one beyond the final valid character)
    coordEndOfText.X = pCurrRow->CharRow.Right - 1;

    // If the X coordinate turns out to be -1, the row was empty, we need to search backwards for the real end of text.
    bool fDoBackUp = (coordEndOfText.X < 0 && coordEndOfText.Y > 0); // this row is empty, and we're not at the top
    while (fDoBackUp)
    {
        coordEndOfText.Y--;
        pCurrRow = &GetRowByOffset(coordEndOfText.Y);
        // We need to back up to the previous row if this line is empty, AND there are more rows

        coordEndOfText.X = pCurrRow->CharRow.Right - 1;
        fDoBackUp = (coordEndOfText.X < 0 && coordEndOfText.Y > 0);
    }

    // don't allow negative results
    coordEndOfText.Y = max(coordEndOfText.Y, 0);
    coordEndOfText.X = max(coordEndOfText.X, 0);

    return coordEndOfText;
}

// Routine Description:
// - Retrieves the position of the previous character relative to the current cursor position
// Arguments:
// - <none>
// Return Value:
// - Coordinate position in screen coordinates of the character just before the cursor.
// - NOTE: Will return 0,0 if already in the top left corner
COORD TEXT_BUFFER_INFO::GetPreviousFromCursor() const
{
    COORD coordPosition = this->GetCursor()->GetPosition();

    // If we're not at the left edge, simply move the cursor to the left by one
    if (coordPosition.X > 0)
    {
        coordPosition.X--;
    }
    else
    {
        // Otherwise, only if we're not on the top row (e.g. we don't move anywhere in the top left corner. there is no previous)
        if (coordPosition.Y > 0)
        {
            // move the cursor to the right edge
            coordPosition.X = this->_coordBufferSize.X - 1;

            // and up one line
            coordPosition.Y--;
        }
    }

    return coordPosition;
}

const SHORT TEXT_BUFFER_INFO::GetFirstRowIndex() const
{
    return this->_FirstRow;
}
const COORD TEXT_BUFFER_INFO::GetCoordBufferSize() const
{
    return this->_coordBufferSize;
}

void TEXT_BUFFER_INFO::SetFirstRowIndex(_In_ SHORT const FirstRowIndex)
{
    this->_FirstRow = FirstRowIndex;
}
void TEXT_BUFFER_INFO::SetCoordBufferSize(_In_ COORD const coordBufferSize)
{
    this->_coordBufferSize = coordBufferSize;
}

Cursor* const TEXT_BUFFER_INFO::GetCursor() const
{
    return this->_pCursor;
}

CHAR_INFO TEXT_BUFFER_INFO::GetFill() const
{
    return _ciFill;
}

void TEXT_BUFFER_INFO::SetFill(_In_ const CHAR_INFO ciFill)
{
    _ciFill = ciFill;
}

// Routine Description:
// - This is the legacy screen resize with minimal changes
// Arguments:
// - currentScreenBufferSize - current size of the screen buffer.
// - newScreenBufferSize - new size of screen.
// - attributes - attributes to set for resized rows
// Return Value:
// - Success if successful. Invalid parameter if screen buffer size is unexpected. No memory if allocation failed.
NTSTATUS TEXT_BUFFER_INFO::ResizeTraditional(_In_ COORD const currentScreenBufferSize,
                                             _In_ COORD const newScreenBufferSize,
                                             _In_ TextAttribute const attributes)
{
    if (newScreenBufferSize.X < 0 || newScreenBufferSize.Y < 0)
    {
        RIPMSG2(RIP_WARNING, "Invalid screen buffer size (0x%x, 0x%x)", newScreenBufferSize.X, newScreenBufferSize.Y);
        return STATUS_INVALID_PARAMETER;
    }

    SHORT TopRow = 0; // new top row of the screen buffer
    if (newScreenBufferSize.Y <= GetCursor()->GetPosition().Y)
    {
        TopRow = GetCursor()->GetPosition().Y - newScreenBufferSize.Y + 1;
    }
    const SHORT TopRowIndex = (GetFirstRowIndex() + TopRow) % currentScreenBufferSize.Y;

    // rotate rows until the top row is at index 0
    try
    {
        const ROW& newTopRow = _storage[TopRowIndex];
        while (&newTopRow != &_storage.front())
        {
            _storage.push_back(std::move(_storage.front()));
            _storage.pop_front();
        }
    }
    catch (...)
    {
        return NTSTATUS_FROM_HRESULT(wil::ResultFromCaughtException());
    }
    SetFirstRowIndex(0);

    // realloc in the Y direction
    // remove rows if we're shrinking
    while (_storage.size() > static_cast<size_t>(newScreenBufferSize.Y))
    {
        _storage.pop_back();
    }
    // add rows if we're growing
    while (_storage.size() < static_cast<size_t>(newScreenBufferSize.Y))
    {
        try
        {
            _storage.emplace_back(static_cast<short>(_storage.size()), newScreenBufferSize.X, attributes);
        }
        catch (...)
        {
            return NTSTATUS_FROM_HRESULT(wil::ResultFromCaughtException());
        }
    }
    HRESULT hr = S_OK;
    for (SHORT i = 0; static_cast<size_t>(i) < _storage.size(); ++i)
    {
        // fix values for sRowId on each row
        _storage[i].sRowId = i;

        // realloc in the X direction
        hr = _storage[i].Resize(newScreenBufferSize.X);
        if (FAILED(hr))
        {
            return NTSTATUS_FROM_HRESULT(wil::ResultFromCaughtException());
        }
    }
    SetCoordBufferSize(newScreenBufferSize);
    return STATUS_SUCCESS;
}
