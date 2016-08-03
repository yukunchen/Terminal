/********************************************************
 *                                                       *
 *   Copyright (C) Microsoft. All rights reserved.       *
 *                                                       *
 ********************************************************/

#include "precomp.h"

#include "textBuffer.hpp"
#include "consrv.h"
#include "globals.h"
#include "output.h"
#include "utils.hpp"

#pragma hdrstop

// Characters used for padding out the buffer with invalid/empty space
#define PADDING_CHAR UNICODE_SPACE
#define PADDING_KATTR '\0'

#pragma region CHAR_ROW

// Routine Description:
// - Sets all properties of the CHAR_ROW to default values
// Arguments:
// - sRowWidth - The width of the row.
// Return Value:
// - <none>
void CHAR_ROW::Initialize(_In_ short const sRowWidth)
{
    this->Left = sRowWidth;
    this->Right = 0;

    wmemset(this->Chars, PADDING_CHAR, sRowWidth);

    if (this->KAttrs != nullptr)
    {
        memset(this->KAttrs, PADDING_KATTR, sRowWidth);
    }

    this->SetWrapStatus(false);
    this->SetDoubleBytePadded(false);
}

// Routine Description:
// - Sets the wrap status for the current row
// Arguments:
// - fWrapWasForced - True if the row ran out of space and we forced to wrap to the next row. False otherwise.
// Return Value:
// - <none>
void CHAR_ROW::SetWrapStatus(_In_ bool const fWrapWasForced)
{
    if (fWrapWasForced)
    {
        this->bRowFlags |= RowFlags::WrapForced;
    }
    else
    {
        this->bRowFlags &= ~RowFlags::WrapForced;
    }
}

// Routine Description:
// - Gets the wrap status for the current row
// Arguments:
// - <none>
// Return Value:
// - True if the row ran out of space and we were forced to wrap to the next row. False otherwise.
bool CHAR_ROW::WasWrapForced() const
{
    return IsFlagSet((DWORD)this->bRowFlags, (DWORD)RowFlags::WrapForced);
}

// Routine Description:
// - Sets the double byte padding for the current row
// Arguments:
// - fWrapWasForced - True if the row ran out of space for a double byte character and we padded out the row. False otherwise.
// Return Value:
// - <none>
void CHAR_ROW::SetDoubleBytePadded(_In_ bool const fDoubleBytePadded)
{
    if (fDoubleBytePadded)
    {
        this->bRowFlags |= RowFlags::DoubleBytePadded;
    }
    else
    {
        this->bRowFlags &= ~RowFlags::DoubleBytePadded;
    }
}

// Routine Description:
// - Gets the double byte padding status for the current row.
// Arguments:
// - <none>
// Return Value:
// - True if the row didn't have space for a double byte character and we were padded out the row. False otherwise.
bool CHAR_ROW::WasDoubleBytePadded() const
{
    return IsFlagSet((DWORD)this->bRowFlags, (DWORD)RowFlags::DoubleBytePadded);
}

// Routine Description:
// - Inspects the current row contents and sets the Left/Right/OldLeft/OldRight boundary values as appropriate.
// Arguments:
// - sRowWidth - The width of the row.
// Return Value:
// - <none>
void CHAR_ROW::RemeasureBoundaryValues(_In_ short const sRowWidth)
{
    this->MeasureAndSaveLeft(sRowWidth);
    this->MeasureAndSaveRight(sRowWidth);
}

// Routine Description:
// - Inspects the current internal string to find the left edge of it
// Arguments:
// - sRowWidth - The width of the row.
// Return Value:
// - The calculated left boundary of the internal string.
short CHAR_ROW::MeasureLeft(_In_ short const sRowWidth) const
{
    PWCHAR pLastChar = &this->Chars[sRowWidth];
    PWCHAR pChar = this->Chars;

    for (; pChar < pLastChar && *pChar == PADDING_CHAR; pChar++)
    {
        /* do nothing */
    }

    return short(pChar - this->Chars);
}

// Routine Description:
// - Inspects the current internal string to find the right edge of it
// Arguments:
// - sRowWidth - The width of the row.
// Return Value:
// - The calculated right boundary of the internal string.
short CHAR_ROW::MeasureRight(_In_ short const sRowWidth) const
{
    PWCHAR pFirstChar = this->Chars;
    PWCHAR pChar = &this->Chars[sRowWidth - 1];

    for (; pChar >= pFirstChar && *pChar == PADDING_CHAR; pChar--)
    {
        /* do nothing */
    }

    return short(pChar - this->Chars + 1);
}

