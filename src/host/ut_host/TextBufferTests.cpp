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

#include "input.h"

#include "..\interactivity\inc\ServiceLocator.hpp"

using namespace WEX::Common;
using namespace WEX::Logging;
using namespace WEX::TestExecution;

class TextBufferTests
{
    CommonState* m_state;

    TEST_CLASS(TextBufferTests);

    TEST_CLASS_SETUP(ClassSetup)
    {
        m_state = new CommonState();

        m_state->PrepareGlobalFont();
        m_state->PrepareGlobalScreenBuffer();

        return true;
    }

    TEST_CLASS_CLEANUP(ClassCleanup)
    {
        m_state->CleanupGlobalScreenBuffer();
        m_state->CleanupGlobalFont();

        delete m_state;

        return true;
    }

    TEST_METHOD_SETUP(MethodSetup)
    {
        m_state->PrepareNewTextBufferInfo();

        return true;
    }

    TEST_METHOD_CLEANUP(MethodCleanup)
    {
        m_state->CleanupNewTextBufferInfo();

        return true;
    }

    TEST_METHOD(TestBufferCreate)
    {
        VERIFY_IS_TRUE(NT_SUCCESS(m_state->GetTextBufferInfoInitResult()));
    }

    void DoBufferRowIterationTest(TEXT_BUFFER_INFO* pTbi)
    {
        ROW* pFirstRow = pTbi->GetFirstRow();
        ROW* pPrior = nullptr;
        ROW* pRow = pFirstRow;
        VERIFY_IS_NOT_NULL(pRow);

        UINT cRows = 0;
        while (pRow != nullptr)
        {
            cRows++;
            pPrior = pRow; // save off the previous for a reverse check
            pRow = pTbi->GetNextRowNoWrap(pRow);

            // if we didn't just hit the last row...
            if (pRow != nullptr)
            {
                // grab the previous row from the one we just found
                ROW* pPriorCheck = pTbi->GetPrevRowNoWrap(pRow);

                VERIFY_ARE_EQUAL(pPrior, pPriorCheck); // it should still equal what the row was before we started this iteration
            }
        }

        // the number of rows we iterated through should be the same as the window size.
        VERIFY_ARE_EQUAL(pTbi->TotalRowCount(), cRows);
    }

    TEXT_BUFFER_INFO* GetTbi()
    {
        return ServiceLocator::LocateGlobals()->getConsoleInformation()->CurrentScreenBuffer->TextInfo;
    }

    SHORT GetBufferWidth()
    {
        return ServiceLocator::LocateGlobals()->getConsoleInformation()->CurrentScreenBuffer->TextInfo->_coordBufferSize.X;
    }

    SHORT GetBufferHeight()
    {
        return ServiceLocator::LocateGlobals()->getConsoleInformation()->CurrentScreenBuffer->TextInfo->_coordBufferSize.Y;
    }

    TEST_METHOD(TestBufferRowIteration)
    {
        DoBufferRowIterationTest(GetTbi());
    }

    TEST_METHOD(TestBufferRowIterationWhenCircular)
    {
        SHORT csBufferHeight = GetBufferHeight();

        ASSERT(csBufferHeight > 4);

        TEXT_BUFFER_INFO* tbi = GetTbi();

        tbi->_FirstRow = csBufferHeight / 2 + 2; // sets to a bit over half way around.

        DoBufferRowIterationTest(tbi);
    }

    TEST_METHOD(TestBufferRowByOffset)
    {
        TEXT_BUFFER_INFO* tbi = GetTbi();
        SHORT csBufferHeight = GetBufferHeight();

        ASSERT(csBufferHeight > 20);

        short sId = csBufferHeight / 2 - 5;

        ROW* pRow = tbi->GetRowByOffset(sId);
        VERIFY_IS_NOT_NULL(pRow);

        if (pRow != nullptr)
        {
            VERIFY_ARE_EQUAL(pRow->sRowId, sId);
        }

    }

