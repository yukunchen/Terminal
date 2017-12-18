/********************************************************
 *                                                       *
 *   Copyright (C) Microsoft. All rights reserved.       *
 *                                                       *
 ********************************************************/

#include "precomp.h"
#include "AttrRow.hpp"

// Routine Description:
// - swaps two ATTR_ROWs
// Arguments:
// - a - the first ATTR_ROW to swap
// - b - the second ATTR_ROW to swap
// Return Value:
// - <none>
void swap(ATTR_ROW& a, ATTR_ROW& b) noexcept
{
    a.swap(b);
}

// Routine Description:
// - constructor
// Arguments:
// - cchRowWidth - the length of the default text attribute
// - attr - the default text attribute
// Return Value:
// - constructed object
// Note: will throw exception if unable to allocate memory for text attribute storage
ATTR_ROW::ATTR_ROW(_In_ const UINT cchRowWidth, _In_ const TextAttribute attr)
{
    _rgList = wil::make_unique_nothrow<TextAttributeRun[]>(1);
    THROW_IF_NULL_ALLOC(_rgList.get());

    _rgList[0].SetAttributes(attr);
    _rgList[0].SetLength(cchRowWidth);
    _cList = 1;
    _cchRowWidth = cchRowWidth;
}

// Routine Description:
// - copy constructor
// Arguments:
// - object to copy
// Return Value:
// - copied object
// Note: will throw exception if unable to allocated memory
ATTR_ROW::ATTR_ROW(const ATTR_ROW& a) :
    _cList{ a._cList },
    _cchRowWidth{ a._cchRowWidth }
{
    _rgList = wil::make_unique_nothrow<TextAttributeRun[]>(_cList);
    THROW_IF_NULL_ALLOC(_rgList.get());
    std::copy(a._rgList.get(), a._rgList.get() + _cList, _rgList.get());
}

// Routine Description:
// - assignement operator overload
// Arguments:
// - a - the object to copy
// Return Value:
// - reference to this object
ATTR_ROW& ATTR_ROW::operator=(const ATTR_ROW& a)
{
    ATTR_ROW temp{ a };
    this->swap(temp);
    return *this;
}

// Routine Description:
// - swaps field of this object with another
// Arguments:
// - other - the other object to swap with
// Return Value:
// - <none>
void ATTR_ROW::swap(ATTR_ROW& other) noexcept
{
    using std::swap;
    swap(_cList, other._cList);
    swap(_rgList, other._rgList);
    swap(_cchRowWidth, other._cchRowWidth);
}

// Routine Description:
// - Sets all properties of the ATTR_ROW to default values
// Arguments:
// - cchRowWidth - The width of the row.
// - pAttr - The default text attributes to use on text in this row.
// Return Value:
// - <none>
bool ATTR_ROW::Reset(_In_ UINT const cchRowWidth, _In_ const TextAttribute attr)
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
// - S_OK on success, otherwise relevant error HRESULT
HRESULT ATTR_ROW::Resize(_In_ const short sOldWidth, _In_ const short sNewWidth)
{
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
    }
    return S_OK;
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