// Routine Description:
// - Updates the Left and OldLeft fields for this structure.
// Arguments:
// - sRowWidth - The width of the row.
// Return Value:
// - <none>
void CHAR_ROW::MeasureAndSaveLeft(_In_ short const sRowWidth)
{
    this->Left = MeasureLeft(sRowWidth);
}

// Routine Description:
// - Updates the Right and OldRight fields for this structure.
// Arguments:
// - sRowWidth - The width of the row.
// Return Value:
// - <none>
void CHAR_ROW::MeasureAndSaveRight(_In_ short const sRowWidth)
{
    this->Right = MeasureRight(sRowWidth);
}

// Routine Description:
// - Tells you whether or not this row contains any valid text.
// Arguments:
// - <none>
// Return Value:
// - True if there is valid text in this row. False otherwise.
bool CHAR_ROW::ContainsText() const
{
    return this->Right > this->Left;
}

// Routine Description:
// - Tells whether the given KAttribute is a leading byte marker
// Arguments:
// - bKAttr - KAttr bit flag field
// Return Value:
// - True if leading byte. False if not.
bool CHAR_ROW::IsLeadingByte(_In_ BYTE const bKAttr)
{
    return IsFlagSet((DWORD)bKAttr, (DWORD)CHAR_ROW::ATTR_LEADING_BYTE);
}

// Routine Description:
// - Tells whether the given KAttribute is a trailing byte marker
// Arguments:
// - bKAttr - KAttr bit flag field
// Return Value:
// - True if trailing byte. False if not.
bool CHAR_ROW::IsTrailingByte(_In_ BYTE const bKAttr)
{
    return IsFlagSet((DWORD)bKAttr, (DWORD)CHAR_ROW::ATTR_TRAILING_BYTE);
}

// Routine Description:
// - Tells whether the given KAttribute has no leading/trailing specification
// - e.g. Tells that this is a standalone, single-byte character
// Arguments:
// - bKAttr - KAttr bit flag field
// Return Value:
// - True if standalone single byte character. False otherwise.
bool CHAR_ROW::IsSingleByte(_In_ BYTE const bKAttr)
{
    return !IsLeadingByte(bKAttr) && !IsTrailingByte(bKAttr);
}

#pragma endregion

#pragma region ATTR_ROW

// Routine Description:
// - Sets all properties of the ATTR_ROW to default values
// Arguments:
// - sRowWidth - The width of the row.
// - wAttr - The default attribute (color) to fill
// Return Value:
// - <none>
void ATTR_ROW::Initialize(_In_ short const sRowWidth, _In_ WORD const wAttr)
{
    this->Length = 1;
    this->AttrPair.Length = sRowWidth;
    this->AttrPair.Attr = wAttr;
    this->Attrs = &this->AttrPair;
}

// Routine Description:
// - This routine finds the nth attribute in this ATTR_ROW.
// Arguments:
// - uiIndex - which attribute to find
// - ppIndexedAttr - pointer to attribute within string
// - cAttrApplies - on output, contains corrected length of indexed attr.
//                  for example, if the attribute string was { 5, BLUE } and the requested
//                  index was 3, CountOfAttr would be 2.
// Return Value:
// <none>
void ATTR_ROW::FindAttrIndex(_In_ UINT const uiIndex, _Outptr_ PPATTR_PAIR const ppIndexedAttr, _Out_ UINT* const cAttrApplies) const
{
    ASSERT(uiIndex < this->TotalLength()); // The requested index cannot be longer than the total length described by this set of Attrs.

    UINT cTotalLength = 0;
    UINT uiAttrsArrayPos;

    ASSERT(this->Length > 0); // There should be a non-zero and positive number of items in the array.

    // Scan through the internal array from position 0 adding up the lengths that each attribute applies to
    for (uiAttrsArrayPos = 0; uiAttrsArrayPos < (UINT)this->Length; uiAttrsArrayPos++)
    {
        cTotalLength += this->Attrs[uiAttrsArrayPos].Length;

        if (cTotalLength > uiIndex)
        {
            // If we've just passed up the requested index with the length we added, break early
            break;
        }
    }

    // The leftover array position (uiAttrsArrayPos) stored at this point in time is the position of the attribute that is applicable at the position requested (uiIndex)
    // Save it off and calculate its remaining applicability
    *ppIndexedAttr = &this->Attrs[uiAttrsArrayPos];

    // The length on which the found attribute applies is the total length seen so far minus the index we were searching for.
    ASSERT(cTotalLength > uiIndex); // The length of all attributes we counted up so far should be longer than the index requested or we'll underflow.

    *cAttrApplies = cTotalLength - uiIndex;

    ASSERT(*cAttrApplies > 0); // An attribute applies for >0 characters
    ASSERT(*cAttrApplies <= this->TotalLength()); // An attribute applies for a maximum of the total length available to us
}

