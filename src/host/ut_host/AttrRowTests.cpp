/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/
#include "precomp.h"
#include "WexTestClass.h"

#include "CommonState.hpp"

#include "globals.h"
#include "textBuffer.hpp"

#include "consrv.h"

#include "input.h"

using namespace WEX::Logging;
using namespace WEX::TestExecution;

class AttrRowTests
{
    ATTR_ROW* pSingle;
    ATTR_ROW* pChain;

    short _sDefaultLength = 80;
    short _sDefaultChainLength = 6;

    short sChainSegLength;
    short sChainLeftover;
    short sChainSegmentsNeeded;

    WORD __wDefaultAttr = FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED;
    WORD __wDefaultChainAttr = BACKGROUND_BLUE | BACKGROUND_GREEN | BACKGROUND_RED | BACKGROUND_INTENSITY;
    TextAttribute _DefaultAttr = TextAttribute(__wDefaultAttr);
    TextAttribute _DefaultChainAttr = TextAttribute(__wDefaultChainAttr);

    TEST_CLASS(AttrRowTests);

    TEST_METHOD_SETUP(MethodSetup)
    {
        pSingle = new ATTR_ROW();
        pSingle->Initialize(_sDefaultLength, _DefaultAttr);

        // Segment length is the expected length divided by the row length
        // E.g. row of 80, 4 segments, 20 segment length each
        sChainSegLength = _sDefaultLength / _sDefaultChainLength;

        // Leftover is spaces that don't fit evenly
        // E.g. row of 81, 4 segments, 1 leftover length segment
        sChainLeftover = _sDefaultLength % _sDefaultChainLength;

        // Start with the number of segments we expect
        sChainSegmentsNeeded = _sDefaultChainLength;

        // If we had a remainder, add one more segment
        if (sChainLeftover)
        {
            sChainSegmentsNeeded++;
        }

        // Create the chain
        pChain = new ATTR_ROW();
        pChain->Length = sChainSegmentsNeeded;
        pChain->_pAttribRunListHead = new TextAttributeRun[sChainSegmentsNeeded];

        // Attach all chain segments that are even multiples of the row length
        for (short iChain = 0; iChain < _sDefaultChainLength; iChain++)
        {
            TextAttributeRun* pRun = &pChain->GetHead()[iChain];

            pRun->SetAttributesFromLegacy(iChain); // Just use the chain position as the value
            pRun->SetLength(sChainSegLength);
        }

        if (sChainLeftover > 0)
        {
            // If we had a leftover, then this chain is one longer than we expected (the default length)
            // So use it as the index (because indicies start at 0)
            TextAttributeRun* pRun = &pChain->GetHead()[_sDefaultChainLength];

            pRun->SetAttributes(_DefaultChainAttr);
            pRun->SetLength(sChainLeftover);
        }

        return true;
    }

    TEST_METHOD_CLEANUP(MethodCleanup)
    {
        delete pSingle;

        delete pChain;

        return true;
    }

    TEST_METHOD(TestInitialize)
    {
        // Properties needed for test
        const short sRowWidth = 37;
        const WORD wAttr = FOREGROUND_RED | BACKGROUND_BLUE;
        TextAttribute attr = TextAttribute(wAttr);
        // Cases to test
        ATTR_ROW* pTestItems[] { pSingle, pChain };

        // Loop cases
        for (UINT iIndex = 0; iIndex < ARRAYSIZE(pTestItems); iIndex++)
        {
            ATTR_ROW* pUnderTest = pTestItems[iIndex];

            pUnderTest->Initialize(sRowWidth, attr);

            VERIFY_ARE_EQUAL(pUnderTest->Length, 1);
            VERIFY_IS_TRUE(pUnderTest->GetHead()->GetAttributes().IsEqual(attr));
            VERIFY_ARE_EQUAL(pUnderTest->GetHead()[0].GetLength(), (unsigned int)sRowWidth);
        }
    }

