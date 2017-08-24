/********************************************************
 *                                                       *
 *   Copyright (C) Microsoft. All rights reserved.       *
 *                                                       *
 ********************************************************/

#include "precomp.h"

#include "textBuffer.hpp"

#include "..\interactivity\inc\ServiceLocator.hpp"

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
    const CONSOLE_INFORMATION* const gci = ServiceLocator::LocateGlobals()->getConsoleInformation();
    COLORREF rgbColor;
    if (_fUseRgbColor)
    {
        rgbColor = _rgbForeground;
    }
    else
    {
        const byte iColorTableIndex = LOBYTE(_wAttrLegacy) & 0x0F;

        ASSERT(iColorTableIndex >= 0);
        ASSERT(iColorTableIndex < gci->GetColorTableSize());

        rgbColor = gci->GetColorTable()[iColorTableIndex];
    }
    return rgbColor;
}

COLORREF TextAttribute::_GetRgbBackground() const
{
    const CONSOLE_INFORMATION* const gci = ServiceLocator::LocateGlobals()->getConsoleInformation();
    COLORREF rgbColor;
    if (_fUseRgbColor)
    {
        rgbColor = _rgbBackground;
    }
    else
    {
        const byte iColorTableIndex = (LOBYTE(_wAttrLegacy) & 0xF0) >> 4;

        ASSERT(iColorTableIndex >= 0);
        ASSERT(iColorTableIndex < gci->GetColorTableSize());

        rgbColor = gci->GetColorTable()[iColorTableIndex];
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

void TextAttribute::SetMetaAttributes(_In_ const WORD wMeta)
{
    UpdateFlagsInMask(_wAttrLegacy, META_ATTRS, wMeta);
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

UINT TextAttributeRun::GetLength() const
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
    wistd::unique_ptr<TextAttributeRun[]> pNewRun = wil::make_unique_nothrow<TextAttributeRun[]>(1);
    bool fSuccess = pNewRun != nullptr;
    if (fSuccess)
    {
        _rgList.swap(pNewRun);
        _rgList.get()->SetAttributes(attr);
        _rgList.get()->SetLength(cchRowWidth);
        _cList = 1;
        _cchRowWidth = cchRowWidth;
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
        // Get the attribute that covers the final column of old width.
        TextAttributeRun* pIndexedRun;
        unsigned int CountOfAttr;
        this->FindAttrIndex((SHORT)(sOldWidth - 1), &pIndexedRun, &CountOfAttr);
        ASSERT(pIndexedRun <= &_rgList[_cList - 1]);

        // Extend its length by the additional columns we're adding.
        pIndexedRun->SetLength(pIndexedRun->GetLength() + sNewWidth - sOldWidth);

        // Store that the new total width we represent is the new width.
        _cchRowWidth = sNewWidth;

        fSuccess = true;
    }
    // harder case: new row is shorter.
    else
    {
        // Get the attribute that covers the final column of the new width
        TextAttributeRun* pIndexedRun;
        unsigned int CountOfAttr;
        this->FindAttrIndex((SHORT)(sNewWidth - 1), &pIndexedRun, &CountOfAttr);
        ASSERT(pIndexedRun <= &_rgList[_cList - 1]);

        // CountOfAttr was given to us as "how many columns left from this point forward are covered by the returned run"
        // So if the original run was B5 covering a 5 size OldWidth and we have a NewWidth of 3
        // then when we called FindAttrIndex, it returned the B5 as the pIndexedRun and a 2 for how many more segments it covers
        // after and including the 3rd column.
        // B5-2 = B3, which is what we desire to cover the new 3 size buffer.
        pIndexedRun->SetLength(pIndexedRun->GetLength() - CountOfAttr + 1);

        // Store that the new total width we represent is the new width.
        _cchRowWidth = sNewWidth;

        // Adjust the number of valid segments to represent the one we just manipulated as the new end.
        // (+1 because this is pointer math of the last valid pos in the array minus the first valid pos in the array.)
        _cList = (UINT)(pIndexedRun - _rgList.get() + 1);

        // NOTE: Under some circumstances here, we have leftover run segments in memory or blank run segments
        // in memory. We're not going to waste time redimensioning the array in the heap. We're just noting that the useful
        // portions of it have changed.

        fSuccess = true;
    }
    return fSuccess;
}

// Routine Description:
// - This routine finds the nth attribute in this ATTR_ROW.
// Arguments:
// - uiIndex - which attribute to find
// - ppIndexedAttr - pointer to attribute within string
// - pcAttrApplies - on output, contains corrected length of indexed attr.
//                  for example, if the attribute string was { 5, BLUE } and the requested
//                  index was 3, CountOfAttr would be 2.
// Return Value:
// <none>
void ATTR_ROW::FindAttrIndex(_In_ UINT const uiIndex, 
                             _Outptr_ TextAttributeRun** const ppIndexedAttr, 
                             _Out_opt_ UINT* const pcAttrApplies) const
{
    ASSERT(uiIndex < _cchRowWidth); // The requested index cannot be longer than the total length described by this set of Attrs.

    UINT cTotalLength = 0;
    UINT uiAttrsArrayPos;

    ASSERT(this->_cList > 0); // There should be a non-zero and positive number of items in the array.

    // Scan through the internal array from position 0 adding up the lengths that each attribute applies to
    for (uiAttrsArrayPos = 0; uiAttrsArrayPos < (UINT)this->_cList; uiAttrsArrayPos++)
    {
        cTotalLength += _rgList[uiAttrsArrayPos].GetLength();

        if (cTotalLength > uiIndex)
        {
            // If we've just passed up the requested index with the length we added, break early
            break;
        }
    }

    // The leftover array position (uiAttrsArrayPos) stored at this point in time is the position of the attribute that is applicable at the position requested (uiIndex)
    // Save it off and calculate its remaining applicability
    *ppIndexedAttr = &_rgList[uiAttrsArrayPos];
    // The length on which the found attribute applies is the total length seen so far minus the index we were searching for.
    ASSERT(cTotalLength > uiIndex); // The length of all attributes we counted up so far should be longer than the index requested or we'll underflow.

    if (nullptr != pcAttrApplies)
    {
        *pcAttrApplies = cTotalLength - uiIndex;

        ASSERT(*pcAttrApplies > 0); // An attribute applies for >0 characters
        ASSERT(*pcAttrApplies <= _cchRowWidth); // An attribute applies for a maximum of the total length available to us
    }
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

    if (SUCCEEDED(UIntToUShort(_cchRowWidth, &cTotalLength)))
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
        for (unsigned short uiPackedIndex = 0; uiPackedIndex < _cList; uiPackedIndex++)
        {
            // Pull out the length of the current run
            const unsigned int uiRunLength = _rgList[uiPackedIndex].GetLength();

            // Fill the output array with the associated attribute for the current run length
            for (unsigned int uiRunCount = 0; uiRunCount < uiRunLength; uiRunCount++)
            {
                if (iOutputIndex >= cRowLength)
                {
                    fOutOfSpace = true;
                    break;
                }

                rgAttrs[iOutputIndex].SetFrom(_rgList[uiPackedIndex].GetAttributes());

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
// - Sets the attributes (colors) of all character positions from the given position through the end of the row.
// Arguments:
// - iStart - Starting index position within the row
// - attr - Attribute (color) to fill remaining characters with
// Return Value:
// - <none>
bool ATTR_ROW::SetAttrToEnd(_In_ UINT const iStart, _In_ const TextAttribute attr)
{
    UINT const iLength = _cchRowWidth - iStart;

    TextAttributeRun run;
    run.SetAttributes(attr);
    run.SetLength(iLength);

    return SUCCEEDED(InsertAttrRuns(&run, 1, iStart, _cchRowWidth - 1, _cchRowWidth));
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

    for (UINT i = 0; i < _cList; i++)
    {
        TextAttributeRun* pAttrRun = &(_rgList[i]);
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

#if DBG
// Routine Description:
// - Debug only method to help add up the total length of every run to help diagnose issues with 
//   manipulating run length encoding in-place.
// Arguments:
// - rgRun - The run to inspect
// - cRun - The length of the run (in array indexes)
// Return Value:
// - The length of the run (in total number of cells covered by adding up how much each segment covers.)
size_t _DebugGetTotalLength(_In_reads_(cRun) const TextAttributeRun* const rgRun,
                            size_t const cRun)
{
    size_t cTotal = 0;

    for (size_t i = 0; i < cRun; i++)
    {
        cTotal += rgRun[i].GetLength();
    }

    return cTotal;
}
#endif

// Routine Description:
// - Takes a array of attribute runs, and inserts them into this row from startIndex to endIndex.
// - For example, if the current row was was [{4, BLUE}], the merge string
//   was [{ 2, RED }], with (StartIndex, EndIndex) = (1, 2),
//   then the row would modified to be = [{ 1, BLUE}, {2, RED}, {1, BLUE}].
// Arguments:
// - rgInsertAttrs - The array of attrRuns to merge into this row.
// - cInsertAttrs - The number of elements in rgInsertAttrs
// - iStart - The index in the row to place the array of runs.
// - iEnd - the final index of the merge runs
// - BufferWidth - the width of the row.
// Return Value:
// - STATUS_NO_MEMORY if there wasn't enough memory to insert the runs
//   otherwise STATUS_SUCCESS if we were successful.
HRESULT ATTR_ROW::InsertAttrRuns(_In_reads_(cAttrs) const TextAttributeRun* const rgInsertAttrs,
                                  _In_ const UINT cInsertAttrs,
                                  _In_ const UINT iStart,
                                  _In_ const UINT iEnd,
                                  _In_ const UINT cBufferWidth)
{
    assert((iEnd - iStart + 1) == _DebugGetTotalLength(rgInsertAttrs, cInsertAttrs));

    // Definitions:
    // Existing Run = The run length encoded color array we're already storing in memory before this was called.
    // Insert Run = The run length encoded color array that someone is asking us to inject into our stored memory run.
    // New Run = The run length encoded color array that we have to allocate and rebuild to store internally
    //           which will replace Existing Run at the end of this function.
    // Example:
    // cBufferWidth = 10.
    // Existing Run: R3 -> G5 -> B2
    // Insert Run: Y1 -> N1 at iStart = 5 and iEnd = 6
    //            (rgInsertAttrs is a 2 length array with Y1->N1 in it and cInsertAttrs = 2)
    // Final Run: R3 -> G2 -> Y1 -> N1 -> G1 -> B2
    assert(cBufferWidth == _DebugGetTotalLength(_rgList.get(), _cList));

    // We'll need to know what the last valid column is for some calculations versus iEnd
    // because iEnd is specified to us as an inclusive index value.
    // Do the -1 math here now so we don't have to have -1s scattered all over this function.
    UINT const iLastBufferCol = cBufferWidth - 1;

    // Get the existing run that we'll be updating/manipulating.
    TextAttributeRun* const pExistingRun = _rgList.get();

    // If the insertion size is 1 and the existing run is 1, do some pre-processing to
    // see if we can get this done quickly.
    if (cInsertAttrs == 1 && _cList == 1)
    {
        // Get the new color attribute we're trying to apply
        TextAttribute const NewAttr = rgInsertAttrs[0].GetAttributes();

        // If the new color is the same as the old, we don't have to do anything and can exit quick.
        if (pExistingRun->GetAttributes().IsEqual(NewAttr))
        {
            return S_OK;
        }
        // If the new color is different, but applies for the entire width of the screen, we can just
        // fix up the existing Run to have the new color for the whole length very quickly.
        else if (iStart == 0 && iEnd == iLastBufferCol)
        {
            pExistingRun->SetAttributes(NewAttr);

            // We are assuming that if our existing run had only 1 item that it covered the entire buffer width.
            assert(pExistingRun->GetLength() == cBufferWidth); 
            pExistingRun->SetLength(cBufferWidth); // Set anyway to be safe.

            return S_OK;
        }
        // Else, fall down and keep trying other processing.
    }
    
    // In the worst case scenario, we will need a new run that is the length of
    // The existing run in memory + The new run in memory + 1.
    // This worst case occurs when we inject a new item in the middle of an existing run like so
    // Existing R3->B5->G2, Insertion Y2 starting at 5 (in the middle of the B5)
    // becomes R3->B2->Y2->B1->G2.
    // The original run was 3 long. The insertion run was 1 long. We need 1 more for the
    // fact that an existing piece of the run was split in half (to hold the latter half).
    UINT const cNewRun = _cList + cInsertAttrs + 1;
    wistd::unique_ptr<TextAttributeRun[]> pNewRun = wil::make_unique_nothrow<TextAttributeRun[]>(cNewRun);
    RETURN_IF_NULL_ALLOC(pNewRun);

    // We will start analyzing from the beginning of our existing run.
    // Use some pointers to keep track of where we are in walking through our runs.
    const TextAttributeRun* pExistingRunPos = pExistingRun;
    const TextAttributeRun* const pExistingRunEnd = pExistingRun + _cList;
    const TextAttributeRun* pInsertRunPos = rgInsertAttrs;
    UINT cInsertRunRemaining = cInsertAttrs;
    TextAttributeRun* pNewRunPos = pNewRun.get();
    UINT iExistingRunCoverage = 0;

    // Copy the existing run into the new buffer up to the "start index" where the new run will be injected.
    // If the new run starts at 0, we have nothing to copy from the beginning.
    if (iStart != 0)
    {
        // While we're less than the desired insertion position...
        while (iExistingRunCoverage < iStart)
        {
            // Add up how much length we can cover by copying an item from the existing run.
            iExistingRunCoverage += pExistingRunPos->GetLength();

            // Copy it to the new run buffer and advance both pointers.
            *pNewRunPos++ = *pExistingRunPos++;
        }

        // When we get to this point, we've copied full segments from the original existing run
        // into our new run buffer. We will have 1 or more full segments of color attributes and
        // we MIGHT have to cut the last copied segment's length back depending on where the inserted
        // attributes will fall in the final/new run.
        // Some examples:
        // - Starting with the original string R3 -> G5 -> B2
        // - 1. If the insertion is Y5 at start index 3 
        //      We are trying to get a result/final/new run of R3 -> Y5 -> B2.
        //      We just copied R3 to the new destination buffer and we can skip down and start inserting the new attrs.
        // - 2. If the insertion is Y3 at start index 5
        //      We are trying to get a result/final/new run of R3 -> G2 -> Y3 -> B2.
        //      We just copied R3 -> G5 to the new destination buffer with the code above.
        //      But the insertion is going to cut out some of the length of the G5. 
        //      We need to fix this up below so it says G2 instead to leave room for the Y3 to fit in
        //      the new/final run.

        // Copying above advanced the pointer to an empty cell beyond what we copied.
        // Back up one cell so we can manipulate the final item we copied from the existing run to the new run.
        pNewRunPos--;

        // Fetch out the length so we can fix it up based on the below conditions.
        UINT uiLength = pNewRunPos->GetLength();

        // If we've covered more cells already than the start of the attributes to be inserted...
        if (iExistingRunCoverage > iStart)
        {
            // ..then subtract some of the length of the final cell we copied.
            // We want to take remove the difference in distance between the cells we've covered in the new
            // run and the insertion point.
            // (This turns G5 into G2 from Example 2 just above)
            uiLength -= (iExistingRunCoverage - iStart);
        }

        // Now we're still on that "last cell copied" into the new run.
        // If the color of that existing copied cell matches the color of the first segment
        // of the run we're about to insert, we can just increment the length to extend the coverage.
        if (pNewRunPos->GetAttributes().IsEqual(pInsertRunPos->GetAttributes()))
        {
            uiLength += pInsertRunPos->GetLength();

            // Since the color matched, we have already "used up" part of the insert run
            // and can skip it in our big "memcopy" step below that will copy the bulk of the insert run.
            cInsertRunRemaining--;
            pInsertRunPos++;
        }

        // We're done manipulating the length. Store it back.
        pNewRunPos->SetLength(uiLength);

        // Now that we're done adjusting the last copied item, advance the pointer into a fresh/blank
        // part of the new run array.
        pNewRunPos++;
    }

    // Bulk copy the majority (or all, depending on circumstance) of the insert run into the final run buffer.
    CopyMemory(pNewRunPos, pInsertRunPos, cInsertRunRemaining * sizeof(TextAttributeRun));

    // Advance the new run pointer into the position just after everything we copied.
    pNewRunPos += cInsertRunRemaining;

    // We're technically done with the insert run now and have 0 remaining, but won't bother updating its pointers
    // and counts any further because we won't use them.

    // Now we need to move our pointer for the original existing run forward and update our counts
    // on how many cells we could have copied from the source before finishing off the new run.
    while (iExistingRunCoverage <= iEnd)
    {
        assert(pExistingRunPos != pExistingRunEnd);
        iExistingRunCoverage += pExistingRunPos->GetLength();
        pExistingRunPos++;
    }

    // If we still have original existing run cells remaining, copy them into the final new run.
    if (pExistingRunPos != pExistingRunEnd || iExistingRunCoverage != (iEnd + 1))
    {
        // Back up one cell so we can inspect the most recent item copied into the new run for optimizations.
        pNewRunPos--;

        // We advanced the existing run pointer and its count to on or past the end of what the insertion run filled in.
        // If this ended up being past the end of what the insertion run covers, we have to account for the cells after
        // the insertion run but before the next piece of the original existing run.
        // The example in this case is if we had...
        // Existing Run = R3 -> G5 -> B2 -> X5
        // Insert Run = Y2 @ iStart = 7 and iEnd = 8
        // ... then at this point in time, our states would look like...
        // New Run so far = R3 -> G4 -> Y2 
        // Existing Run Pointer is at X5 
        // Existing run coverage count at 3 + 5 + 2 = 10.
        // However, in order to get the final desired New Run 
        //   (which is R3 -> G4 -> Y2 -> B1 -> X5)
        //   we would need to grab a piece of that B2 we already skipped past.
        // iExistingRunCoverage = 10. iEnd = 8. iEnd+1 = 9. 10 > 9. So we skipped something.
        if (iExistingRunCoverage > (iEnd + 1))
        {
            // Back up the existing run pointer so we can grab the piece we skipped.
            pExistingRunPos--;

            // If the color matches what's already in our run, just increment the count value.
            // This case is slightly off from the example above. This case is for if the B2 above was actually Y2.
            // That Y2 from the existing run is the same color as the Y2 we just filled a few columns left in the final run
            // so we can just adjust the final run's column count instead of adding another segment here.
            if (pNewRunPos->GetAttributes().IsEqual(pExistingRunPos->GetAttributes()))
            {
                UINT uiLength = pNewRunPos->GetLength();
                uiLength += (iExistingRunCoverage - (iEnd + 1));
                pNewRunPos->SetLength(uiLength);
            }
            else
            {
                // If the color didn't match, then we just need to copy the piece we skipped and adjust
                // its length for the discrepency in columns not yet covered by the final/new run.

                // Move forward to a blank spot in the new run
                pNewRunPos++;

                // Copy the existing run's color information to the new run
                pNewRunPos->SetAttributes(pExistingRunPos->GetAttributes());
                
                // Adjust the length of that copied color to cover only the reduced number of columns needed
                // now that some have been replaced by the insert run.
                pNewRunPos->SetLength(iExistingRunCoverage - (iEnd + 1));
            }
            
            // Now that we're done recovering a piece of the existing run we skipped, move the pointer forward again.
            pExistingRunPos++;
        }

        // OK. In this case, we didn't skip anything. The end of the insert run fell right at a boundary
        // in columns that was in the original existing run.
        // However, the next piece of the original existing run might happen to have the same color attribute
        // as the final piece of what we just copied.
        // As an example...
        // Existing Run = R3 -> G5 -> B2.
        // Insert Run = B5 @ iStart = 3 and iEnd = 7
        // New Run so far = R3 -> B5
        // New Run desired when done = R3 -> B7
        // Existing run pointer is on B2.
        // We want to merge the 2 from the B2 into the B5 so we get B7.
        else if (pNewRunPos->GetAttributes().IsEqual(pExistingRunPos->GetAttributes()))
        {
            // Add the value from the existing run into the current new run position.
            UINT uiLength = pNewRunPos->GetLength();
            uiLength += pExistingRunPos->GetLength();
            pNewRunPos->SetLength(uiLength);

            // Advance the existing run position since we consumed its value and merged it in.
            pExistingRunPos++;
        }

        // OK. We're done inspecting the most recently copied cell for optimizations.
        assert(!(pNewRunPos - 1)->GetAttributes().IsEqual(pNewRunPos->GetAttributes()));
        pNewRunPos++;

        // Now bulk copy any segments left in the original existing run
        if (pExistingRunPos < pExistingRunEnd)
        {
            CopyMemory(pNewRunPos, pExistingRunPos, (pExistingRunEnd - pExistingRunPos) * sizeof(TextAttributeRun));

            // Fix up the end pointer so we know where we are for counting how much of the new run's memory space we used.
            pNewRunPos += (pExistingRunEnd - pExistingRunPos);
        }
    }

    // OK, phew. We're done. Now we just need to free the existing run, store the new run in its place,
    // and update the count for the correct length of the new run now that we've filled it up.

    size_t const cNew = (pNewRunPos - pNewRun.get());

    assert(cBufferWidth == _DebugGetTotalLength(pNewRun.get(), cNew));

    _cList = (UINT)cNew;
    _rgList.swap(pNewRun);
    
    return S_OK;
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
    _fiCurrentFont(*pfiFont),
    _fiDesiredFont(*pfiFont)
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
UINT TEXT_BUFFER_INFO::TotalRowCount() const
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
    UINT const totalRows = this->TotalRowCount();
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
    int totalRows = (int)this->TotalRowCount();

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
    UINT totalRows = this->TotalRowCount();

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
        if (++i == sOldHeight)
        {
            i = 0;
        }
    }
}