// Routine Description
// - Finds the total length covered by all attribute runs within this list
// Arguments:
// - <none>
// Return Value:
// - Total length of the row covered by all attributes in the encoding.
UINT ATTR_ROW::TotalLength() const
{
    UINT cLength = 0;

    ASSERT(this->Length > 0); // There should be a non-zero and positive number of items in the array.

    for (UINT uiAttrIndex = 0; uiAttrIndex < (UINT)this->Length; uiAttrIndex++)
    {
        cLength += this->Attrs[uiAttrIndex].Length;
    }

    return cLength;
}

// Routine Description:
// - Unpacks run length encoded attributes into an array of words that is the width of the given row.
// Arguments:
// - rgAttrs - Preallocated array which will be filled with unpacked attributes
// - cRowLength - Length of this array
//  Return Value:
// - Success if unpacked. Buffer too small if row length is incorrect
NTSTATUS ATTR_ROW::UnpackAttrs(_Out_writes_(cRowLength) WORD* const rgAttrs, _In_ int const cRowLength) const
{
    NTSTATUS status = STATUS_SUCCESS;

    short cTotalLength = 0;

    if (!SUCCEEDED(UIntToShort(this->TotalLength(), &cTotalLength)))
    {
        status = STATUS_UNSUCCESSFUL;
    }

    if (NT_SUCCESS(status))
    {
        if (cRowLength < cTotalLength)
        {
            status = STATUS_BUFFER_TOO_SMALL;
        }
    }
    else
    {
        ASSERT(false); // assert if math problem occurred
    }

    if (NT_SUCCESS(status))
    {
        // hold a running count of the index position in the given output buffer
        short iOutputIndex = 0;
        bool fOutOfSpace = false;

        // Iterate through every packed ATTR_PAIR that is in our internal run length encoding
        for (short iPackedIndex = 0; iPackedIndex < this->Length; iPackedIndex++)
        {
            // Pull out the length of the current run
            const short iRunLength = this->Attrs[iPackedIndex].Length;

            // Fill the output array with the associated attribute for the current run length
            for (short iRunCount = 0; iRunCount < iRunLength; iRunCount++)
            {
                if (iOutputIndex >= cRowLength)
                {
                    fOutOfSpace = true;
                    break;
                }

                rgAttrs[iOutputIndex] = this->Attrs[iPackedIndex].Attr;

                // Increment output array index after every insertion.
                iOutputIndex++;
            }

            if (iOutputIndex >= cRowLength)
            {
                break;
            }
        }

        if (fOutOfSpace)
        {
            status = STATUS_BUFFER_TOO_SMALL;
        }
    }

    return status;
}

// Routine Description:
// - Packs an array of words representing attributes into the more compact storage form used by the row.
// Arguments:
// - rgAttrs - Array of words representing the attribute associated with each character position in the row.
// - cRowLength - Length of preceeding array.
// Return Value:
// - Success if success. Buffer too small if row length is incorrect.
NTSTATUS ATTR_ROW::PackAttrs(_In_reads_(cRowLength) const WORD* const rgAttrs, _In_ int const cRowLength)
{
    NTSTATUS status = STATUS_SUCCESS;

    if (cRowLength <= 0)
    {
        status = STATUS_BUFFER_TOO_SMALL;
    }

    if (NT_SUCCESS(status))
    {
        // first count up the deltas in the array
        int cDeltas = 0;

        WORD wPrevAttr = rgAttrs[0];

        for (int i = 1; i < cRowLength; i++)
        {
            WORD wCurAttr = rgAttrs[i];

            if (wCurAttr != wPrevAttr)
            {
                cDeltas++;
            }

            wPrevAttr = wCurAttr;
        }

        // free attrs array if it already exists as we're about to replace it
        // we only made an array here if the existing length was > 1
        // also check that the pointer is not null and is not pointing to the local attrpair
        if (this->Length > 1 && this->Attrs != nullptr && this->Attrs != &this->AttrPair)
        {
            ConsoleHeapFree(this->Attrs);
        }

        if (cDeltas == 0) // if no changes, then just pack the first one for the length
        {
            this->AttrPair.Attr = rgAttrs[0];
            this->AttrPair.Length = (short)cRowLength;
            this->Attrs = &this->AttrPair;
            this->Length = 1;
        }
        else // otherwise, allocate space and pack them all in
        {
            int cAttrLength = cDeltas + 1;

            this->Length = (short)cAttrLength;
            this->Attrs = (PATTR_PAIR)ConsoleHeapAlloc(SCREEN_TAG, cAttrLength * sizeof(ATTR_PAIR));

            status = NT_TESTNULL(this->Attrs);

            if (NT_SUCCESS(status))
            {
                PATTR_PAIR papCurrent = this->Attrs;
                papCurrent->Attr = rgAttrs[0];
                papCurrent->Length = 1;

                for (int i = 1; i < cRowLength; i++)
                {
                    if (papCurrent->Attr == rgAttrs[i])
                    {
                        papCurrent->Length++;
                    }
                    else
                    {
                        papCurrent++;
                        papCurrent->Attr = rgAttrs[i];
                        papCurrent->Length = 1;
                    }
                }
            }
        }
    }

    return status;
}

