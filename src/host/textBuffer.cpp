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

    return short((pChar - this->Chars) + 1);
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

#pragma region TextAttribute

TextAttribute::TextAttribute()
{
    _wAttrLegacy = 0;
    _fUseRgbColor = false;
    _rgbForeground = RGB(0, 0, 0);
    _rgbBackground = RGB(0, 0, 0);
}

TextAttribute::TextAttribute(_In_ const WORD wLegacyAttr)
{
    _wAttrLegacy = wLegacyAttr;
    _fUseRgbColor = false;
    _rgbForeground = RGB(0, 0, 0);
    _rgbBackground = RGB(0, 0, 0);
}

TextAttribute::TextAttribute(_In_ const COLORREF rgbForeground, _In_ const COLORREF rgbBackground)
{
    _rgbForeground = rgbForeground;
    _rgbBackground = rgbBackground;
    _fUseRgbColor = true;
}

WORD TextAttribute::GetLegacyAttributes() const
{
    return _wAttrLegacy;
}

bool TextAttribute::IsLegacy() const
{
    return _fUseRgbColor == false;
}

COLORREF TextAttribute::GetRgbForeground() const
{
    return _IsReverseVideo()? _GetRgbBackground() : _GetRgbForeground();
}

COLORREF TextAttribute::GetRgbBackground() const
{
    return _IsReverseVideo()? _GetRgbForeground() : _GetRgbBackground();
}

COLORREF TextAttribute::_GetRgbForeground() const
{
    COLORREF rgbColor;
    if (_fUseRgbColor)
    {
        rgbColor = _rgbForeground;
    }
    else
    {
        const byte iColorTableIndex = LOBYTE(_wAttrLegacy) & 0x0F;

        ASSERT(iColorTableIndex >= 0);
        ASSERT(iColorTableIndex < g_ciConsoleInformation.GetColorTableSize());

        rgbColor = g_ciConsoleInformation.GetColorTable()[iColorTableIndex];
    }
    return rgbColor;
}

COLORREF TextAttribute::_GetRgbBackground() const
{
    COLORREF rgbColor;
    if (_fUseRgbColor)
    {
        rgbColor = _rgbBackground;
    }
    else
    {
        const byte iColorTableIndex = (LOBYTE(_wAttrLegacy) & 0xF0) >> 4;

        ASSERT(iColorTableIndex >= 0);
        ASSERT(iColorTableIndex < g_ciConsoleInformation.GetColorTableSize());

        rgbColor = g_ciConsoleInformation.GetColorTable()[iColorTableIndex];
    }
    return rgbColor;
}

void TextAttribute::SetFrom(_In_ const TextAttribute& otherAttr)
{
    *this = otherAttr;
}

void TextAttribute::SetFromLegacy(_In_ const WORD wLegacy)
{
    _wAttrLegacy = wLegacy;
    _fUseRgbColor = false;
}

void TextAttribute::SetForeground(_In_ const COLORREF rgbForeground)
{
    _rgbForeground = rgbForeground;
    if (!_fUseRgbColor)
    {
        _rgbBackground = _GetRgbBackground();
    }
    _fUseRgbColor = true;
}

void TextAttribute::SetBackground(_In_ const COLORREF rgbBackground)
{
    _rgbBackground = rgbBackground;
    if (!_fUseRgbColor)
    {
        _rgbForeground = _GetRgbForeground();
    }
    _fUseRgbColor = true;
}

void TextAttribute::SetColor(_In_ const COLORREF rgbColor, _In_ const bool fIsForeground)
{
    if (fIsForeground)
    {
        SetForeground(rgbColor);
    }
    else
    {
        SetBackground(rgbColor);
    }
}

bool TextAttribute::IsEqual(_In_ const TextAttribute& otherAttr) const
{
    return _wAttrLegacy == otherAttr._wAttrLegacy &&
           _fUseRgbColor == otherAttr._fUseRgbColor &&
           _rgbForeground == otherAttr._rgbForeground &&
           _rgbBackground == otherAttr._rgbBackground;
}

bool TextAttribute::IsEqualToLegacy(_In_ const WORD wLegacy) const
{
    return _wAttrLegacy == wLegacy && !_fUseRgbColor;
}

bool TextAttribute::_IsReverseVideo() const
{
    return IsFlagSet(_wAttrLegacy, COMMON_LVB_REVERSE_VIDEO);
}