    TEST_METHOD(TestWrapFlag)
    {
        TEXT_BUFFER_INFO* tbi = GetTbi();

        ROW* pRow = tbi->GetFirstRow();

        // no wrap by default
        VERIFY_IS_FALSE(pRow->CharRow.WasWrapForced());

        // try set wrap and check
        pRow->CharRow.SetWrapStatus(true);
        VERIFY_IS_TRUE(pRow->CharRow.WasWrapForced());

        // try unset wrap and check
        pRow->CharRow.SetWrapStatus(false);
        VERIFY_IS_FALSE(pRow->CharRow.WasWrapForced());
    }

    TEST_METHOD(TestDoubleBytePadFlag)
    {
        TEXT_BUFFER_INFO* tbi = GetTbi();

        ROW* pRow = tbi->GetFirstRow();

        // no padding by default
        VERIFY_IS_FALSE(pRow->CharRow.WasDoubleBytePadded());

        // try set and check
        pRow->CharRow.SetDoubleBytePadded(true);
        VERIFY_IS_TRUE(pRow->CharRow.WasDoubleBytePadded());

        // try unset and check
        pRow->CharRow.SetDoubleBytePadded(false);
        VERIFY_IS_FALSE(pRow->CharRow.WasDoubleBytePadded());
    }

    void DoBoundaryTest(PWCHAR const pwszInputString, short const cLength, short const cMax, short const cLeft, short const cRight)
    {
        TEXT_BUFFER_INFO* tbi = GetTbi();

        ROW* pRow = tbi->GetFirstRow();
        CHAR_ROW* pCharRow = &pRow->CharRow;

        // copy string into buffer
        CopyMemory(pCharRow->Chars, pwszInputString, cLength * sizeof(WCHAR));

        // space pad the rest of the string
        if (cLength < cMax)
        {
            PWCHAR pChars= pCharRow->Chars;

            for (short cStart = cLength; cStart <= cMax; cStart++)
            {
                pChars[cStart] = UNICODE_SPACE;
            }
        }

        // left edge should be 0 since there are no leading spaces
        VERIFY_ARE_EQUAL(pCharRow->MeasureLeft(cMax), cLeft);
        // right edge should be one past the index of the last character or the string length
        VERIFY_ARE_EQUAL(pCharRow->MeasureRight(cMax), cRight);

        const short csLeftBefore = -1234;
        const short csRightBefore = -4567;

        pCharRow->Left = csLeftBefore;
        pCharRow->MeasureAndSaveLeft(cMax);
        VERIFY_ARE_EQUAL(pCharRow->Left, cLeft);

        pCharRow->Right = csRightBefore;
        pCharRow->MeasureAndSaveRight(cMax);
        VERIFY_ARE_EQUAL(pCharRow->Right, cRight);

        pCharRow->Left = csLeftBefore;
        pCharRow->Right = csRightBefore;
        pCharRow->RemeasureBoundaryValues(cMax);
        VERIFY_ARE_EQUAL(pCharRow->Left, cLeft);
        VERIFY_ARE_EQUAL(pCharRow->Right, cRight);
    }

    TEST_METHOD(TestBoundaryMeasuresRegularString)
    {
        SHORT csBufferWidth = GetBufferWidth();

        const PWCHAR pwszLazyDog = L"The quick brown fox jumps over the lazy dog."; // length 44, left 0, right 44
        DoBoundaryTest(pwszLazyDog, 44, csBufferWidth, 0, 44);
    }

    TEST_METHOD(TestBoundaryMeasuresFloatingString)
    {
        SHORT csBufferWidth = GetBufferWidth();

        const PWCHAR pwszOffsets = L"     C:\\>     "; // length 5 spaces + 4 chars + 5 spaces = 14, left 5, right 9
        DoBoundaryTest(pwszOffsets, 14, csBufferWidth, 5, 9);
    }