// Routine Description:
// - Sets the attributes (colors) of all character positions from the given position through the end of the row.
// Arguments:
// - sStartIndex - Starting index position within the row
// - wAttr - Attribute (color) to fill remaining characters with
// Return Value:
// - <none>
void ATTR_ROW::SetAttrToEnd(_In_ short const sStartIndex, _In_ WORD const wAttr)
{
    // First get the information about the attribute applying at the current position
    ATTR_PAIR* pAttrAtIndex;
    UINT cAttrRunLengthFromIndex;

    this->FindAttrIndex(sStartIndex, &pAttrAtIndex, &cAttrRunLengthFromIndex);

    // Only proceed if we're trying to change it
    if (wAttr != pAttrAtIndex->Attr)
    {
        // Unpack the old attributes
        const short cRowLength = (SHORT)this->TotalLength();
        WORD* rgAttrs = new WORD[cRowLength];

        this->UnpackAttrs(rgAttrs, cRowLength);

        ASSERT(sStartIndex >= 0 && sStartIndex < cRowLength);
        wmemset((wchar_t*)(rgAttrs + sStartIndex), wAttr, cRowLength - sStartIndex);

        // Repack the attributes
        this->PackAttrs(rgAttrs, cRowLength);
    }
}


#pragma endregion

#pragma region ROW

// Routine Description:
// - Sets all properties of the ROW to default values
// Arguments:
// - sRowWidth - The width of the row.
// - wAttr - The default attribute (color) to fill
// Return Value:
// - <none>
void ROW::Initialize(_In_ short const sRowWidth, _In_ const WORD wAttr)
{
    this->CharRow.Initialize(sRowWidth);
    this->AttrRow.Initialize(sRowWidth, wAttr);
}
#pragma endregion

#pragma region TEXT_BUFFER_INFO

// Routine Description:
// - Constructor to set default properties for TEXT_BUFFER_INFO
TEXT_BUFFER_INFO::TEXT_BUFFER_INFO(_In_ const FontInfo* const pfiFont) :
    Rows(),
    TextRows(nullptr),
    pDbcsScreenBuffer(nullptr),
    _fiCurrentFont(*pfiFont)
{

}

// Routine Description:
// - Destructor to free memory associated with TEXT_BUFFER_INFO
// - NOTE: This will release font structures which may not have been allocated at CreateInstance time.
#pragma prefast(push)
#pragma prefast(disable:6001, "Prefast fires that *this is not initialized, which is absurd since this is a destructor.")
TEXT_BUFFER_INFO::~TEXT_BUFFER_INFO()
{
    if (this->TextRows != nullptr)
    {
        ConsoleHeapFree(this->TextRows);
    }

    if (this->Rows != nullptr)
    {
        ConsoleHeapFree(this->Rows);
    }

    if (this->pDbcsScreenBuffer != nullptr)
    {
        delete this->pDbcsScreenBuffer;
    }

    if (this->_pCursor != nullptr)
    {
        delete this->_pCursor;
    }
}
#pragma prefast(pop)