    TEST_METHOD(TestUnpackAttrs)
    {
        TextAttribute* rAttrs;
        UINT cAttrs;

        Log::Comment(L"Checking unpack of a single color for the entire length");
        cAttrs = pSingle->_TotalLength();
        rAttrs = new TextAttribute[cAttrs];

        pSingle->UnpackAttrs(rAttrs, (int)cAttrs);

        for (UINT iAttrIndex = 0; iAttrIndex < cAttrs; iAttrIndex++)
        {
            VERIFY_IS_TRUE(rAttrs[iAttrIndex].IsEqual(_DefaultAttr));
        }

        delete[] rAttrs;

        Log::Comment(L"Checking unpack of the multiple color chain");
        cAttrs = pChain->_TotalLength();
        rAttrs = new TextAttribute[cAttrs];

        pChain->UnpackAttrs(rAttrs, cAttrs);

        short cChainRun = 0; // how long we've been looking at the current piece of the chain
        short iChainSegIndex = 0; // which piece of the chain we should be on right now

        for (UINT iAttrIndex = 0; iAttrIndex < cAttrs; iAttrIndex++)
        {
            // by default the chain was assembled above to have the chain segment index be the attribute
            TextAttribute MatchingAttr = TextAttribute(iChainSegIndex);

            // However, if the index is greater than the expected chain length, a remainder piece was made with a default attribute
            if (iChainSegIndex >= _sDefaultChainLength)
            {
                MatchingAttr = _DefaultChainAttr;
            }

            VERIFY_IS_TRUE(rAttrs[iAttrIndex].IsEqual(MatchingAttr));

            // Add to the chain run
            cChainRun++;

            // If the chain run is greater than the length the segments were specified to be
            if (cChainRun >= sChainSegLength)
            {
                // reset to 0
                cChainRun = 0;

                // move to the next chain segment down the line
                iChainSegIndex++;
            }
        }

        delete[] rAttrs;
    }

    TEST_METHOD(TestPackAttrs)
    {
        Log::Comment(L"Test turning chain into a single attr");
        UINT cAttrs = pChain->_TotalLength();
        TextAttribute* rgAttrs = new TextAttribute[cAttrs];

        for (UINT iAttr = 0; iAttr < cAttrs; iAttr++)
        {
            rgAttrs[iAttr] = _DefaultAttr;
        }

        pChain->PackAttrs(rgAttrs, (int)cAttrs);

        // after packing, old chain data should be discarded and we should be down to...

        VERIFY_ARE_EQUAL(pChain->Length, 1); // 1 item
        VERIFY_IS_TRUE(pChain->GetHead()->GetAttributes().IsEqual(_DefaultAttr)); // has the attribute we specified for every cell in the array
        VERIFY_ARE_EQUAL(pChain->GetHead()->GetLength(), (unsigned int)cAttrs); // and the length of the array to represent every character in the row

        delete[] rgAttrs;

        Log::Comment(L"Test turning single attr into chain");

        cAttrs = pSingle->_TotalLength();
        rgAttrs = new TextAttribute[cAttrs];

        WORD wCurrentAttr = 0;
        UINT cCurrAttrCount = 0; // this is the count of the number of times the wCurrentAttr has been placed into the array
        const UINT cCurrAttrLimit = 5; // this is the limit of how many times any unique attr should be placed in the array in a row

        for (UINT iAttr = 0; iAttr < cAttrs; iAttr++)
        {
            // set the attributes to the current
            rgAttrs[iAttr] = TextAttribute(wCurrentAttr);

            cCurrAttrCount++;
            // if we're passing up the index of the limit
            if (cCurrAttrCount >= cCurrAttrLimit)
            {
                // reset to 0
                cCurrAttrCount = 0;

                // change which attribute will be applied to the array next
                wCurrentAttr++;
            }
        }

        pSingle->PackAttrs(rgAttrs, cAttrs);

        // the number of segments we created is the number of characters in the row divided by how long each attribute was used
        // E.g. 80 items / runs of 5 = 16 chain segments created (all 5 long each).
        UINT cNumberOfChainSegments = cAttrs / cCurrAttrLimit;

        // If we had a remainder, then one more was created besides the division math above
        // E.g. 81 items / runs of 5 = 16 chain segments of 5 + 1 chain segment of 1 = 17 total segments
        if (cAttrs % cCurrAttrLimit > 0)
        {
            cNumberOfChainSegments++;
        }

        // length should match
        VERIFY_ARE_EQUAL((UINT)pSingle->Length, cNumberOfChainSegments);

        // now verify each chain segment is holding the correct data
        for (UINT iChainSeg = 0; iChainSeg < cNumberOfChainSegments; iChainSeg++)
        {
            // We set the attribute to the number of the chain segment when creating it, so just test against the index.
            TextAttribute SegmentAttributes = TextAttribute((WORD)iChainSeg);
            TextAttributeRun* pSegment = &pSingle->GetHead()[iChainSeg];
            VERIFY_IS_TRUE(pSegment->GetAttributes().IsEqual(SegmentAttributes));

            // if we're looking at a non-remainder chain segment
            if (iChainSeg < (cAttrs / cCurrAttrLimit))
            {
                // then the length of it is equal to the entire limit length while we were creating
                VERIFY_ARE_EQUAL((UINT)pSegment->GetLength(), cCurrAttrLimit);
            }
            else
            {
                // otherwise it's equal to the remainder amount from the length of the string over the length of the limit of each segment
                VERIFY_ARE_EQUAL((UINT)pSegment->GetLength(), cAttrs % cCurrAttrLimit);
            }
        }

        delete[] rgAttrs;
    }