    TEST_METHOD(TestCopyProperties)
    {
        TEXT_BUFFER_INFO* pOtherTbi = GetTbi();

        TEXT_BUFFER_INFO* pNewTbi = new TEXT_BUFFER_INFO(&pOtherTbi->_fiCurrentFont);
        pNewTbi->_pCursor = new Cursor(ServiceLocator::LocateAccessibilityNotifier(), 40);// value is irrelevant for this test
        // set initial mapping values
        pNewTbi->GetCursor()->SetHasMoved(FALSE);
        pOtherTbi->GetCursor()->SetHasMoved(TRUE);

        pNewTbi->GetCursor()->SetIsVisible(FALSE);
        pOtherTbi->GetCursor()->SetIsVisible(TRUE);

        pNewTbi->GetCursor()->SetIsOn(FALSE);
        pOtherTbi->GetCursor()->SetIsOn(TRUE);

        pNewTbi->GetCursor()->SetIsDouble(FALSE);
        pOtherTbi->GetCursor()->SetIsDouble(TRUE);

        pNewTbi->GetCursor()->SetDelay(FALSE);
        pOtherTbi->GetCursor()->SetDelay(TRUE);

        // run copy
        pNewTbi->CopyProperties(pOtherTbi);

        // test that new now contains values from other
        VERIFY_ARE_EQUAL(pNewTbi->GetCursor()->HasMoved(), TRUE);
        VERIFY_ARE_EQUAL(pNewTbi->GetCursor()->IsVisible(), TRUE);
        VERIFY_ARE_EQUAL(pNewTbi->GetCursor()->IsOn(), TRUE);
        VERIFY_ARE_EQUAL(pNewTbi->GetCursor()->IsDouble(), TRUE);
        VERIFY_ARE_EQUAL(pNewTbi->GetCursor()->GetDelay(), TRUE);

        delete pNewTbi;
    }

    TEST_METHOD(TestInsertCharacter)
    {
        TEXT_BUFFER_INFO* const pTbi = GetTbi();

        // get starting cursor position
        COORD const coordCursorBefore = pTbi->GetCursor()->GetPosition();

        // Get current row from the buffer
        ROW* const pRow = pTbi->GetRowByOffset(coordCursorBefore.Y);

        // create some sample test data
        WCHAR const wchTest = L'Z';
        BYTE const bKAttrTest = 0x80; // invalid flag, technically, for test only. see textBuffer.hpp's ATTR_LEADING_BYTE, etc
        WORD const wAttrTest = BACKGROUND_INTENSITY | FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_BLUE;
        TextAttribute TestAttributes = TextAttribute(wAttrTest);

        // ensure that the buffer didn't start with these fields
        VERIFY_ARE_NOT_EQUAL(pRow->CharRow.Chars[coordCursorBefore.X], wchTest);
        VERIFY_ARE_NOT_EQUAL(pRow->CharRow.KAttrs[coordCursorBefore.X], bKAttrTest);

        TextAttributeRun* pAttrRun;
        UINT cAttrApplies;
        pRow->AttrRow.FindAttrIndex(coordCursorBefore.X, &pAttrRun, &cAttrApplies);

        VERIFY_IS_FALSE(pAttrRun->GetAttributes().IsEqual(TestAttributes));

        // now apply the new data to the buffer
        pTbi->InsertCharacter(wchTest, bKAttrTest, TestAttributes);

        // ensure that the buffer position where the cursor WAS contains the test items
        VERIFY_ARE_EQUAL(pRow->CharRow.Chars[coordCursorBefore.X], wchTest);
        VERIFY_ARE_EQUAL(pRow->CharRow.KAttrs[coordCursorBefore.X], bKAttrTest);

        pRow->AttrRow.FindAttrIndex(coordCursorBefore.X, &pAttrRun, &cAttrApplies);
        VERIFY_IS_TRUE(pAttrRun->GetAttributes().IsEqual(TestAttributes));

        // ensure that the cursor moved to a new position (X or Y or both have changed)
        VERIFY_IS_TRUE((coordCursorBefore.X != pTbi->GetCursor()->GetPosition().X) || (coordCursorBefore.Y != pTbi->GetCursor()->GetPosition().Y));
        // the proper advancement of the cursor (e.g. which position it goes to) is validated in other tests
    }