// Routine Description:
// - Creates a new instance of TEXT_BUFFER_INFO
// Arguments:
// - nFont - The index of the font to use for this text buffer as specified in the global font cache
// - coordScreenBufferSize - The X by Y dimensions of the new screen buffer
// - ciFill - Uses the .Attributes property to decide which default color to apply to all text in this buffer
// - uiCursorSize - The height of the cursor within this buffer
// - pwszFaceName - (Optional) Used in conjunction with nFont to look up the appropriate font in the global font cache
// - ppTextBufferInfo - Pointer to accept the instance of the newly created text buffer
// Return Value:
// - Success or a relevant error status (usually out of memory).
NTSTATUS TEXT_BUFFER_INFO::CreateInstance(_In_ const FontInfo* const pFontInfo,
                                          _In_ COORD const coordScreenBufferSize,
                                          _In_ CHAR_INFO const ciFill,
                                          _In_ UINT const uiCursorSize,
                                          _Outptr_ PPTEXT_BUFFER_INFO const ppTextBufferInfo)
{
    *ppTextBufferInfo = nullptr;

    NTSTATUS status = STATUS_SUCCESS;

    PTEXT_BUFFER_INFO pTextBufferInfo = new TEXT_BUFFER_INFO(pFontInfo);

    status = NT_TESTNULL(pTextBufferInfo);
    if (NT_SUCCESS(status))
    {
        status = Cursor::CreateInstance((ULONG)uiCursorSize, &pTextBufferInfo->_pCursor);
        if (NT_SUCCESS(status))
        {
            // This has to come after the font is set because this function is dependent on the font info.
            // TODO: make this less prone to error by perhaps putting the storage of the first buffer font info as a part of TEXT_BUFFER_INFO's constructor

            pTextBufferInfo->_FirstRow = 0;

            pTextBufferInfo->_ciFill = ciFill;

            pTextBufferInfo->_coordBufferSize = coordScreenBufferSize;

            const size_t cbRows = coordScreenBufferSize.Y * sizeof(ROW);

            pTextBufferInfo->Rows = (PROW)ConsoleHeapAlloc(SCREEN_TAG, cbRows);

            status = NT_TESTNULL(pTextBufferInfo->Rows);
            if (NT_SUCCESS(status))
            {
                ZeroMemory(pTextBufferInfo->Rows, cbRows);

                pTextBufferInfo->TextRows = (PWCHAR)ConsoleHeapAlloc(SCREEN_TAG, coordScreenBufferSize.X * coordScreenBufferSize.Y * sizeof(WCHAR));

                status = NT_TESTNULL(pTextBufferInfo->TextRows);
                if (NT_SUCCESS(status))
                {
                    status = DBCS_SCREEN_BUFFER::CreateInstance(coordScreenBufferSize, &pTextBufferInfo->pDbcsScreenBuffer);

                    if (NT_SUCCESS(status))
                    {
                        PBYTE AttrRowPtr = pTextBufferInfo->pDbcsScreenBuffer->KAttrRows;
                        PWCHAR TextRowPtr = pTextBufferInfo->TextRows;

                        for (long i = 0; i < coordScreenBufferSize.Y; i++, TextRowPtr += coordScreenBufferSize.X)
                        {
                            ROW* const pRow = &pTextBufferInfo->Rows[i];

                            pRow->CharRow.Chars = TextRowPtr;
                            pRow->CharRow.KAttrs = AttrRowPtr;

                            if (AttrRowPtr)
                            {
                                AttrRowPtr += coordScreenBufferSize.X;
                            }

                            pRow->sRowId = (SHORT)i;

                            pRow->Initialize(coordScreenBufferSize.X, ciFill.Attributes);
                        }
                    }
                }
            }
        }
    }

    if (!NT_SUCCESS(status))
    {
        if (pTextBufferInfo != nullptr)
        {
            delete pTextBufferInfo;
        }
    }
    else
    {
        *ppTextBufferInfo = pTextBufferInfo;
    }

    return status;
}

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

//Routine Description:
// - The minimum text buffer width needed to hold all rows and not lose information.
//Arguments:
// - <none>
//Return Value:
// - Width required in character count.
short TEXT_BUFFER_INFO::GetMinBufferWidthNeeded() const
{
    short sMaxRight = 0;

    ROW* pRow = this->GetFirstRow();

    while (pRow != nullptr)
    {
        // note that .Right is always one position past the final character.
        // therefore a row with characters in array positions 0-19 will have a .Right of 20
        const SHORT sRowRight = pRow->CharRow.Right;

        if (sRowRight > sMaxRight)
        {
            sMaxRight = sRowRight;
        }

        pRow = this->GetNextRowNoWrap(pRow);
    }

    return sMaxRight;
}

#pragma region Row Manipulation

//Routine Description:
// - Gets the number of rows in the buffer
//Arguments:
// - <none>
//Return Value:
// - Total number of rows in the buffer
UINT TEXT_BUFFER_INFO::_TotalRowCount() const
{
    return _coordBufferSize.Y;
}