    TEST_METHOD(TestSetAttrToEnd)
    {
        const WORD wTestAttr = FOREGROUND_BLUE | BACKGROUND_GREEN;
        TextAttribute TestAttr = TextAttribute(wTestAttr);

        Log::Comment(L"FIRST: Set index to > 0 to test making/modifying chains");
        const short iTestIndex = 50;
        ASSERT(iTestIndex >= 0 && iTestIndex < _sDefaultLength);

        Log::Comment(L"SetAttrToEnd for single color applied to whole string.");
        pSingle->SetAttrToEnd(iTestIndex, TestAttr);

        // Was 1 (single), should now have 2 segments
        VERIFY_ARE_EQUAL(pSingle->Length, 2);

        VERIFY_IS_TRUE(pSingle->GetHead()[0].GetAttributes().IsEqual(_DefaultAttr));
        VERIFY_ARE_EQUAL(pSingle->GetHead()[0].GetLength(), (unsigned int)(_sDefaultLength - (_sDefaultLength - iTestIndex)));

        VERIFY_IS_TRUE(pSingle->GetHead()[1].GetAttributes().IsEqual(TestAttr));
        VERIFY_ARE_EQUAL(pSingle->GetHead()[1].GetLength(), (unsigned int)(_sDefaultLength - iTestIndex));

        Log::Comment(L"SetAttrToEnd for existing chain of multiple colors.");
        pChain->SetAttrToEnd(iTestIndex, TestAttr);

        // From 7 segments down to 5.
        VERIFY_ARE_EQUAL(pChain->Length, 5);

        // Verify chain colors and lengths
        VERIFY_IS_TRUE(TextAttribute(0).IsEqual(pChain->GetHead()[0].GetAttributes()));
        VERIFY_ARE_EQUAL(pChain->GetHead()[0].GetLength(), (unsigned int)13);

        VERIFY_IS_TRUE(TextAttribute(1).IsEqual(pChain->GetHead()[1].GetAttributes()));
        VERIFY_ARE_EQUAL(pChain->GetHead()[1].GetLength(), (unsigned int)13);

        VERIFY_IS_TRUE(TextAttribute(2).IsEqual(pChain->GetHead()[2].GetAttributes()));
        VERIFY_ARE_EQUAL(pChain->GetHead()[2].GetLength(), (unsigned int)13);

        VERIFY_IS_TRUE(TextAttribute(3).IsEqual(pChain->GetHead()[3].GetAttributes()));
        VERIFY_ARE_EQUAL(pChain->GetHead()[3].GetLength(), (unsigned int)11);

        VERIFY_IS_TRUE(TestAttr.IsEqual(pChain->GetHead()[4].GetAttributes()));
        VERIFY_ARE_EQUAL(pChain->GetHead()[4].GetLength(), (unsigned int)30);

        Log::Comment(L"SECOND: Set index to 0 to test replacing anything with a single");

        ATTR_ROW* pTestItems[] { pSingle, pChain };

        for (UINT iIndex = 0; iIndex < ARRAYSIZE(pTestItems); iIndex++)
        {
            ATTR_ROW* pUnderTest = pTestItems[iIndex];

            pUnderTest->SetAttrToEnd(0, TestAttr);

            // should be down to 1 attribute set from beginning to end of string
            VERIFY_ARE_EQUAL(pUnderTest->Length, 1);

            // singular pair should contain the color
            VERIFY_IS_TRUE(pUnderTest->GetHead()[0].GetAttributes().IsEqual(TestAttr));

            // and its length should be the length of the whole string
            VERIFY_ARE_EQUAL(pUnderTest->GetHead()[0].GetLength(), (unsigned int)_sDefaultLength);
        }
    }

    TEST_METHOD(TestTotalLength)
    {
        ATTR_ROW* pTestItems[] { pSingle, pChain };

        for (UINT iIndex = 0; iIndex < ARRAYSIZE(pTestItems); iIndex++)
        {
            ATTR_ROW* pUnderTest = pTestItems[iIndex];

            const UINT sResult = pUnderTest->_TotalLength();

            VERIFY_ARE_EQUAL((short)sResult, _sDefaultLength);
        }
    }
};