bool TextAttribute::IsLeadingByte() const
{
    return IsFlagSet(_wAttrLegacy, COMMON_LVB_LEADING_BYTE);
}

bool TextAttribute::IsTrailingByte() const
{
    return IsFlagSet(_wAttrLegacy, COMMON_LVB_LEADING_BYTE);
}

bool TextAttribute::IsTopHorizontalDisplayed() const
{
    return IsFlagSet(_wAttrLegacy, COMMON_LVB_GRID_HORIZONTAL);
}

bool TextAttribute::IsBottomHorizontalDisplayed() const
{
    return IsFlagSet(_wAttrLegacy, COMMON_LVB_UNDERSCORE);
}

bool TextAttribute::IsLeftVerticalDisplayed() const
{
    return IsFlagSet(_wAttrLegacy, COMMON_LVB_GRID_LVERTICAL);
}

bool TextAttribute::IsRightVerticalDisplayed() const
{
    return IsFlagSet(_wAttrLegacy, COMMON_LVB_GRID_RVERTICAL);
}

#pragma endregion

#pragma region TextAttributeRun

UINT TextAttributeRun::GetLength()
{
    return _cchLength;
}

void TextAttributeRun::SetLength(_In_ UINT const cchLength)
{
    _cchLength = cchLength;
}

const TextAttribute TextAttributeRun::GetAttributes() const
{
    return _attributes;
}

void TextAttributeRun::SetAttributes(_In_ const TextAttribute textAttribute)
{
    _attributes.SetFrom(textAttribute);
}

#pragma endregion

#pragma region ATTR_ROW

// Routine Description:
// - Sets all properties of the ATTR_ROW to default values
// Arguments:
// - cchRowWidth - The width of the row.
// - pAttr - The default text attributes to use on text in this row.
// Return Value:
// - <none>
bool ATTR_ROW::Initialize(_In_ UINT const cchRowWidth, _In_ const TextAttribute attr)
{
    TextAttributeRun* pNewRun = new TextAttributeRun[1];
    bool fSuccess = pNewRun != nullptr;
    if (fSuccess)
    {
        if (this->GetHead() != nullptr)
        {
            delete[] this->GetHead();
        }
        this->_pAttribRunListHead = pNewRun;
        this->_pAttribRunListHead->SetAttributes(attr);
        this->_pAttribRunListHead->SetLength(cchRowWidth);
        this->Length = 1;
    }

    return fSuccess;
}