    TEST_METHOD(TestIncrementCursor)
    {
        TEXT_BUFFER_INFO* const pTbi = GetTbi();

        // only checking X increments here
        // Y increments are covered in the NewlineCursor test

        short const sBufferWidth = pTbi->_coordBufferSize.X;

        #if DBG
        short const sBufferHeight = pTbi->_coordBufferSize.Y;
        ASSERT(sBufferWidth > 1 && sBufferHeight > 1);
        #endif

        Log::Comment(L"Test normal case of moving once to the right within a single line");
        pTbi->GetCursor()->SetXPosition(0);
        pTbi->GetCursor()->SetYPosition(0);

        COORD coordCursorBefore = pTbi->GetCursor()->GetPosition();

        pTbi->IncrementCursor();

        VERIFY_ARE_EQUAL(pTbi->GetCursor()->GetPosition().X, 1); // X should advance by 1
        VERIFY_ARE_EQUAL(pTbi->GetCursor()->GetPosition().Y, coordCursorBefore.Y); // Y shouldn't have moved

        Log::Comment(L"Test line wrap case where cursor is on the right edge of the line");
        pTbi->GetCursor()->SetXPosition(sBufferWidth - 1);
        pTbi->GetCursor()->SetYPosition(0);

        coordCursorBefore = pTbi->GetCursor()->GetPosition();

        pTbi->IncrementCursor();

        VERIFY_ARE_EQUAL(pTbi->GetCursor()->GetPosition().X, 0); // position should be reset to the left edge when passing right edge
        VERIFY_ARE_EQUAL(pTbi->GetCursor()->GetPosition().Y - 1, coordCursorBefore.Y); // the cursor should be moved one row down from where it used to be
    }

    TEST_METHOD(TestNewlineCursor)
    {
        TEXT_BUFFER_INFO* const pTbi = GetTbi();


        const short sBufferHeight = pTbi->_coordBufferSize.Y;

        #if DBG
        const short sBufferWidth = pTbi->_coordBufferSize.X;
        // width and height are sufficiently large for upcoming math
        ASSERT(sBufferWidth > 4 && sBufferHeight > 4);
        #endif

        Log::Comment(L"Verify standard row increment from somewhere in the buffer");

        // set cursor X position to non zero, any position in buffer
        pTbi->GetCursor()->SetXPosition(3);

        // set cursor Y position to not-the-final row in the buffer
        pTbi->GetCursor()->SetYPosition(3);

        COORD coordCursorBefore = pTbi->GetCursor()->GetPosition();

        // perform operation
        pTbi->NewlineCursor();

        // verify
        VERIFY_ARE_EQUAL(pTbi->GetCursor()->GetPosition().X, 0); // move to left edge of buffer
        VERIFY_ARE_EQUAL(pTbi->GetCursor()->GetPosition().Y, coordCursorBefore.Y + 1); // move down one row

        Log::Comment(L"Verify increment when already on last row of buffer");

        // X position still doesn't matter
        pTbi->GetCursor()->SetXPosition(3);

        // Y position needs to be on the last row of the buffer
        pTbi->GetCursor()->SetYPosition(sBufferHeight - 1);

        coordCursorBefore = pTbi->GetCursor()->GetPosition();

        // perform operation
        pTbi->NewlineCursor();

        // verify
        VERIFY_ARE_EQUAL(pTbi->GetCursor()->GetPosition().X, 0); // move to left edge
        VERIFY_ARE_EQUAL(pTbi->GetCursor()->GetPosition().Y, coordCursorBefore.Y); // cursor Y position should not have moved. stays on same logical final line of buffer

        // This is okay because the backing circular buffer changes, not the logical screen position (final visible line of the buffer)
    }

