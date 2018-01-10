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

    TEST_METHOD(TestBufferCreate);

    void DoBufferRowIterationTest(TEXT_BUFFER_INFO* pTbi);

    TEXT_BUFFER_INFO* GetTbi();

    SHORT GetBufferWidth();

    SHORT GetBufferHeight();

    TEST_METHOD(TestBufferRowIteration);

    TEST_METHOD(TestBufferRowIterationWhenCircular);

    TEST_METHOD(TestBufferRowByOffset);

    TEST_METHOD(TestWrapFlag);

    TEST_METHOD(TestDoubleBytePadFlag);

    void DoBoundaryTest(PWCHAR const pwszInputString, 
                        short const cLength, 
                        short const cMax, 
                        short const cLeft, 
                        short const cRight);

    TEST_METHOD(TestBoundaryMeasuresRegularString);

    TEST_METHOD(TestBoundaryMeasuresFloatingString);

    TEST_METHOD(TestCopyProperties);

    TEST_METHOD(TestInsertCharacter);

    TEST_METHOD(TestIncrementCursor);

    TEST_METHOD(TestNewlineCursor);

    void TestLastNonSpace(short const cursorPosY);

    TEST_METHOD(TestGetLastNonSpaceCharacter);
    
    TEST_METHOD(TestSetWrapOnCurrentRow);
    
    TEST_METHOD(TestIncrementCircularBuffer);
    
    TEST_METHOD(TestMixedRgbAndLegacyForeground);
    TEST_METHOD(TestMixedRgbAndLegacyBackground);
    TEST_METHOD(TestMixedRgbAndLegacyUnderline);
    TEST_METHOD(TestMixedRgbAndLegacyBrightness);
    
    TEST_METHOD(TestRgbEraseLine);
    
    TEST_METHOD(TestUnBold);
    TEST_METHOD(TestUnBoldRgb);
    TEST_METHOD(TestComplexUnBold);
    
    TEST_METHOD(CopyAttrs);
    TEST_METHOD(EmptySgrTest);

};

void TextBufferTests::TestBufferCreate()
{
    VERIFY_IS_TRUE(NT_SUCCESS(m_state->GetTextBufferInfoInitResult()));
}

void TextBufferTests::DoBufferRowIterationTest(TEXT_BUFFER_INFO* pTbi)
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

            // it should still equal what the row was before we started this iteration
            VERIFY_ARE_EQUAL(pPrior, pPriorCheck); 
        }
    }

    // the number of rows we iterated through should be the same as the window size.
    VERIFY_ARE_EQUAL(pTbi->TotalRowCount(), cRows);
}

TEXT_BUFFER_INFO* TextBufferTests::GetTbi()
{
    const CONSOLE_INFORMATION* const gci = ServiceLocator::LocateGlobals()->getConsoleInformation();
    return gci->CurrentScreenBuffer->TextInfo;
}

SHORT TextBufferTests::GetBufferWidth()
{
    return GetTbi()->_coordBufferSize.X;
}

SHORT TextBufferTests::GetBufferHeight()
{
    return GetTbi()->_coordBufferSize.Y;
}


void TextBufferTests::TestBufferRowIteration()
{
    DoBufferRowIterationTest(GetTbi());
}

void TextBufferTests::TestBufferRowIterationWhenCircular()
{
    SHORT csBufferHeight = GetBufferHeight();

    ASSERT(csBufferHeight > 4);

    TEXT_BUFFER_INFO* tbi = GetTbi();

    tbi->_FirstRow = csBufferHeight / 2 + 2; // sets to a bit over half way around.

    DoBufferRowIterationTest(tbi);
}

void TextBufferTests::TestBufferRowByOffset()
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

void TextBufferTests::TestWrapFlag()
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

void TextBufferTests::TestDoubleBytePadFlag()
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


void TextBufferTests::DoBoundaryTest(PWCHAR const pwszInputString, 
                                     short const cLength, 
                                     short const cMax, 
                                     short const cLeft, 
                                     short const cRight)
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