//Routine Description:
// -  Retrieves the first row from the underlying buffer.
//Arguments:
// - <none>
//Return Value:
//  - Pointer to the first row.
PROW TEXT_BUFFER_INFO::GetFirstRow() const
{
    return GetRowByOffset(0);
}

//Routine Description:
// - Retrieves a row from the buffer by its offset from the top
//Arguments:
// - Number of rows down from the top of the buffer.
//Return Value:
// - Pointer to the requested row. Asserts if out of bounds.
PROW TEXT_BUFFER_INFO::GetRowByOffset(_In_ UINT const rowIndex) const
{
    UINT const totalRows = this->_TotalRowCount();
    PROW retVal = nullptr;

    ASSERT(rowIndex < totalRows);

    // Rows are stored circularly, so the index you ask for is offset by the start position and mod the total of rows.
    UINT const offsetIndex = (this->_FirstRow + rowIndex) % totalRows;

    if (offsetIndex < totalRows)
    {
        retVal = &this->Rows[offsetIndex];
    }

    return retVal;
}

//Routine Description:
// - Retrieves the row that comes before the given row.
// - Does not wrap around the buffer.
//Arguments:
// - The current row.
//Return Value:
// - Pointer to the previous row, or nullptr if there is no previous row.
PROW TEXT_BUFFER_INFO::GetPrevRowNoWrap(_In_ PROW const pRow) const
{
    PROW pReturnRow = nullptr;

    int prevRowIndex = pRow->sRowId - 1;
    int totalRows = (int)this->_TotalRowCount();

    ASSERT(totalRows >= 0);

    if (prevRowIndex < 0)
    {
        prevRowIndex = totalRows - 1;
    }

    ASSERT(prevRowIndex >= 0 && prevRowIndex < (int)totalRows);

    // if the prev row would be before the first, we don't want to return anything to signify we've reached the end
    if (pRow->sRowId != this->_FirstRow)
    {
        pReturnRow = &this->Rows[prevRowIndex];
    }

    return pReturnRow;
}

//Routine Description:
// - Retrieves the row that comes after the given row.
// - Does not wrap around the buffer.
//Arguments:
// - The current row.
//Return Value:
// - Pointer to the next row, or nullptr if there is no next row.
PROW TEXT_BUFFER_INFO::GetNextRowNoWrap(_In_ PROW const pRow) const
{
    PROW pReturnRow = nullptr;

    UINT nextRowIndex = pRow->sRowId + 1;
    UINT totalRows = this->_TotalRowCount();

    if (nextRowIndex >= totalRows)
    {
        nextRowIndex = 0;
    }

    ASSERT(nextRowIndex < totalRows);

    // if the next row would be the first again, we don't want to return anything to signify we've reached the end
    if ((short)nextRowIndex != this->_FirstRow)
    {
        pReturnRow = &this->Rows[nextRowIndex];
    }

    return pReturnRow;
}