    void TestLastNonSpace(short const cursorPosY)
    {
        TEXT_BUFFER_INFO* const pTbi = GetTbi();
        pTbi->GetCursor()->SetYPosition(cursorPosY);

        COORD coordLastNonSpace = pTbi->GetLastNonSpaceCharacter();

        // We expect the last non space character to be the last printable character in the row.
        // The .Right property on a row is 1 past the last printable character in the row.
        // If there is one character in the row, the last character would be 0.
        // If there are no characters in the row, the last character would be -1 and we need to seek backwards to find the previous row with a character.

        // start expected position from cursor
        COORD coordExpected = pTbi->GetCursor()->GetPosition();

        // Try to get the X position from the current cursor position.
        coordExpected.X = pTbi->GetRowByOffset(coordExpected.Y)->CharRow.Right - 1;

        // If we went negative, this row was empty and we need to continue seeking upward...
        // - As long as X is negative (empty rows)
        // - As long as we have space before the top of the buffer (Y isn't the 0th/top row).
        while (coordExpected.X < 0 && coordExpected.Y > 0)
        {
            coordExpected.Y--;
            coordExpected.X = pTbi->GetRowByOffset(coordExpected.Y)->CharRow.Right - 1;
        }

        VERIFY_ARE_EQUAL(coordLastNonSpace.X, coordExpected.X);
        VERIFY_ARE_EQUAL(coordLastNonSpace.Y, coordExpected.Y);
    }

    TEST_METHOD(TestGetLastNonSpaceCharacter)
    {
        m_state->FillTextBuffer(); // fill buffer with some text, it should be 4 rows. See CommonState for details

        Log::Comment(L"Test with cursor inside last row of text");
        TestLastNonSpace(3);

        Log::Comment(L"Test with cursor one beyond last row of text");
        TestLastNonSpace(4);

        Log::Comment(L"Test with cursor way beyond last row of text");
        TestLastNonSpace(14);
    }

    TEST_METHOD(TestSetWrapOnCurrentRow)
    {
        TEXT_BUFFER_INFO* const pTbi = GetTbi();

        short sCurrentRow = pTbi->GetCursor()->GetPosition().Y;

        ROW* pRow = pTbi->GetRowByOffset(sCurrentRow);

        Log::Comment(L"Testing off to on");

        // turn wrap status off first
        pRow->CharRow.SetWrapStatus(false);

        // trigger wrap
        pTbi->SetWrapOnCurrentRow();

        // ensure this row was flipped
        VERIFY_IS_TRUE(pRow->CharRow.WasWrapForced());

        Log::Comment(L"Testing on stays on");

        // make sure wrap status is on
        pRow->CharRow.SetWrapStatus(true);

        // trigger wrap
        pTbi->SetWrapOnCurrentRow();

        // ensure row is still on
        VERIFY_IS_TRUE(pRow->CharRow.WasWrapForced());
    }