void TextBufferTests::TestBoundaryMeasuresRegularString()
{
    SHORT csBufferWidth = GetBufferWidth();

    // length 44, left 0, right 44
    const PWCHAR pwszLazyDog = L"The quick brown fox jumps over the lazy dog."; 
    DoBoundaryTest(pwszLazyDog, 44, csBufferWidth, 0, 44);
}

void TextBufferTests::TestBoundaryMeasuresFloatingString()
{
    SHORT csBufferWidth = GetBufferWidth();

    // length 5 spaces + 4 chars + 5 spaces = 14, left 5, right 9
    const PWCHAR pwszOffsets = L"     C:\\>     "; 
    DoBoundaryTest(pwszOffsets, 14, csBufferWidth, 5, 9);
}

void TextBufferTests::TestCopyProperties()
{
    TEXT_BUFFER_INFO* pOtherTbi = GetTbi();

    TEXT_BUFFER_INFO* pNewTbi = new TEXT_BUFFER_INFO(&pOtherTbi->_fiCurrentFont);
    // value is irrelevant for this test
    pNewTbi->_pCursor = new Cursor(ServiceLocator::LocateAccessibilityNotifier(), 40);
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

void TextBufferTests::TestInsertCharacter()
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

void TextBufferTests::TestIncrementCursor()
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

void TextBufferTests::TestNewlineCursor()
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

void TextBufferTests::TestLastNonSpace(short const cursorPosY)
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

void TextBufferTests::TestGetLastNonSpaceCharacter()
{
    m_state->FillTextBuffer(); // fill buffer with some text, it should be 4 rows. See CommonState for details

    Log::Comment(L"Test with cursor inside last row of text");
    TestLastNonSpace(3);

    Log::Comment(L"Test with cursor one beyond last row of text");
    TestLastNonSpace(4);

    Log::Comment(L"Test with cursor way beyond last row of text");
    TestLastNonSpace(14);
}

void TextBufferTests::TestSetWrapOnCurrentRow()
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

void TextBufferTests::TestIncrementCircularBuffer()
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

void TextBufferTests::TestMixedRgbAndLegacyForeground()
{
    const CONSOLE_INFORMATION* const gci = ServiceLocator::LocateGlobals()->getConsoleInformation();
    const SCREEN_INFORMATION* const psi = gci->CurrentScreenBuffer->GetActiveBuffer();
    const TEXT_BUFFER_INFO* const tbi = psi->TextInfo;
    StateMachine* const stateMachine = psi->GetStateMachine();
    const Cursor* const cursor = tbi->GetCursor();
    VERIFY_IS_NOT_NULL(stateMachine);
    VERIFY_IS_NOT_NULL(cursor);

    // Case 1 - 
    //      Write '\E[38;2;64;128;255mX\E[49mX\E[m'
    //      Make sure that the second X has RGB attributes (FG and BG)
    //      FG = rgb(64;128;255), BG = rgb(default)
    Log::Comment(L"Case 1 \"\\E[38;2;64;128;255mX\\E[49mX\\E[m\"");
    
    wchar_t* sequence = L"\x1b[38;2;64;128;255mX\x1b[49mX\x1b[m";

    stateMachine->ProcessString(sequence, std::wcslen(sequence));
    const short x = cursor->GetPosition().X;
    const short y = cursor->GetPosition().Y;
    const ROW* const row = tbi->GetRowByOffset(y);
    const auto attrRow = &row->AttrRow;
    const auto attrs = new TextAttribute[tbi->_coordBufferSize.X];
    VERIFY_IS_NOT_NULL(attrs);
    attrRow->UnpackAttrs(attrs, tbi->_coordBufferSize.X);
    const auto attrA = attrs[x-2];
    const auto attrB = attrs[x-1];
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
    
    wchar_t* reset = L"\x1b[0m";
    stateMachine->ProcessString(reset, std::wcslen(reset));
    
}

void TextBufferTests::TestMixedRgbAndLegacyBackground()
{
    const CONSOLE_INFORMATION* const gci = ServiceLocator::LocateGlobals()->getConsoleInformation();
    const SCREEN_INFORMATION* const psi = gci->CurrentScreenBuffer->GetActiveBuffer();
    const TEXT_BUFFER_INFO* const tbi = psi->TextInfo;
    StateMachine* const stateMachine = psi->GetStateMachine();
    const Cursor* const cursor = tbi->GetCursor();
    VERIFY_IS_NOT_NULL(stateMachine);
    VERIFY_IS_NOT_NULL(cursor);

    // Case 2 - 
    //      \E[48;2;64;128;255mX\E[39mX\E[m
    //      Make sure that the second X has RGB attributes (FG and BG)
    //      FG = rgb(default), BG = rgb(64;128;255)
    Log::Comment(L"Case 2 \"\\E[48;2;64;128;255mX\\E[39mX\\E[m\"");

    wchar_t* sequence = L"\x1b[48;2;64;128;255mX\x1b[39mX\x1b[m";
    stateMachine->ProcessString(sequence, std::wcslen(sequence));
    const auto x = cursor->GetPosition().X;
    const auto y = cursor->GetPosition().Y;
    const auto row = tbi->GetRowByOffset(y);
    const auto attrRow = &row->AttrRow;
    const auto attrs = new TextAttribute[tbi->_coordBufferSize.X];
    VERIFY_IS_NOT_NULL(attrs);
    attrRow->UnpackAttrs(attrs, tbi->_coordBufferSize.X);
    const auto attrA = attrs[x-2];
    const auto attrB = attrs[x-1];
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
    
    wchar_t* reset = L"\x1b[0m";
    stateMachine->ProcessString(reset, std::wcslen(reset));


}

void TextBufferTests::TestMixedRgbAndLegacyUnderline()
{
    const CONSOLE_INFORMATION* const gci = ServiceLocator::LocateGlobals()->getConsoleInformation();
    const SCREEN_INFORMATION* const psi = gci->CurrentScreenBuffer->GetActiveBuffer();
    const TEXT_BUFFER_INFO* const tbi = psi->TextInfo;
    StateMachine* const stateMachine = psi->GetStateMachine();
    const Cursor* const cursor = tbi->GetCursor();
    VERIFY_IS_NOT_NULL(stateMachine);
    VERIFY_IS_NOT_NULL(cursor);

    // Case 3 - 
    //      '\E[48;2;64;128;255mX\E[4mX\E[m'
    //      Make sure that the second X has RGB attributes AND underline
    Log::Comment(L"Case 3 \"\\E[48;2;64;128;255mX\\E[4mX\\E[m\"");
    wchar_t* sequence = L"\x1b[48;2;64;128;255mX\x1b[4mX\x1b[m";
    stateMachine->ProcessString(sequence, std::wcslen(sequence));
    const auto x = cursor->GetPosition().X;
    const auto y = cursor->GetPosition().Y;
    const auto row = tbi->GetRowByOffset(y);
    const auto attrRow = &row->AttrRow;
    const auto attrs = new TextAttribute[tbi->_coordBufferSize.X];
    VERIFY_IS_NOT_NULL(attrs);
    attrRow->UnpackAttrs(attrs, tbi->_coordBufferSize.X);
    const auto attrA = attrs[x-2];
    const auto attrB = attrs[x-1];
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

    wchar_t* reset = L"\x1b[0m";
    stateMachine->ProcessString(reset, std::wcslen(reset));

}

void TextBufferTests::TestMixedRgbAndLegacyBrightness()
{
    const CONSOLE_INFORMATION* const gci = ServiceLocator::LocateGlobals()->getConsoleInformation();
    const SCREEN_INFORMATION* const psi = gci->CurrentScreenBuffer->GetActiveBuffer();
    const TEXT_BUFFER_INFO* const tbi = psi->TextInfo;
    StateMachine* const stateMachine = psi->GetStateMachine();
    const Cursor* const cursor = tbi->GetCursor();
    VERIFY_IS_NOT_NULL(stateMachine);
    VERIFY_IS_NOT_NULL(cursor);
    // Case 4 - 
    //      '\E[32mX\E[1mX'
    //      Make sure that the second X is a BRIGHT green, not white.
    Log::Comment(L"Case 4 ;\"\\E[32mX\\E[1mX\"");
    const auto dark_green = gci->GetColorTableEntry(2);
    const auto bright_green = gci->GetColorTableEntry(10);
    VERIFY_ARE_NOT_EQUAL(dark_green, bright_green);

    wchar_t* sequence = L"\x1b[32mX\x1b[1mX";
    stateMachine->ProcessString(sequence, std::wcslen(sequence));
    const auto x = cursor->GetPosition().X;
    const auto y = cursor->GetPosition().Y;
    const auto row = tbi->GetRowByOffset(y);
    const auto attrRow = &row->AttrRow;
    const auto attrs = new TextAttribute[tbi->_coordBufferSize.X];
    VERIFY_IS_NOT_NULL(attrs);
    attrRow->UnpackAttrs(attrs, tbi->_coordBufferSize.X);
    const auto attrA = attrs[x-2];
    const auto attrB = attrs[x-1];
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

    VERIFY_ARE_EQUAL(attrA.GetRgbForeground(), dark_green);
    VERIFY_ARE_EQUAL(attrB.GetRgbForeground(), bright_green);

    wchar_t* reset = L"\x1b[0m";
    stateMachine->ProcessString(reset, std::wcslen(reset));
}  

void TextBufferTests::TestRgbEraseLine()
{
    const CONSOLE_INFORMATION* const gci = ServiceLocator::LocateGlobals()->getConsoleInformation();
    SCREEN_INFORMATION* const psi = gci->CurrentScreenBuffer->GetActiveBuffer();
    const TEXT_BUFFER_INFO* const tbi = psi->TextInfo;
    StateMachine* const stateMachine = psi->GetStateMachine();
    Cursor* const cursor = tbi->GetCursor();
    SetFlag(psi->OutputMode, ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    VERIFY_IS_NOT_NULL(stateMachine);
    VERIFY_IS_NOT_NULL(cursor);

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
        const auto attrRow = &row->AttrRow;
        const auto len = tbi->_coordBufferSize.X;
        const auto attrs = new TextAttribute[len];
        VERIFY_IS_NOT_NULL(attrs);
        attrRow->UnpackAttrs(attrs, len);

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

void TextBufferTests::TestUnBold()
{
    const CONSOLE_INFORMATION* const gci = ServiceLocator::LocateGlobals()->getConsoleInformation();
    SCREEN_INFORMATION* const psi = gci->CurrentScreenBuffer->GetActiveBuffer();
    const TEXT_BUFFER_INFO* const tbi = psi->TextInfo;
    StateMachine* const stateMachine = psi->GetStateMachine();
    Cursor* const cursor = tbi->GetCursor();
    SetFlag(psi->OutputMode, ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    VERIFY_IS_NOT_NULL(stateMachine);
    VERIFY_IS_NOT_NULL(cursor);

    cursor->SetXPosition(0);
    // Case 1 - 
    //      Write '\E[1;32mX\E[22mX'
    //      The first X should be bright green.
    //      The second x should be dark green.
    std::wstring sequence = L"\x1b[1;32mX\x1b[22mX";
    stateMachine->ProcessString(&sequence[0], sequence.length());

    const auto x = cursor->GetPosition().X;
    const auto y = cursor->GetPosition().Y;
    const auto dark_green = gci->GetColorTableEntry(2);
    const auto bright_green = gci->GetColorTableEntry(10);

    Log::Comment(NoThrowString().Format(
        L"cursor={X:%d,Y:%d}", 
        x, y
    ));
    VERIFY_ARE_EQUAL(x, 2);
    VERIFY_ARE_EQUAL(y, 0);

    const auto row = tbi->GetRowByOffset(y);
    const auto attrRow = &row->AttrRow;
    const auto len = tbi->_coordBufferSize.X;
    const auto attrs = new TextAttribute[len];
    VERIFY_IS_NOT_NULL(attrs);
    attrRow->UnpackAttrs(attrs, len);
    const auto attrA = attrs[x-2];
    const auto attrB = attrs[x-1];

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

    VERIFY_ARE_EQUAL(attrA.GetRgbForeground(), bright_green);
    VERIFY_ARE_EQUAL(attrB.GetRgbForeground(), dark_green);
    
    std::wstring reset = L"\x1b[0m";
    stateMachine->ProcessString(&reset[0], reset.length());
}

void TextBufferTests::TestUnBoldRgb()
{
    const CONSOLE_INFORMATION* const gci = ServiceLocator::LocateGlobals()->getConsoleInformation();
    SCREEN_INFORMATION* const psi = gci->CurrentScreenBuffer->GetActiveBuffer();
    const TEXT_BUFFER_INFO* const tbi = psi->TextInfo;
    StateMachine* const stateMachine = psi->GetStateMachine();
    Cursor* const cursor = tbi->GetCursor();
    SetFlag(psi->OutputMode, ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    VERIFY_IS_NOT_NULL(stateMachine);
    VERIFY_IS_NOT_NULL(cursor);

    cursor->SetXPosition(0);
    // Case 2 - 
    //      Write '\E[1;32m\E[48;2;1;2;3mX\E[22mX'
    //      The first X should be bright green, and not legacy.
    //      The second X should be dark green, and not legacy.
    //      BG = rgb(1;2;3)
    std::wstring sequence = L"\x1b[1;32m\x1b[48;2;1;2;3mX\x1b[22mX";
    stateMachine->ProcessString(&sequence[0], sequence.length());

    const auto x = cursor->GetPosition().X;
    const auto y = cursor->GetPosition().Y;
    const auto dark_green = gci->GetColorTableEntry(2);
    const auto bright_green = gci->GetColorTableEntry(10);

    Log::Comment(NoThrowString().Format(
        L"cursor={X:%d,Y:%d}", 
        x, y
    ));
    VERIFY_ARE_EQUAL(x, 2);
    VERIFY_ARE_EQUAL(y, 0);

    const auto row = tbi->GetRowByOffset(y);
    const auto attrRow = &row->AttrRow;
    const auto len = tbi->_coordBufferSize.X;
    const auto attrs = new TextAttribute[len];
    VERIFY_IS_NOT_NULL(attrs);
    attrRow->UnpackAttrs(attrs, len);
    const auto attrA = attrs[x-2];
    const auto attrB = attrs[x-1];

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

    VERIFY_ARE_EQUAL(attrA.GetRgbForeground(), bright_green);
    VERIFY_ARE_EQUAL(attrB.GetRgbForeground(), dark_green);
    
    std::wstring reset = L"\x1b[0m";
    stateMachine->ProcessString(&reset[0], reset.length());
}

void TextBufferTests::TestComplexUnBold()
{
    const CONSOLE_INFORMATION* const gci = ServiceLocator::LocateGlobals()->getConsoleInformation();
    SCREEN_INFORMATION* const psi = gci->CurrentScreenBuffer->GetActiveBuffer();
    const TEXT_BUFFER_INFO* const tbi = psi->TextInfo;
    StateMachine* const stateMachine = psi->GetStateMachine();
    Cursor* const cursor = tbi->GetCursor();
    SetFlag(psi->OutputMode, ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    VERIFY_IS_NOT_NULL(stateMachine);
    VERIFY_IS_NOT_NULL(cursor);

    cursor->SetXPosition(0);
    // Case 3 - 
    //      Write '\E[1;32m\E[48;2;1;2;3mX\E[22mX\E[38;2;32;32;32mX\E[1mX\E[38;2;64;64;64mX\E[22mX'
    //      The first X should be bright green, and not legacy.
    //      The second X should be dark green, and not legacy.
    //      The third X should be rgb(32, 32, 32), and not legacy.
    //      The fourth X should be bright green, again.
    //      The fifth X should be rgb(64, 64, 64), and not legacy.
    //      The sixth X should be dark green, again.
    //      BG = rgb(1;2;3)
    std::wstring sequence = L"\x1b[1;32m\x1b[48;2;1;2;3mX\x1b[22mX\x1b[38;2;32;32;32mX\x1b[1mX\x1b[38;2;64;64;64mX\x1b[22mX";
    stateMachine->ProcessString(&sequence[0], sequence.length());

    const auto x = cursor->GetPosition().X;
    const auto y = cursor->GetPosition().Y;
    const auto dark_green = gci->GetColorTableEntry(2);
    const auto bright_green = gci->GetColorTableEntry(10);

    Log::Comment(NoThrowString().Format(
        L"cursor={X:%d,Y:%d}", 
        x, y
    ));
    VERIFY_ARE_EQUAL(x, 6);
    VERIFY_ARE_EQUAL(y, 0);

    const auto row = tbi->GetRowByOffset(y);
    const auto attrRow = &row->AttrRow;
    const auto len = tbi->_coordBufferSize.X;
    const auto attrs = new TextAttribute[len];
    VERIFY_IS_NOT_NULL(attrs);
    attrRow->UnpackAttrs(attrs, len);
    const auto attrA = attrs[x-6];
    const auto attrB = attrs[x-5];
    const auto attrC = attrs[x-4];
    const auto attrD = attrs[x-3];
    const auto attrE = attrs[x-2];
    const auto attrF = attrs[x-1];

    Log::Comment(NoThrowString().Format(
        L"cursor={X:%d,Y:%d}", 
        x, y
    ));
    Log::Comment(NoThrowString().Format(
        L"attrA={IsLegacy:%d,GetLegacyAttributes:0x%x}", attrA.IsLegacy(), attrA.GetLegacyAttributes()
    ));
    Log::Comment(NoThrowString().Format(
        L"attrA={FG:0x%x,BG:0x%x}", attrA.GetRgbForeground(), attrA.GetRgbBackground()
    ));
    Log::Comment(NoThrowString().Format(
        L"attrB={IsLegacy:%d,GetLegacyAttributes:0x%x}", attrB.IsLegacy(), attrB.GetLegacyAttributes()
    ));
    Log::Comment(NoThrowString().Format(
        L"attrB={FG:0x%x,BG:0x%x}", attrB.GetRgbForeground(), attrB.GetRgbBackground()
    ));
    Log::Comment(NoThrowString().Format(
        L"attrC={IsLegacy:%d,GetLegacyAttributes:0x%x}", attrC.IsLegacy(), attrC.GetLegacyAttributes()
    ));
    Log::Comment(NoThrowString().Format(
        L"attrC={FG:0x%x,BG:0x%x}", attrC.GetRgbForeground(), attrC.GetRgbBackground()
    ));
    Log::Comment(NoThrowString().Format(
        L"attrD={IsLegacy:%d,GetLegacyAttributes:0x%x}", attrD.IsLegacy(), attrD.GetLegacyAttributes()
    ));
    Log::Comment(NoThrowString().Format(
        L"attrD={FG:0x%x,BG:0x%x}", attrD.GetRgbForeground(), attrD.GetRgbBackground()
    ));
    Log::Comment(NoThrowString().Format(
        L"attrE={IsLegacy:%d,GetLegacyAttributes:0x%x}", attrE.IsLegacy(), attrE.GetLegacyAttributes()
    ));
    Log::Comment(NoThrowString().Format(
        L"attrE={FG:0x%x,BG:0x%x}", attrE.GetRgbForeground(), attrE.GetRgbBackground()
    ));
    Log::Comment(NoThrowString().Format(
        L"attrF={IsLegacy:%d,GetLegacyAttributes:0x%x}", attrF.IsLegacy(), attrF.GetLegacyAttributes()
    ));
    Log::Comment(NoThrowString().Format(
        L"attrF={FG:0x%x,BG:0x%x}", attrF.GetRgbForeground(), attrF.GetRgbBackground()
    ));

    VERIFY_ARE_EQUAL(attrA.IsLegacy(), false);
    VERIFY_ARE_EQUAL(attrB.IsLegacy(), false);
    VERIFY_ARE_EQUAL(attrC.IsLegacy(), false);
    VERIFY_ARE_EQUAL(attrD.IsLegacy(), false);
    VERIFY_ARE_EQUAL(attrE.IsLegacy(), false);
    VERIFY_ARE_EQUAL(attrF.IsLegacy(), false);

    VERIFY_ARE_EQUAL(attrA.GetRgbForeground(), bright_green);
    VERIFY_ARE_EQUAL(attrA.GetRgbBackground(), RGB(1,2,3));

    VERIFY_ARE_EQUAL(attrB.GetRgbForeground(), dark_green);
    VERIFY_ARE_EQUAL(attrB.GetRgbBackground(), RGB(1,2,3));

    VERIFY_ARE_EQUAL(attrC.GetRgbForeground(), RGB(32,32,32));
    VERIFY_ARE_EQUAL(attrC.GetRgbBackground(), RGB(1,2,3));

    VERIFY_ARE_EQUAL(attrD.GetRgbForeground(), bright_green);
    VERIFY_ARE_EQUAL(attrD.GetRgbBackground(), RGB(1,2,3));

    VERIFY_ARE_EQUAL(attrE.GetRgbForeground(), RGB(64,64,64));
    VERIFY_ARE_EQUAL(attrE.GetRgbBackground(), RGB(1,2,3));

    VERIFY_ARE_EQUAL(attrF.GetRgbForeground(), dark_green);
    VERIFY_ARE_EQUAL(attrF.GetRgbBackground(), RGB(1,2,3));
    
    std::wstring reset = L"\x1b[0m";
    stateMachine->ProcessString(&reset[0], reset.length());
}


void TextBufferTests::CopyAttrs()
{
    const CONSOLE_INFORMATION* const gci = ServiceLocator::LocateGlobals()->getConsoleInformation();
    SCREEN_INFORMATION* const psi = gci->CurrentScreenBuffer->GetActiveBuffer();
    const TEXT_BUFFER_INFO* const tbi = psi->TextInfo;
    StateMachine* const stateMachine = psi->GetStateMachine();
    Cursor* const cursor = tbi->GetCursor();
    SetFlag(psi->OutputMode, ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    VERIFY_IS_NOT_NULL(stateMachine);
    VERIFY_IS_NOT_NULL(cursor);

    cursor->SetXPosition(0);
    cursor->SetYPosition(0);
    // Write '\E[32mX\E[33mX\n\E[34mX\E[35mX\E[H\E[M'
    // The first two X's should get deleted.
    // The third X should be blue
    // The fourth X should be magenta
    std::wstring sequence = L"\x1b[32mX\x1b[33mX\n\x1b[34mX\x1b[35mX\x1b[H\x1b[M";
    stateMachine->ProcessString(&sequence[0], sequence.length());

    const auto x = cursor->GetPosition().X;
    const auto y = cursor->GetPosition().Y;
    const auto dark_blue = gci->GetColorTableEntry(1);
    const auto dark_magenta = gci->GetColorTableEntry(5);

    Log::Comment(NoThrowString().Format(
        L"cursor={X:%d,Y:%d}", 
        x, y
    ));
    VERIFY_ARE_EQUAL(x, 0);
    VERIFY_ARE_EQUAL(y, 0);

    const auto row = tbi->GetRowByOffset(0);
    const auto attrRow = &row->AttrRow;
    const auto len = tbi->_coordBufferSize.X;
    const auto attrs = std::make_unique<TextAttribute[]>(len);
    VERIFY_IS_NOT_NULL(attrs);
    attrRow->UnpackAttrs(attrs.get(), len);
    const auto attrA = attrs[0];
    const auto attrB = attrs[1];

    Log::Comment(NoThrowString().Format(
        L"cursor={X:%d,Y:%d}", 
        x, y
    ));
    Log::Comment(NoThrowString().Format(
        L"attrA={IsLegacy:%d,GetLegacyAttributes:0x%x}", attrA.IsLegacy(), attrA.GetLegacyAttributes()
    ));
    Log::Comment(NoThrowString().Format(
        L"attrA={FG:0x%x,BG:0x%x}", attrA.GetRgbForeground(), attrA.GetRgbBackground()
    ));
    Log::Comment(NoThrowString().Format(
        L"attrB={IsLegacy:%d,GetLegacyAttributes:0x%x}", attrB.IsLegacy(), attrB.GetLegacyAttributes()
    ));
    Log::Comment(NoThrowString().Format(
        L"attrB={FG:0x%x,BG:0x%x}", attrB.GetRgbForeground(), attrB.GetRgbBackground()
    ));


    VERIFY_ARE_EQUAL(attrA.GetRgbForeground(), dark_blue);
    VERIFY_ARE_EQUAL(attrB.GetRgbForeground(), dark_magenta);

}

void TextBufferTests::EmptySgrTest()
{
    const CONSOLE_INFORMATION* const gci = ServiceLocator::LocateGlobals()->getConsoleInformation();
    SCREEN_INFORMATION* const psi = gci->CurrentScreenBuffer->GetActiveBuffer();
    const TEXT_BUFFER_INFO* const tbi = psi->TextInfo;
    StateMachine* const stateMachine = psi->GetStateMachine();
    Cursor* const cursor = tbi->GetCursor();
    SetFlag(psi->OutputMode, ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    VERIFY_IS_NOT_NULL(stateMachine);
    VERIFY_IS_NOT_NULL(cursor);

    cursor->SetXPosition(0);
    cursor->SetYPosition(0);

    std::wstring reset = L"\x1b[0m";
    stateMachine->ProcessString(&reset[0], reset.length());
    const COLORREF defaultFg = psi->GetAttributes().GetRgbForeground();
    const COLORREF defaultBg = psi->GetAttributes().GetRgbBackground();
    
    // Write '\x1b[0mX\x1b[31mX\x1b[31;m'
    // The first X should be default colors.
    // The second X should be (darkRed,default).
    // The third X should be default colors.
    std::wstring sequence = L"\x1b[0mX\x1b[31mX\x1b[31;mX";
    stateMachine->ProcessString(&sequence[0], sequence.length());

    const auto x = cursor->GetPosition().X;
    const auto y = cursor->GetPosition().Y;
    const COLORREF darkRed = gci->GetColorTableEntry(4);

    Log::Comment(NoThrowString().Format(
        L"cursor={X:%d,Y:%d}", 
        x, y
    ));
    VERIFY_IS_TRUE(x >= 3);

    const auto row = tbi->GetRowByOffset(y);
    const auto attrRow = &row->AttrRow;
    const auto len = tbi->_coordBufferSize.X;
    const auto attrs = new TextAttribute[len];
    VERIFY_IS_NOT_NULL(attrs);
    attrRow->UnpackAttrs(attrs, len);
    const auto attrA = attrs[x-3];
    const auto attrB = attrs[x-2];
    const auto attrC = attrs[x-1];

    Log::Comment(NoThrowString().Format(
        L"cursor={X:%d,Y:%d}", 
        x, y
    ));
    Log::Comment(NoThrowString().Format(
        L"attrA={IsLegacy:%d, GetLegacyAttributes:0x%x, FG:0x%x, BG:0x%x}", 
        attrA.IsLegacy(), attrA.GetLegacyAttributes(), attrA.GetRgbForeground(), attrA.GetRgbBackground()
    ));
    Log::Comment(NoThrowString().Format(
        L"attrB={IsLegacy:%d, GetLegacyAttributes:0x%x, FG:0x%x, BG:0x%x}", 
        attrB.IsLegacy(), attrB.GetLegacyAttributes(), attrB.GetRgbForeground(), attrB.GetRgbBackground()
    ));
    Log::Comment(NoThrowString().Format(
        L"attrC={IsLegacy:%d, GetLegacyAttributes:0x%x, FG:0x%x, BG:0x%x}", 
        attrC.IsLegacy(), attrC.GetLegacyAttributes(), attrC.GetRgbForeground(), attrC.GetRgbBackground()
    ));

    VERIFY_ARE_EQUAL(attrA.GetRgbForeground(), defaultFg);
    VERIFY_ARE_EQUAL(attrA.GetRgbBackground(), defaultBg);
    
    VERIFY_ARE_EQUAL(attrB.GetRgbForeground(), darkRed);
    VERIFY_ARE_EQUAL(attrB.GetRgbBackground(), defaultBg);

    VERIFY_ARE_EQUAL(attrC.GetRgbForeground(), defaultFg);
    VERIFY_ARE_EQUAL(attrC.GetRgbBackground(), defaultBg);

    stateMachine->ProcessString(&reset[0], reset.length());

}