//Routine Description:
// - Corrects and enforces consistent double byte character state (KAttrs line) within a row of the text buffer.
// - This will take the given double byte information and check that it will be consistent when inserted into the buffer
//   at the current cursor position.
// - It will correct the buffer (by erasing the character prior to the cursor) if necessary to make a consistent state.
//Arguments:
// - bKAttr - Double byte information associated with the character about to be inserted into the buffer
//Return Value:
// - True if it is valid to insert a character with the given double byte attributes. False otherwise.
bool TEXT_BUFFER_INFO::AssertValidDoubleByteSequence(_In_ BYTE const bKAttr)
{
    // To figure out if the sequence is valid, we have to look at the character that comes before the current one
    const COORD coordPrevPosition = GetPreviousFromCursor();
    const ROW* pPrevRow = GetRowByOffset(coordPrevPosition.Y);

    // By default, assume it's a single byte character if no KAttrs data exists
    BYTE bPrevKAttr = CHAR_ROW::ATTR_SINGLE_BYTE;
    if (pPrevRow->CharRow.KAttrs != nullptr)
    {
        bPrevKAttr = pPrevRow->CharRow.KAttrs[coordPrevPosition.X];
    }

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
    if (CHAR_ROW::IsSingleByte(bPrevKAttr) && CHAR_ROW::IsTrailingByte(bKAttr))
    {
        // N, T failing case (uncorrectable)
        fValidSequence = false;
    }
    else if (CHAR_ROW::IsLeadingByte(bPrevKAttr))
    {
        if (CHAR_ROW::IsSingleByte(bKAttr) || CHAR_ROW::IsLeadingByte(bKAttr))
        {
            // L, N and L, L failing cases (correctable)
            fValidSequence = false;
            fCorrectableByErase = true;
        }
    }
    else if (CHAR_ROW::IsTrailingByte(bPrevKAttr) && CHAR_ROW::IsTrailingByte(bKAttr))
    {
        // T, T failing case (uncorrectable)
        fValidSequence = false;
    }

    // If it's correctable by erase, erase the previous character
    if (fCorrectableByErase)
    {
        // Erase previous character into an N type.
        pPrevRow->CharRow.Chars[coordPrevPosition.X] = PADDING_CHAR;

        if (pPrevRow->CharRow.KAttrs != nullptr)
        {
            pPrevRow->CharRow.KAttrs[coordPrevPosition.X] = PADDING_KATTR;
        }

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
// - bKAttr - Double byte information associated with the character about to be inserted into the buffer
//Return Value:
// - <none>
void TEXT_BUFFER_INFO::PrepareForDoubleByteSequence(_In_ BYTE const bKAttr)
{
    // Assert the buffer state is ready for this character
    // This function corrects most errors. If this is false, we had an uncorrectable one.
    bool const fValidSequence = AssertValidDoubleByteSequence(bKAttr);
    ASSERT(fValidSequence); // Shouldn't be uncorrectable sequences unless something is very wrong.
    UNREFERENCED_PARAMETER(fValidSequence);

    // Now compensate if we don't have enough space for the upcoming double byte sequence
    // We only need to compensate for leading bytes
    if (CHAR_ROW::IsLeadingByte(bKAttr))
    {
        short const sBufferWidth = this->_coordBufferSize.X;

        // If we're about to lead on the last column in the row, we need to add a padding space
        if (this->GetCursor()->GetPosition().X == sBufferWidth - 1)
        {
            // set that we're wrapping for double byte reasons
            GetRowByOffset(this->GetCursor()->GetPosition().Y)->CharRow.SetDoubleBytePadded(true);

            // then move the cursor forward and onto the next row
            IncrementCursor();
        }
    }
}

//Routine Description:
// - Inserts one character into the buffer at the current character position and advances the cursor as appropriate.
//Arguments:
// - wchChar - The character to insert
// - bKAttr - Double byte information associated with the charadcter
// - bAttr - Color data associated with the character
//Return Value:
// - <none>
void TEXT_BUFFER_INFO::InsertCharacter(_In_ WCHAR const wch, _In_ BYTE const bKAttr, _In_ WORD const wAttr)
{
    // Ensure consistent buffer state for double byte characters based on the character type we're about to insert
    PrepareForDoubleByteSequence(bKAttr);

    // Get the current cursor position
    short const iRow = this->GetCursor()->GetPosition().Y; // row stored as logical position, not array position
    short const iCol = this->GetCursor()->GetPosition().X; // column logical and array positions are equal.

    // Get the row associated with the given logical position
    ROW* const pRow = this->GetRowByOffset(iRow);

    // Store character and double byte data
    CHAR_ROW* const pCharRow = &pRow->CharRow;
    short const cBufferWidth = this->_coordBufferSize.X;

    pCharRow->Chars[iCol] = wch;
    if (pCharRow->KAttrs != nullptr)
    {
        pCharRow->KAttrs[iCol] = bKAttr;
    }

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
    pRow->AttrRow.SetAttrToEnd(iCol, wAttr);

    // Advance the cursor
    this->IncrementCursor();
}

//Routine Description:
// - Inserts a new line into the buffer and advances the character as appropriate.
//Arguments:
// - <none>
//Return Value:
// - <none>
void TEXT_BUFFER_INFO::InsertNewline()
{
    // Move cursor to the new line position. No other action needed.
    this->NewlineCursor();
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

    // Translate the offset position (the logical position within the window where 0 is the top of the window)
    // into the circular buffer position (rows array index) using the helper function.
    ROW* const pCurrentRow = this->GetRowByOffset(uiCurrentRowOffset);

    // Set the wrap status as appropriate
    pCurrentRow->CharRow.SetWrapStatus(fSet);
}

//Routine Description:
// - Increments the cursor one position in the buffer as if text is being typed into the buffer.
// - NOTE: Will introduce a wrap marker if we run off the end of the current row
//Arguments:
// - <none>
//Return Value:
// - <none>
void TEXT_BUFFER_INFO::IncrementCursor()
{
    // Cursor position is stored as logical array indices (starts at 0) for the window
    // Buffer Size is specified as the "length" of the array. It would say 80 for valid values of 0-79.
    // So subtract 1 from buffer size in each direction to find the index of the final column in the buffer

    ASSERT(this->_coordBufferSize.X > 0);
    const short iFinalColumnIndex = this->_coordBufferSize.X - 1;

    // Move the cursor one position to the right
    this->GetCursor()->IncrementXPosition(1);

    // If we've passed the final valid column...
    if (this->GetCursor()->GetPosition().X > iFinalColumnIndex)
    {
        // Then mark that we've been forced to wrap
        this->SetWrapOnCurrentRow();

        // Then move the cursor to a new line
        this->NewlineCursor();
    }
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
// - <none>
void TEXT_BUFFER_INFO::NewlineCursor()
{
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
        this->IncrementCircularBuffer();
    }
}

//Routine Description:
// - Increments the circular buffer by one. Circular buffer is represented by FirstRow variable.
//Arguments:
// - <none>
//Return Value:
// - <none>
void TEXT_BUFFER_INFO::IncrementCircularBuffer()
{
    // FirstRow is at any given point in time the array index in the circular buffer that corresponds
    // to the logical position 0 in the window (cursor coordinates and all other coordinates).

    // First, clean out the old "first row" as it will become the "last row" of the buffer after the circle is performed.
    this->Rows[this->_FirstRow].Initialize(this->_coordBufferSize.X, this->_ciFill.Attributes);

    // Now proceed to increment.
    // Incrementing it will cause the next line down to become the new "top" of the window (the new "0" in logical coordinates)
    this->_FirstRow++;

    // If we pass up the height of the buffer, loop back to 0.
    if (this->_FirstRow >= this->_coordBufferSize.Y)
    {
        this->_FirstRow = 0;
    }
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
    coordEndOfText.Y = this->GetCursor()->GetPosition().Y;

    // The X position of the end of the valid text is the Right draw boundary (which is one beyond the final valid character)
    coordEndOfText.X = this->GetRowByOffset(coordEndOfText.Y)->CharRow.Right - 1;

    // If the X coordinate turns out to be -1, the row was empty, we need to search backwards for the real end of text.
    while (coordEndOfText.X < 0 && coordEndOfText.Y > 0)
    {
        coordEndOfText.Y--;
        coordEndOfText.X = this->GetRowByOffset(coordEndOfText.Y)->CharRow.Right - 1;
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

#pragma endregion

#pragma endregion

#pragma region DBCS_SCREEN_BUFFER

// Routine Description:
// - Creates a new instance of DBCS_SCREEN_BUFFER
// Arguments:
// - dwScreenBufferSize - The X by Y dimensions of the new buffer
// - ppDbcsScreenBuffer - Pointer to accept the instance of the newly created buffer
// Return Value:
// - Success or a relevant error status (usually out of memory).
NTSTATUS DBCS_SCREEN_BUFFER::CreateInstance(_In_ COORD const dwScreenBufferSize,
                                            _Outptr_ DBCS_SCREEN_BUFFER ** const ppDbcsScreenBuffer)
{
    *ppDbcsScreenBuffer = nullptr;

    PDBCS_SCREEN_BUFFER pDbcsScreenBuffer = new DBCS_SCREEN_BUFFER();
    NTSTATUS status = NT_TESTNULL(pDbcsScreenBuffer);
    if (NT_SUCCESS(status))
    {
        pDbcsScreenBuffer->KAttrRows = (PBYTE)ConsoleHeapAlloc(SCREEN_DBCS_TAG,
                                                               dwScreenBufferSize.X * dwScreenBufferSize.Y * sizeof(BYTE));
        status = NT_TESTNULL(pDbcsScreenBuffer->KAttrRows);
    }

    if (!NT_SUCCESS(status))
    {
        delete pDbcsScreenBuffer;
    }
    else
    {
        *ppDbcsScreenBuffer = pDbcsScreenBuffer;
    }

    return status;
}

// Routine Description:
// - Constructor to set default properties for DBCS_SCREEN_BUFFER
DBCS_SCREEN_BUFFER::DBCS_SCREEN_BUFFER()
{
}

// Routine Description:
// - Destructor to free memory associated with DBCS_SCREEN_BUFFER
DBCS_SCREEN_BUFFER::~DBCS_SCREEN_BUFFER()
{
    if (this->KAttrRows != nullptr)
    {
        ConsoleHeapFree(this->KAttrRows);
        this->KAttrRows = nullptr;
    }
}

#pragma endregion

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