    TEST_METHOD(TestIncrementCircularBuffer)
    {
        TEXT_BUFFER_INFO* const pTbi = GetTbi();

        short const sBufferHeight = pTbi->_coordBufferSize.Y;

        ASSERT(sBufferHeight > 4); // buffer should be sufficiently large

        Log::Comment(L"Test 1 = FirstRow of circular buffer is not the final row of the buffer");
        Log::Comment(L"Test 2 = FirstRow of circular buffer IS THE FINAL ROW of the buffer (and therefore circles)");
        short rgRowsToTest[] = { 2, sBufferHeight - 1 };

        for (UINT iTestIndex = 0; iTestIndex < ARRAYSIZE(rgRowsToTest); iTestIndex++)
        {
            const short iRowToTestIndex = rgRowsToTest[iTestIndex];

            short iNextRowIndex = iRowToTestIndex + 1;
            // if we're at or crossing the height, loop back to 0 (circular buffer)
            if (iNextRowIndex >= sBufferHeight)
            {
                iNextRowIndex = 0;
            }

            pTbi->_FirstRow = iRowToTestIndex;

            // fill first row with some stuff
            ROW* pFirstRow = pTbi->GetFirstRow();
            pFirstRow->CharRow.Left = 0;
            pFirstRow->CharRow.Right = 15;

            // ensure it does say that it contains text
            VERIFY_IS_TRUE(pFirstRow->CharRow.ContainsText());

            // try increment
            pTbi->IncrementCircularBuffer();

            // validate that first row has moved
            VERIFY_ARE_EQUAL(pTbi->_FirstRow, iNextRowIndex); // first row has incremented
            VERIFY_ARE_NOT_EQUAL(pTbi->GetFirstRow(), pFirstRow); // the old first row is no longer the first

            // ensure old first row has been emptied
            VERIFY_IS_FALSE(pFirstRow->CharRow.ContainsText());
        }
    }
    TEST_METHOD(TestMixedRgbAndLegacy);
    TEST_METHOD(TestRgbEraseLine);
};

    
void TextBufferTests::TestMixedRgbAndLegacy()
{
    // BOOL fSuccess = TRUE;
    auto gci = ServiceLocator::LocateGlobals()->getConsoleInformation();
    auto psi = gci->CurrentScreenBuffer->GetActiveBuffer();
    auto tbi = psi->TextInfo;
    // auto bufferWriter = psi->GetBufferWriter();
    auto stateMachine = psi->GetStateMachine();
    auto cursor = tbi->GetCursor();
    VERIFY_IS_NOT_NULL(stateMachine);
    VERIFY_IS_NOT_NULL(cursor);

    // This is an assumption to make a lot of math easier.
    VERIFY_ARE_EQUAL(tbi->GetFirstRowIndex(), 0);

    // Case 1 - 
    //      Write '\E[38;2;64;128;255mX\E[49mX\E[m'
    //      Make sure that the second X has RGB attributes (FG and BG)
    //      FG = rgb(64;128;255), BG = rgb(default)
    {
        std::wstring sequence = L"\x1b[38;2;64;128;255mX\x1b[49mX\x1b[m";
        stateMachine->ProcessString(&sequence[0], sequence.length());
        auto x = cursor->GetPosition().X;
        auto y = cursor->GetPosition().Y;
        auto row = tbi->GetRowByOffset(y);
        auto attrRow = row->AttrRow;
        auto attrs = new TextAttribute[tbi->_coordBufferSize.X];
        VERIFY_IS_NOT_NULL(attrs);
        attrRow.UnpackAttrs(attrs, tbi->_coordBufferSize.X);
        auto attrA = attrs[x-2];
        auto attrB = attrs[x-1];
        Log::Comment(NoThrowString().Format(
            L"cursor={X:%d,Y:%d}", 
            x, y
        ));
        Log::Comment(NoThrowString().Format(
            L"attrA={IsLegacy:%d,GetLegacyAttributes:0x%x}", 
            attrA.IsLegacy(), attrA.GetLegacyAttributes()
        ));
        Log::Comment(NoThrowString().Format(
            L"attrA={FG:0x%x,BG:0x%x}", 
            attrA.GetRgbForeground(), attrA.GetRgbBackground()
        ));
        Log::Comment(NoThrowString().Format(
            L"attrB={IsLegacy:%d,GetLegacyAttributes:0x%x}", 
            attrB.IsLegacy(), attrB.GetLegacyAttributes()
        ));
        Log::Comment(NoThrowString().Format(
            L"attrB={FG:0x%x,BG:0x%x}", 
            attrB.GetRgbForeground(), attrB.GetRgbBackground()
        ));
    
        VERIFY_ARE_EQUAL(attrA.IsLegacy(), false);
        VERIFY_ARE_EQUAL(attrB.IsLegacy(), false);
        
        VERIFY_ARE_EQUAL(attrA.GetRgbForeground(), RGB(64,128,255));
        VERIFY_ARE_EQUAL(attrA.GetRgbBackground(), psi->GetAttributes().GetRgbBackground());
        
        VERIFY_ARE_EQUAL(attrB.GetRgbForeground(), RGB(64,128,255));
        VERIFY_ARE_EQUAL(attrB.GetRgbBackground(), psi->GetAttributes().GetRgbBackground());
        
        std::wstring reset = L"\x1b[0m";
        stateMachine->ProcessString(&reset[0], reset.length());
    }

    // Case 2 - 
    //      \E[48;2;64;128;255mX\E[39mX\E[m
    //      Make sure that the second X has RGB attributes (FG and BG)
    //      FG = rgb(default), BG = rgb(64;128;255)
    {
        std::wstring sequence = L"\x1b[48;2;64;128;255mX\x1b[39mX\x1b[m";
        stateMachine->ProcessString(&sequence[0], sequence.length());
        auto x = cursor->GetPosition().X;
        auto y = cursor->GetPosition().Y;
        auto row = tbi->GetRowByOffset(y);
        auto attrRow = row->AttrRow;
        auto attrs = new TextAttribute[tbi->_coordBufferSize.X];
        VERIFY_IS_NOT_NULL(attrs);
        attrRow.UnpackAttrs(attrs, tbi->_coordBufferSize.X);
        auto attrA = attrs[x-2];
        auto attrB = attrs[x-1];
        Log::Comment(NoThrowString().Format(
            L"cursor={X:%d,Y:%d}", 
            x, y
        ));
        Log::Comment(NoThrowString().Format(
            L"attrA={IsLegacy:%d,GetLegacyAttributes:0x%x}", 
            attrA.IsLegacy(), attrA.GetLegacyAttributes()
        ));
        Log::Comment(NoThrowString().Format(
            L"attrA={FG:0x%x,BG:0x%x}", 
            attrA.GetRgbForeground(), attrA.GetRgbBackground()
        ));
        Log::Comment(NoThrowString().Format(
            L"attrB={IsLegacy:%d,GetLegacyAttributes:0x%x}", 
            attrB.IsLegacy(), attrB.GetLegacyAttributes()
        ));
        Log::Comment(NoThrowString().Format(
            L"attrB={FG:0x%x,BG:0x%x}", 
            attrB.GetRgbForeground(), attrB.GetRgbBackground()
        ));
    
        VERIFY_ARE_EQUAL(attrA.IsLegacy(), false);
        VERIFY_ARE_EQUAL(attrB.IsLegacy(), false);

        VERIFY_ARE_EQUAL(attrA.GetRgbBackground(), RGB(64,128,255));
        VERIFY_ARE_EQUAL(attrA.GetRgbForeground(), psi->GetAttributes().GetRgbForeground());
        
        VERIFY_ARE_EQUAL(attrB.GetRgbBackground(), RGB(64,128,255));
        VERIFY_ARE_EQUAL(attrB.GetRgbForeground(), psi->GetAttributes().GetRgbForeground());
        
        std::wstring reset = L"\x1b[0m";
        stateMachine->ProcessString(&reset[0], reset.length());
    }

    // Case 3 - 
    //      '\E[48;2;64;128;255mX\E[4mX\E[m'
    //      Make sure that the second X has RGB attributes AND underline
    {
        std::wstring sequence = L"\x1b[48;2;64;128;255mX\x1b[4mX\x1b[m";
        stateMachine->ProcessString(&sequence[0], sequence.length());
        auto x = cursor->GetPosition().X;
        auto y = cursor->GetPosition().Y;
        auto row = tbi->GetRowByOffset(y);
        auto attrRow = row->AttrRow;
        auto attrs = new TextAttribute[tbi->_coordBufferSize.X];
        VERIFY_IS_NOT_NULL(attrs);
        attrRow.UnpackAttrs(attrs, tbi->_coordBufferSize.X);
        auto attrA = attrs[x-2];
        auto attrB = attrs[x-1];
        Log::Comment(NoThrowString().Format(
            L"cursor={X:%d,Y:%d}", 
            x, y
        ));
        Log::Comment(NoThrowString().Format(
            L"attrA={IsLegacy:%d,GetLegacyAttributes:0x%x}", 
            attrA.IsLegacy(), attrA.GetLegacyAttributes()
        ));
        Log::Comment(NoThrowString().Format(
            L"attrA={FG:0x%x,BG:0x%x}", 
            attrA.GetRgbForeground(), attrA.GetRgbBackground()
        ));
        Log::Comment(NoThrowString().Format(
            L"attrB={IsLegacy:%d,GetLegacyAttributes:0x%x}", 
            attrB.IsLegacy(), attrB.GetLegacyAttributes()
        ));
        Log::Comment(NoThrowString().Format(
            L"attrB={FG:0x%x,BG:0x%x}", 
            attrB.GetRgbForeground(), attrB.GetRgbBackground()
        ));
    
        VERIFY_ARE_EQUAL(attrA.IsLegacy(), false);
        VERIFY_ARE_EQUAL(attrB.IsLegacy(), false);

        VERIFY_ARE_EQUAL(attrA.GetRgbBackground(), RGB(64,128,255));
        VERIFY_ARE_EQUAL(attrA.GetRgbForeground(), psi->GetAttributes().GetRgbForeground());

        VERIFY_ARE_EQUAL(attrB.GetRgbBackground(), RGB(64,128,255));
        VERIFY_ARE_EQUAL(attrB.GetRgbForeground(), psi->GetAttributes().GetRgbForeground());

        VERIFY_ARE_EQUAL(attrA.GetLegacyAttributes()&COMMON_LVB_UNDERSCORE, 0);
        VERIFY_ARE_EQUAL(attrB.GetLegacyAttributes()&COMMON_LVB_UNDERSCORE, COMMON_LVB_UNDERSCORE);

        std::wstring reset = L"\x1b[0m";
        stateMachine->ProcessString(&reset[0], reset.length());
    }

}  