// Routine Description:
// - Takes an existing row of attributes, and changes the length so that it fills the sNewWidth.
//     If the new size is bigger, then the last attr is extended to fill the sNewWidth.
//     If the new size is smaller, the runs are cut off to fit.
// Arguments:
// - sOldWidth - The original width of the row.
// - sNewWidth - The new width of the row.
// Return Value:
// - <none>
bool ATTR_ROW::Resize(_In_ const short sOldWidth, _In_ const short sNewWidth)
{
    bool fSuccess = false;
    // Easy case. If the new row is longer, increase the length of the last run by how much new space there is.
    if (sNewWidth > sOldWidth)
    {
        TextAttributeRun* pIndexedRun;
        unsigned int CountOfAttr;
        this->FindAttrIndex((SHORT)(sOldWidth - 1), &pIndexedRun, &CountOfAttr);
        ASSERT(pIndexedRun <= &this->GetHead()[this->Length - 1]);
        pIndexedRun->SetLength(pIndexedRun->GetLength() + sNewWidth - sOldWidth);
        fSuccess = true;
    }
    // harder case: new row is shorter.
    else
    {
        // Unpack the row, then pack the row with the new width.
        // This will only store the runs up until the new width.
        TextAttribute* rAttrs = new TextAttribute[this->_TotalLength()];
        fSuccess = rAttrs != nullptr;
        if (fSuccess)
        {
            UnpackAttrs(rAttrs, this->Length);
            PackAttrs(rAttrs, sNewWidth);
            delete[] rAttrs;
            fSuccess = true;
        }
    }
    return fSuccess;
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
void ATTR_ROW::FindAttrIndex(_In_ UINT const uiIndex, _Outptr_ TextAttributeRun** const ppIndexedAttr, _Out_ UINT* const cAttrApplies) const
{
    ASSERT(uiIndex < this->_TotalLength()); // The requested index cannot be longer than the total length described by this set of Attrs.

    UINT cTotalLength = 0;
    UINT uiAttrsArrayPos;

    ASSERT(this->Length > 0); // There should be a non-zero and positive number of items in the array.

    // Scan through the internal array from position 0 adding up the lengths that each attribute applies to
    for (uiAttrsArrayPos = 0; uiAttrsArrayPos < (UINT)this->Length; uiAttrsArrayPos++)
    {
        cTotalLength += this->GetHead()[uiAttrsArrayPos].GetLength();

        if (cTotalLength > uiIndex)
        {
            // If we've just passed up the requested index with the length we added, break early
            break;
        }
    }

    // The leftover array position (uiAttrsArrayPos) stored at this point in time is the position of the attribute that is applicable at the position requested (uiIndex)
    // Save it off and calculate its remaining applicability
    *ppIndexedAttr = &this->GetHead()[uiAttrsArrayPos];
    // The length on which the found attribute applies is the total length seen so far minus the index we were searching for.
    ASSERT(cTotalLength > uiIndex); // The length of all attributes we counted up so far should be longer than the index requested or we'll underflow.

    *cAttrApplies = cTotalLength - uiIndex;

    ASSERT(*cAttrApplies > 0); // An attribute applies for >0 characters
    ASSERT(*cAttrApplies <= this->_TotalLength()); // An attribute applies for a maximum of the total length available to us

}

// Routine Description
// - Finds the total length covered by all attribute runs within this list
// Arguments:
// - <none>
// Return Value:
// - Total length of the row covered by all attributes in the encoding.
UINT ATTR_ROW::_TotalLength() const
{
    UINT cLength = 0;

    ASSERT(this->Length > 0); // There should be a non-zero and positive number of items in the array.

    for (UINT uiAttrIndex = 0; uiAttrIndex < (UINT)this->Length; uiAttrIndex++)
    {
        cLength += this->GetHead()[uiAttrIndex].GetLength();
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
NTSTATUS ATTR_ROW::UnpackAttrs(_Out_writes_(cRowLength) TextAttribute* const rgAttrs, _In_ UINT const cRowLength) const
{
    NTSTATUS status = STATUS_SUCCESS;

    unsigned short cTotalLength = 0;

    if (SUCCEEDED(UIntToUShort(this->_TotalLength(), &cTotalLength)))
    {
        if (cRowLength < cTotalLength)
        {
            status = STATUS_BUFFER_TOO_SMALL;
        }
    }
    else
    {
        ASSERT(false); // assert if math problem occurred
        status = STATUS_UNSUCCESSFUL;
    }

    if (NT_SUCCESS(status))
    {
        // hold a running count of the index position in the given output buffer
        unsigned short iOutputIndex = 0;
        bool fOutOfSpace = false;

        // Iterate through every packed ATTR_PAIR that is in our internal run length encoding
        for (unsigned short uiPackedIndex = 0; uiPackedIndex < this->Length; uiPackedIndex++)
        {
            // Pull out the length of the current run
            const unsigned int uiRunLength = this->GetHead()[uiPackedIndex].GetLength();

            // Fill the output array with the associated attribute for the current run length
            for (unsigned int uiRunCount = 0; uiRunCount < uiRunLength; uiRunCount++)
            {
                if (iOutputIndex >= cRowLength)
                {
                    fOutOfSpace = true;
                    break;
                }

                rgAttrs[iOutputIndex].SetFrom(this->GetHead()[uiPackedIndex].GetAttributes());

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
NTSTATUS ATTR_ROW::PackAttrs(_In_reads_(cRowLength) const TextAttribute* const rgAttrs, _In_ UINT const cRowLength)
{
    NTSTATUS status = STATUS_SUCCESS;

    if (cRowLength <= 0)
    {
        status = STATUS_BUFFER_TOO_SMALL;
    }

    if (NT_SUCCESS(status))
    {
        // first count up the deltas in the array
        int cDeltas = 1;

        const TextAttribute* pPrevAttr = &rgAttrs[0];

        for (unsigned int i = 1; i < cRowLength; i++)
        {
            const TextAttribute* pCurAttr = &rgAttrs[i];

            if (!pCurAttr->IsEqual(*pPrevAttr))
            {
                cDeltas++;
            }

            pPrevAttr = pCurAttr;
        }

        // This whole situation was too complicated with a one off holder for one row attr
        // new method:
        // delete the old buffer
        // make a new buffer, one run + one run for each change
        // set the values for each run one attr index at a time

        short cAttrLength;
        if (SUCCEEDED(IntToShort(cDeltas, &cAttrLength)))
        {
            delete[] this->_pAttribRunListHead;
            this->_pAttribRunListHead = new TextAttributeRun[cAttrLength];
            status = NT_TESTNULL(this->_pAttribRunListHead);
            if (NT_SUCCESS(status))
            {
                this->Length = cAttrLength;
                TextAttributeRun* pCurrentRun = this->GetHead();
                pCurrentRun->SetAttributes(rgAttrs[0]);
                pCurrentRun->SetLength(1);
                for (unsigned int i = 1; i < cRowLength; i++)
                {
                    if (pCurrentRun->GetAttributes().IsEqual(rgAttrs[i]))
                    {
                        pCurrentRun->SetLength(pCurrentRun->GetLength() + 1);
                    }
                    else
                    {
                        pCurrentRun++;
                        pCurrentRun->SetAttributes(rgAttrs[i]);
                        pCurrentRun->SetLength(1);
                    }
                }
            }
        }
        else
        {
            status = STATUS_UNSUCCESSFUL;
        }

    }

    return status;
}

// Routine Description:
// - Sets the attributes (colors) of all character positions from the given position through the end of the row.
// Arguments:
// - iStart - Starting index position within the row
// - attr - Attribute (color) to fill remaining characters with
// Return Value:
// - <none>
bool ATTR_ROW::SetAttrToEnd(_In_ UINT const iStart, _In_ const TextAttribute attr)
{
    // First get the information about the attribute applying at the current position
    TextAttributeRun* pIndexedAttrRun;
    UINT cAttrRunLengthFromIndex;
    this->FindAttrIndex(iStart, &pIndexedAttrRun, &cAttrRunLengthFromIndex);
    bool fSuccess = true;
    // Only proceed if we're trying to change it
    if (!pIndexedAttrRun->GetAttributes().IsEqual(attr))
    {
        // Unpack the old attributes
        const UINT cRowLength = this->_TotalLength();
        TextAttribute* rgAttrs = new TextAttribute[cRowLength];
        if (rgAttrs != nullptr)
        {
            this->UnpackAttrs(rgAttrs, cRowLength);

            ASSERT(iStart >= 0 && iStart < cRowLength);
            for (UINT i = iStart; i < cRowLength; i++)
            {
                rgAttrs[i].SetFrom(attr);
            }

            // Repack the attributes
            this->PackAttrs(rgAttrs, cRowLength);

            delete[] rgAttrs;
            fSuccess = true;
        }
        else
        {
            fSuccess = false;
        }
    }
    return fSuccess;
}

// Routine Description:
// - Replaces all runs in the row with the given wToBeReplacedAttr with the new attribute wReplaceWith.
// Arguments:
// - wToBeReplacedAttr - the legacy attribute to replace in this row.
// - wReplaceWith - the new value for the matching runs' attributes.
// Return Value:
// <none>
void ATTR_ROW::ReplaceLegacyAttrs(_In_ WORD wToBeReplacedAttr, _In_ WORD wReplaceWith)
{
    TextAttribute ToBeReplaced;
    ToBeReplaced.SetFromLegacy(wToBeReplacedAttr);

    TextAttribute ReplaceWith;
    ReplaceWith.SetFromLegacy(wReplaceWith);

    for (SHORT i = 0; i < Length; i++)
    {
        TextAttributeRun* pAttrRun = &(this->GetHead()[i]);
        if (pAttrRun->GetAttributes().IsEqual(ToBeReplaced))
        {
            pAttrRun->SetAttributes(ReplaceWith);
        }
    }
}

// Routine Description:
// - Sets the attributes of this run to the given legacy attributes
// Arguments:
// - wNew - the new value for this run's attributes
// Return Value:
// <none>
void TextAttributeRun::SetAttributesFromLegacy(_In_ const WORD wNew)
{
    _attributes.SetFromLegacy(wNew);
}

// Routine Description:
// - Returns a pointer to the start of this row's attribute runs
// Arguments:
// - <none>
// Return Value:
// - a pointer to the first TextAttributeRun in this row's AttrRow
TextAttributeRun* ATTR_ROW::GetHead() const
{
    return _pAttribRunListHead;
}

// Routine Description:
// - Takes a array of attribute runs, and inserts them into this row from startIndex to endIndex.
// - For example, if the current row was was [{4, BLUE}], the merge string
//   was [{ 2, RED }], with (StartIndex, EndIndex) = (1, 2),
//   then the row would modified to be = [{ 1, BLUE}, {2, RED}, {1, BLUE}].
// Arguments:
// - prgMergeAttrRuns - The array of attrRuns to merge into this row.
// - cMergeAttrRuns - The number of elements in prgMergeAttrRuns
// - sStartIndex - The index in the row to place the array of runs.
// - sEndIndex - the final index of the merge runs
// - BufferWidth - the width of the row.
// Return Value:
// - STATUS_NO_MEMORY if there wasn't enough memory to insert the runs
//   otherwise STATUS_SUCCESS if we were successful.
NTSTATUS ATTR_ROW::InsertAttrRuns(_In_reads_(cMergeAttrRuns) TextAttributeRun* prgMergeAttrRuns,
                                  _In_ const short cMergeAttrRuns,
                                  _In_ const short sStartIndex,
                                  _In_ const short sEndIndex,
                                  _In_ const short sBufferWidth)
{
    ASSERT(sEndIndex <= sBufferWidth);

    TextAttribute* rgUnpackedAttrs = new TextAttribute[sBufferWidth];
    NTSTATUS Status = NT_TESTNULL(rgUnpackedAttrs);
    if (NT_SUCCESS(Status))
    {
        Status = this->UnpackAttrs(rgUnpackedAttrs, sBufferWidth);
        if (NT_SUCCESS(Status))
        {
            short sCurrentX = sStartIndex;
            // For each attr run we're merging in...
            for (short sMergeIndex = 0; sMergeIndex < cMergeAttrRuns; sMergeIndex++)
            {
                TextAttributeRun* pAttrRun = &(prgMergeAttrRuns[sMergeIndex]);
                // ... loop over every spot in the run
                for (short sSpotInRun = 0;
                     ((UINT)sSpotInRun < pAttrRun->GetLength()) && (sSpotInRun+sCurrentX <= sEndIndex);
                     sSpotInRun++)
                {
                    // And place that atribute into the unpacked array.
                    rgUnpackedAttrs[sCurrentX + sSpotInRun].SetFrom(pAttrRun->GetAttributes());
                }
                // move past the current run.
                sCurrentX += (short) pAttrRun->GetLength();
            }

            Status = this->PackAttrs(rgUnpackedAttrs, sBufferWidth);
        }
        delete[] rgUnpackedAttrs;
    }
    return Status;
}

#pragma endregion

#pragma region ROW

// Routine Description:
// - Sets all properties of the ROW to default values
// Arguments:
// - sRowWidth - The width of the row.
// - Attr - The default attribute (color) to fill
// Return Value:
// - <none>
bool ROW::Initialize(_In_ short const sRowWidth, _In_ const TextAttribute Attr)
{
    this->CharRow.Initialize(sRowWidth);
    return this->AttrRow.Initialize(sRowWidth, Attr);

}
#pragma endregion

#pragma region TEXT_BUFFER_INFO

// Routine Description:
// - Constructor to set default properties for TEXT_BUFFER_INFO
TEXT_BUFFER_INFO::TEXT_BUFFER_INFO(_In_ const FontInfo* const pfiFont) :
    Rows(),
    TextRows(nullptr),
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
        delete[] this->TextRows;
    }

    if (this->Rows != nullptr)
    {
        for (int y = 0; y < this->_coordBufferSize.Y; y++)
        {
            if (this->Rows[y].AttrRow.GetHead() != nullptr)
            {
                delete this->Rows[y].AttrRow.GetHead();
            }
        }
        delete[] this->Rows;
    }

    if (this->KAttrRows != nullptr)
    {
        delete[] this->KAttrRows;
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

            pTextBufferInfo->Rows = new ROW[coordScreenBufferSize.Y];
            status = NT_TESTNULL(pTextBufferInfo->Rows);
            if (NT_SUCCESS(status))
            {
                pTextBufferInfo->TextRows = new WCHAR[coordScreenBufferSize.X * coordScreenBufferSize.Y];
                status = NT_TESTNULL(pTextBufferInfo->TextRows);
                if (NT_SUCCESS(status))
                {
                    pTextBufferInfo->KAttrRows = new BYTE[coordScreenBufferSize.X * coordScreenBufferSize.Y];
                    status = NT_TESTNULL(pTextBufferInfo->KAttrRows);
                    if (NT_SUCCESS(status))
                    {
                        PBYTE AttrRowPtr = pTextBufferInfo->KAttrRows;
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
                            TextAttribute FillAttributes;
                            FillAttributes.SetFromLegacy(ciFill.Attributes);
                            bool fSuccess = pRow->Initialize(coordScreenBufferSize.X, FillAttributes);
                            if (!fSuccess)
                            {
                                status = STATUS_NO_MEMORY;
                                break;
                            }
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
ROW* TEXT_BUFFER_INFO::GetFirstRow() const
{
    return GetRowByOffset(0);
}

//Routine Description:
// - Retrieves a row from the buffer by its offset from the top
//Arguments:
// - Number of rows down from the top of the buffer.
//Return Value:
// - Pointer to the requested row. Asserts if out of bounds.
ROW* TEXT_BUFFER_INFO::GetRowByOffset(_In_ UINT const rowIndex) const
{
    UINT const totalRows = this->_TotalRowCount();
    ROW* retVal = nullptr;

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
ROW* TEXT_BUFFER_INFO::GetPrevRowNoWrap(_In_ ROW* const pRow) const
{
    ROW* pReturnRow = nullptr;

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
ROW* TEXT_BUFFER_INFO::GetNextRowNoWrap(_In_ ROW* const pRow) const
{
    ROW* pReturnRow = nullptr;

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
// - true if we successfully prepared the buffer and moved the cursor
// - false otherwise (out of memory)
bool TEXT_BUFFER_INFO::_PrepareForDoubleByteSequence(_In_ BYTE const bKAttr)
{
    // Assert the buffer state is ready for this character
    // This function corrects most errors. If this is false, we had an uncorrectable one.
    bool const fValidSequence = AssertValidDoubleByteSequence(bKAttr);
    ASSERT(fValidSequence); // Shouldn't be uncorrectable sequences unless something is very wrong.
    UNREFERENCED_PARAMETER(fValidSequence);

    bool fSuccess = true;
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
            fSuccess = IncrementCursor();
        }
    }
    return fSuccess;
}

//Routine Description:
// - Inserts one character into the buffer at the current character position and advances the cursor as appropriate.
//Arguments:
// - wchChar - The character to insert
// - bKAttr - Double byte information associated with the charadcter
// - bAttr - Color data associated with the character
//Return Value:
// - true if we successfully inserted the character
// - false otherwise (out of memory)
bool TEXT_BUFFER_INFO::InsertCharacter(_In_ WCHAR const wch, _In_ BYTE const bKAttr, _In_ const TextAttribute attr)
{
    // Ensure consistent buffer state for double byte characters based on the character type we're about to insert
    bool fSuccess = _PrepareForDoubleByteSequence(bKAttr);

    if (fSuccess)
    {
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
        fSuccess = pRow->AttrRow.SetAttrToEnd(iCol, attr);
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

    // First, clean out the old "first row" as it will become the "last row" of the buffer after the circle is performed.
    TextAttribute FillAttributes;
    FillAttributes.SetFromLegacy(_ciFill.Attributes);
    bool fSuccess = this->Rows[this->_FirstRow].Initialize(this->_coordBufferSize.X, FillAttributes);
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

    ROW* pCurrRow = this->GetRowByOffset(coordEndOfText.Y);
    // The X position of the end of the valid text is the Right draw boundary (which is one beyond the final valid character)
    coordEndOfText.X = pCurrRow->CharRow.Right - 1;

    // If the X coordinate turns out to be -1, the row was empty, we need to search backwards for the real end of text.
    bool fDoBackUp = (coordEndOfText.X < 0 && coordEndOfText.Y > 0); // this row is empty, and we're not at the top
    while (fDoBackUp)
    {
        coordEndOfText.Y--;
        pCurrRow = this->GetRowByOffset(coordEndOfText.Y);
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

#pragma endregion

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

void TEXT_BUFFER_INFO::FreeExtraAttributeRows(_In_ const short sTopRowIndex, _In_ const short sOldHeight, _In_ const short sNewHeight)
{
    short i = (sTopRowIndex + sNewHeight) % sOldHeight;
    for (short j = sNewHeight; j < sOldHeight; j++)
    {
        delete[] this->Rows[i].AttrRow.GetHead();
        if (++i == sOldHeight)
        {
            i = 0;
        }
    }
}