void TextBufferTests::TestRgbEraseLine()
{
    auto gci = ServiceLocator::LocateGlobals()->getConsoleInformation();
    auto psi = gci->CurrentScreenBuffer->GetActiveBuffer();
    psi->OutputMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    auto tbi = psi->TextInfo;

    auto stateMachine = psi->GetStateMachine();
    auto cursor = tbi->GetCursor();
    VERIFY_IS_NOT_NULL(stateMachine);
    VERIFY_IS_NOT_NULL(cursor);

    // This is an assumption to make a lot of math easier.
    VERIFY_ARE_EQUAL(tbi->GetFirstRowIndex(), 0);
    cursor->SetXPosition(0);
    // Case 1 - 
    //      Write '\E[48;2;64;128;255X\E[48;2;128;128;255\E[KX'
    //      Make sure that all the characters after the first have the rgb attrs
    //      BG = rgb(128;128;255)
    {
        std::wstring sequence = L"\x1b[48;2;64;128;255mX\x1b[48;2;128;128;255m\x1b[KX";
        stateMachine->ProcessString(&sequence[0], sequence.length());

        const auto x = cursor->GetPosition().X;
        const auto y = cursor->GetPosition().Y;

        Log::Comment(NoThrowString().Format(
            L"cursor={X:%d,Y:%d}", 
            x, y
        ));
        VERIFY_ARE_EQUAL(x, 2);
        VERIFY_ARE_EQUAL(y, 0);

        const auto row = tbi->GetRowByOffset(y);
        const auto attrRow = row->AttrRow;
        const auto len = tbi->_coordBufferSize.X;
        const auto attrs = new TextAttribute[len];
        VERIFY_IS_NOT_NULL(attrs);
        attrRow.UnpackAttrs(attrs, len);

        const auto attr0 = attrs[0];

        VERIFY_ARE_EQUAL(attr0.IsLegacy(), false);
        VERIFY_ARE_EQUAL(attr0.GetRgbBackground(), RGB(64,128,255));
        for (auto i = 1; i < len; i++)
        {
            const auto attr = attrs[i];
            Log::Comment(NoThrowString().Format(
                L"attr={IsLegacy:%d,GetLegacyAttributes:0x%x}", 
                attr.IsLegacy(), attr.GetLegacyAttributes()
            ));
            Log::Comment(NoThrowString().Format(
                L"attr={FG:0x%x,BG:0x%x}", 
                attr.GetRgbForeground(), attr.GetRgbBackground()
            ));
            VERIFY_ARE_EQUAL(attr.IsLegacy(), false);
            VERIFY_ARE_EQUAL(attr.GetRgbBackground(), RGB(128,128,255));

        }
        std::wstring reset = L"\x1b[0m";
        stateMachine->ProcessString(&reset[0], reset.length());
    }
}