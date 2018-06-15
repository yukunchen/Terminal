/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/
#include "precomp.h"
#include "WexTestClass.h"

#include "CommonState.hpp"

#include "globals.h"
#include "../buffer/out/textBuffer.hpp"
#include "../buffer/out/CharRow.hpp"

#include "input.h"

#include "../interactivity/inc/ServiceLocator.hpp"

using namespace WEX::Common;
using namespace WEX::Logging;
using namespace WEX::TestExecution;

#define LOG_ATTR(attr) (Log::Comment(NoThrowString().Format(\
    L#attr L"=%s", VerifyOutputTraits<TextAttribute>::ToString(attr).GetBuffer())))

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

    void DoBufferRowIterationTest(TextBuffer& textBuffer);

    TextBuffer& GetTbi();

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

    TEST_METHOD(TestReverseReset);

    TEST_METHOD(CopyLastAttr);

    TEST_METHOD(TestTextAttributeColorGetters);
    TEST_METHOD(TestRgbThenBold);
    TEST_METHOD(TestResetClearsBoldness);

};

void TextBufferTests::TestBufferCreate()
{
    VERIFY_SUCCESS_NTSTATUS(m_state->GetTextBufferInfoInitResult());
}

void TextBufferTests::DoBufferRowIterationTest(TextBuffer& textBuffer)
{
    const ROW* pFirstRow = &textBuffer.GetFirstRow();
    const ROW* pPrior = nullptr;
    const ROW* pRow = pFirstRow;
    VERIFY_IS_NOT_NULL(pRow);

    UINT cRows = 0;
    while (pRow != nullptr)
    {
        cRows++;
        pPrior = pRow; // save off the previous for a reverse check
        try
        {
            pRow = &textBuffer.GetNextRowNoWrap(*pRow);
        }
        catch (...)
        {
            pRow = nullptr;
        }

        // if we didn't just hit the last row...
        if (pRow != nullptr)
        {
            // grab the previous row from the one we just found
            const ROW* pPriorCheck;
            try
            {
                pPriorCheck = &textBuffer.GetPrevRowNoWrap(*pRow);
            }
            catch (...)
            {
                pPriorCheck = nullptr;
            }

            // it should still equal what the row was before we started this iteration
            VERIFY_ARE_EQUAL(pPrior, pPriorCheck);
        }
    }

    // the number of rows we iterated through should be the same as the window size.
    VERIFY_ARE_EQUAL(textBuffer.TotalRowCount(), cRows);
}

TextBuffer& TextBufferTests::GetTbi()
{
    CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    return gci.GetActiveOutputBuffer().GetTextBuffer();
}

SHORT TextBufferTests::GetBufferWidth()
{
    return GetTbi()._coordBufferSize.X;
}

SHORT TextBufferTests::GetBufferHeight()
{
    return GetTbi()._coordBufferSize.Y;
}


void TextBufferTests::TestBufferRowIteration()
{
    DoBufferRowIterationTest(GetTbi());
}

void TextBufferTests::TestBufferRowIterationWhenCircular()
{
    SHORT csBufferHeight = GetBufferHeight();

    VERIFY_IS_TRUE(csBufferHeight > 4);

    TextBuffer& textBuffer = GetTbi();

    textBuffer._FirstRow = csBufferHeight / 2 + 2; // sets to a bit over half way around.

    DoBufferRowIterationTest(textBuffer);
}

void TextBufferTests::TestBufferRowByOffset()
{
    TextBuffer& textBuffer = GetTbi();
    SHORT csBufferHeight = GetBufferHeight();

    VERIFY_IS_TRUE(csBufferHeight > 20);

    short sId = csBufferHeight / 2 - 5;

    const ROW& row = textBuffer.GetRowByOffset(sId);
    VERIFY_ARE_EQUAL(row.GetId(), sId);
}

void TextBufferTests::TestWrapFlag()
{
    TextBuffer& textBuffer = GetTbi();

    ROW& Row = textBuffer.GetFirstRow();

    // no wrap by default
    VERIFY_IS_FALSE(Row.GetCharRow().WasWrapForced());

    // try set wrap and check
    Row.GetCharRow().SetWrapForced(true);
    VERIFY_IS_TRUE(Row.GetCharRow().WasWrapForced());

    // try unset wrap and check
    Row.GetCharRow().SetWrapForced(false);
    VERIFY_IS_FALSE(Row.GetCharRow().WasWrapForced());
}

void TextBufferTests::TestDoubleBytePadFlag()
{
    TextBuffer& textBuffer = GetTbi();

    ROW& Row = textBuffer.GetFirstRow();

    // no padding by default
    VERIFY_IS_FALSE(Row.GetCharRow().WasDoubleBytePadded());

    // try set and check
    Row.GetCharRow().SetDoubleBytePadded(true);
    VERIFY_IS_TRUE(Row.GetCharRow().WasDoubleBytePadded());

    // try unset and check
    Row.GetCharRow().SetDoubleBytePadded(false);
    VERIFY_IS_FALSE(Row.GetCharRow().WasDoubleBytePadded());
}


void TextBufferTests::DoBoundaryTest(PWCHAR const pwszInputString,
                                     short const cLength,
                                     short const cMax,
                                     short const cLeft,
                                     short const cRight)
{
    TextBuffer& textBuffer = GetTbi();

    CharRow& charRow = textBuffer.GetFirstRow().GetCharRow();

    // copy string into buffer
    for (size_t i = 0; i < static_cast<size_t>(cLength); ++i)
    {
        charRow.GlyphAt(i) = { pwszInputString[i] };
    }

    // space pad the rest of the string
    if (cLength < cMax)
    {
        for (short cStart = cLength; cStart < cMax; cStart++)
        {
            charRow.ClearGlyph(cStart);
        }
    }

    // left edge should be 0 since there are no leading spaces
    VERIFY_ARE_EQUAL(charRow.MeasureLeft(), static_cast<size_t>(cLeft));
    // right edge should be one past the index of the last character or the string length
    VERIFY_ARE_EQUAL(charRow.MeasureRight(), static_cast<size_t>(cRight));
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
    TextBuffer& otherTbi = GetTbi();

    std::unique_ptr<TextBuffer> testTextBuffer = std::make_unique<TextBuffer>(otherTbi._fiCurrentFont,
                                                                                          otherTbi._coordBufferSize,
                                                                                          otherTbi._ciFill,
                                                                                          12);
    VERIFY_IS_NOT_NULL(testTextBuffer.get());

    // set initial mapping values
    testTextBuffer->GetCursor().SetHasMoved(false);
    otherTbi.GetCursor().SetHasMoved(true);

    testTextBuffer->GetCursor().SetIsVisible(false);
    otherTbi.GetCursor().SetIsVisible(true);

    testTextBuffer->GetCursor().SetIsOn(false);
    otherTbi.GetCursor().SetIsOn(true);

    testTextBuffer->GetCursor().SetIsDouble(false);
    otherTbi.GetCursor().SetIsDouble(true);

    testTextBuffer->GetCursor().SetDelay(false);
    otherTbi.GetCursor().SetDelay(true);

    // run copy
    testTextBuffer->CopyProperties(otherTbi);

    // test that new now contains values from other
    VERIFY_IS_TRUE(testTextBuffer->GetCursor().HasMoved());
    VERIFY_IS_TRUE(testTextBuffer->GetCursor().IsVisible());
    VERIFY_IS_TRUE(testTextBuffer->GetCursor().IsOn());
    VERIFY_IS_TRUE(testTextBuffer->GetCursor().IsDouble());
    VERIFY_IS_TRUE(testTextBuffer->GetCursor().GetDelay());
}

void TextBufferTests::TestInsertCharacter()
{
    TextBuffer& textBuffer = GetTbi();

    // get starting cursor position
    COORD const coordCursorBefore = textBuffer.GetCursor().GetPosition();

    // Get current row from the buffer
    ROW& Row = textBuffer.GetRowByOffset(coordCursorBefore.Y);

    // create some sample test data
    const std::vector<wchar_t> wchTest{ L'Z' };
    DbcsAttribute dbcsAttribute;
    dbcsAttribute.SetTrailing();
    WORD const wAttrTest = BACKGROUND_INTENSITY | FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_BLUE;
    TextAttribute TestAttributes = TextAttribute(wAttrTest);

    CharRow& charRow = Row.GetCharRow();
    charRow.DbcsAttrAt(coordCursorBefore.X).SetLeading();
    // ensure that the buffer didn't start with these fields
    VERIFY_ARE_NOT_EQUAL(charRow.GlyphAt(coordCursorBefore.X), wchTest);
    VERIFY_ARE_NOT_EQUAL(charRow.DbcsAttrAt(coordCursorBefore.X), dbcsAttribute);

    auto attr = Row.GetAttrRow().GetAttrByColumn(coordCursorBefore.X);

    VERIFY_ARE_NOT_EQUAL(attr, TestAttributes);

    // now apply the new data to the buffer
    textBuffer.InsertCharacter(wchTest, dbcsAttribute, TestAttributes);

    // ensure that the buffer position where the cursor WAS contains the test items
    VERIFY_ARE_EQUAL(charRow.GlyphAt(coordCursorBefore.X), wchTest);
    VERIFY_ARE_EQUAL(charRow.DbcsAttrAt(coordCursorBefore.X), dbcsAttribute);

    attr = Row.GetAttrRow().GetAttrByColumn(coordCursorBefore.X);
    VERIFY_ARE_EQUAL(attr, TestAttributes);

    // ensure that the cursor moved to a new position (X or Y or both have changed)
    VERIFY_IS_TRUE((coordCursorBefore.X != textBuffer.GetCursor().GetPosition().X) ||
                   (coordCursorBefore.Y != textBuffer.GetCursor().GetPosition().Y));
    // the proper advancement of the cursor (e.g. which position it goes to) is validated in other tests
}

void TextBufferTests::TestIncrementCursor()
{
    TextBuffer& textBuffer = GetTbi();

    // only checking X increments here
    // Y increments are covered in the NewlineCursor test

    short const sBufferWidth = textBuffer._coordBufferSize.X;

    short const sBufferHeight = textBuffer._coordBufferSize.Y;
    VERIFY_IS_TRUE(sBufferWidth > 1 && sBufferHeight > 1);

    Log::Comment(L"Test normal case of moving once to the right within a single line");
    textBuffer.GetCursor().SetXPosition(0);
    textBuffer.GetCursor().SetYPosition(0);

    COORD coordCursorBefore = textBuffer.GetCursor().GetPosition();

    textBuffer.IncrementCursor();

    VERIFY_ARE_EQUAL(textBuffer.GetCursor().GetPosition().X, 1); // X should advance by 1
    VERIFY_ARE_EQUAL(textBuffer.GetCursor().GetPosition().Y, coordCursorBefore.Y); // Y shouldn't have moved

    Log::Comment(L"Test line wrap case where cursor is on the right edge of the line");
    textBuffer.GetCursor().SetXPosition(sBufferWidth - 1);
    textBuffer.GetCursor().SetYPosition(0);

    coordCursorBefore = textBuffer.GetCursor().GetPosition();

    textBuffer.IncrementCursor();

    VERIFY_ARE_EQUAL(textBuffer.GetCursor().GetPosition().X, 0); // position should be reset to the left edge when passing right edge
    VERIFY_ARE_EQUAL(textBuffer.GetCursor().GetPosition().Y - 1, coordCursorBefore.Y); // the cursor should be moved one row down from where it used to be
}

void TextBufferTests::TestNewlineCursor()
{
    TextBuffer& textBuffer = GetTbi();


    const short sBufferHeight = textBuffer._coordBufferSize.Y;

    const short sBufferWidth = textBuffer._coordBufferSize.X;
    // width and height are sufficiently large for upcoming math
    VERIFY_IS_TRUE(sBufferWidth > 4 && sBufferHeight > 4);

    Log::Comment(L"Verify standard row increment from somewhere in the buffer");

    // set cursor X position to non zero, any position in buffer
    textBuffer.GetCursor().SetXPosition(3);

    // set cursor Y position to not-the-final row in the buffer
    textBuffer.GetCursor().SetYPosition(3);

    COORD coordCursorBefore = textBuffer.GetCursor().GetPosition();

    // perform operation
    textBuffer.NewlineCursor();

    // verify
    VERIFY_ARE_EQUAL(textBuffer.GetCursor().GetPosition().X, 0); // move to left edge of buffer
    VERIFY_ARE_EQUAL(textBuffer.GetCursor().GetPosition().Y, coordCursorBefore.Y + 1); // move down one row

    Log::Comment(L"Verify increment when already on last row of buffer");

    // X position still doesn't matter
    textBuffer.GetCursor().SetXPosition(3);

    // Y position needs to be on the last row of the buffer
    textBuffer.GetCursor().SetYPosition(sBufferHeight - 1);

    coordCursorBefore = textBuffer.GetCursor().GetPosition();

    // perform operation
    textBuffer.NewlineCursor();

    // verify
    VERIFY_ARE_EQUAL(textBuffer.GetCursor().GetPosition().X, 0); // move to left edge
    VERIFY_ARE_EQUAL(textBuffer.GetCursor().GetPosition().Y, coordCursorBefore.Y); // cursor Y position should not have moved. stays on same logical final line of buffer

    // This is okay because the backing circular buffer changes, not the logical screen position (final visible line of the buffer)
}

void TextBufferTests::TestLastNonSpace(short const cursorPosY)
{
    TextBuffer& textBuffer = GetTbi();
    textBuffer.GetCursor().SetYPosition(cursorPosY);

    COORD coordLastNonSpace = textBuffer.GetLastNonSpaceCharacter();

    // We expect the last non space character to be the last printable character in the row.
    // The .Right property on a row is 1 past the last printable character in the row.
    // If there is one character in the row, the last character would be 0.
    // If there are no characters in the row, the last character would be -1 and we need to seek backwards to find the previous row with a character.

    // start expected position from cursor
    COORD coordExpected = textBuffer.GetCursor().GetPosition();

    // Try to get the X position from the current cursor position.
    coordExpected.X = static_cast<short>(textBuffer.GetRowByOffset(coordExpected.Y).GetCharRow().MeasureRight()) - 1;

    // If we went negative, this row was empty and we need to continue seeking upward...
    // - As long as X is negative (empty rows)
    // - As long as we have space before the top of the buffer (Y isn't the 0th/top row).
    while (coordExpected.X < 0 && coordExpected.Y > 0)
    {
        coordExpected.Y--;
        coordExpected.X = static_cast<short>(textBuffer.GetRowByOffset(coordExpected.Y).GetCharRow().MeasureRight()) - 1;
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
    TextBuffer& textBuffer = GetTbi();

    short sCurrentRow = textBuffer.GetCursor().GetPosition().Y;

    ROW& Row = textBuffer.GetRowByOffset(sCurrentRow);

    Log::Comment(L"Testing off to on");

    // turn wrap status off first
    Row.GetCharRow().SetWrapForced(false);

    // trigger wrap
    textBuffer.SetWrapOnCurrentRow();

    // ensure this row was flipped
    VERIFY_IS_TRUE(Row.GetCharRow().WasWrapForced());

    Log::Comment(L"Testing on stays on");

    // make sure wrap status is on
    Row.GetCharRow().SetWrapForced(true);

    // trigger wrap
    textBuffer.SetWrapOnCurrentRow();

    // ensure row is still on
    VERIFY_IS_TRUE(Row.GetCharRow().WasWrapForced());
}

void TextBufferTests::TestIncrementCircularBuffer()
{
    TextBuffer& textBuffer = GetTbi();

    short const sBufferHeight = textBuffer._coordBufferSize.Y;

    VERIFY_IS_TRUE(sBufferHeight > 4); // buffer should be sufficiently large

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

        textBuffer._FirstRow = iRowToTestIndex;

        // fill first row with some stuff
        ROW& FirstRow = textBuffer.GetFirstRow();
        CharRow& charRow = FirstRow.GetCharRow();
        charRow.GlyphAt(0) = { L'A' };

        // ensure it does say that it contains text
        VERIFY_IS_TRUE(FirstRow.GetCharRow().ContainsText());

        // try increment
        textBuffer.IncrementCircularBuffer();

        // validate that first row has moved
        VERIFY_ARE_EQUAL(textBuffer._FirstRow, iNextRowIndex); // first row has incremented
        VERIFY_ARE_NOT_EQUAL(textBuffer.GetFirstRow(), FirstRow); // the old first row is no longer the first

        // ensure old first row has been emptied
        VERIFY_IS_FALSE(FirstRow.GetCharRow().ContainsText());
    }
}

void TextBufferTests::TestMixedRgbAndLegacyForeground()
{
    CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    SCREEN_INFORMATION& si = gci.GetActiveOutputBuffer().GetActiveBuffer();
    const TextBuffer& tbi = si.GetTextBuffer();
    StateMachine* const stateMachine = si.GetStateMachine();
    const Cursor& cursor = tbi.GetCursor();
    VERIFY_IS_NOT_NULL(stateMachine);

    // Case 1 -
    //      Write '\E[38;2;64;128;255mX\E[49mX\E[m'
    //      Make sure that the second X has RGB attributes (FG and BG)
    //      FG = rgb(64;128;255), BG = rgb(default)
    Log::Comment(L"Case 1 \"\\E[38;2;64;128;255mX\\E[49mX\\E[m\"");

    wchar_t* sequence = L"\x1b[38;2;64;128;255mX\x1b[49mX\x1b[m";

    stateMachine->ProcessString(sequence, std::wcslen(sequence));
    const short x = cursor.GetPosition().X;
    const short y = cursor.GetPosition().Y;
    const ROW& row = tbi.GetRowByOffset(y);
    const auto attrRow = &row.GetAttrRow();
    const std::vector<TextAttribute> attrs{ attrRow->begin(), attrRow->end() };
    const auto attrA = attrs[x-2];
    const auto attrB = attrs[x-1];
    Log::Comment(NoThrowString().Format(
        L"cursor={X:%d,Y:%d}",
        x, y
    ));

    LOG_ATTR(attrA);
    LOG_ATTR(attrB);

    VERIFY_ARE_EQUAL(attrA.IsLegacy(), false);
    VERIFY_ARE_EQUAL(attrB.IsLegacy(), false);

    VERIFY_ARE_EQUAL(attrA.CalculateRgbForeground(), RGB(64,128,255));
    VERIFY_ARE_EQUAL(attrA.CalculateRgbBackground(), si.GetAttributes().CalculateRgbBackground());

    VERIFY_ARE_EQUAL(attrB.CalculateRgbForeground(), RGB(64,128,255));
    VERIFY_ARE_EQUAL(attrB.CalculateRgbBackground(), si.GetAttributes().CalculateRgbBackground());

    wchar_t* reset = L"\x1b[0m";
    stateMachine->ProcessString(reset, std::wcslen(reset));

}

void TextBufferTests::TestMixedRgbAndLegacyBackground()
{
    CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    SCREEN_INFORMATION& si = gci.GetActiveOutputBuffer().GetActiveBuffer();
    const TextBuffer& tbi = si.GetTextBuffer();
    StateMachine* const stateMachine = si.GetStateMachine();
    const Cursor& cursor = tbi.GetCursor();
    VERIFY_IS_NOT_NULL(stateMachine);

    // Case 2 -
    //      \E[48;2;64;128;255mX\E[39mX\E[m
    //      Make sure that the second X has RGB attributes (FG and BG)
    //      FG = rgb(default), BG = rgb(64;128;255)
    Log::Comment(L"Case 2 \"\\E[48;2;64;128;255mX\\E[39mX\\E[m\"");

    wchar_t* sequence = L"\x1b[48;2;64;128;255mX\x1b[39mX\x1b[m";
    stateMachine->ProcessString(sequence, std::wcslen(sequence));
    const auto x = cursor.GetPosition().X;
    const auto y = cursor.GetPosition().Y;
    const auto& row = tbi.GetRowByOffset(y);
    const auto attrRow = &row.GetAttrRow();
    const std::vector<TextAttribute> attrs{ attrRow->begin(), attrRow->end() };
    const auto attrA = attrs[x-2];
    const auto attrB = attrs[x-1];
    Log::Comment(NoThrowString().Format(
        L"cursor={X:%d,Y:%d}",
        x, y
    ));

    LOG_ATTR(attrA);
    LOG_ATTR(attrB);

    VERIFY_ARE_EQUAL(attrA.IsLegacy(), false);
    VERIFY_ARE_EQUAL(attrB.IsLegacy(), false);

    VERIFY_ARE_EQUAL(attrA.CalculateRgbBackground(), RGB(64,128,255));
    VERIFY_ARE_EQUAL(attrA.CalculateRgbForeground(), si.GetAttributes().CalculateRgbForeground());

    VERIFY_ARE_EQUAL(attrB.CalculateRgbBackground(), RGB(64,128,255));
    VERIFY_ARE_EQUAL(attrB.CalculateRgbForeground(), si.GetAttributes().CalculateRgbForeground());

    wchar_t* reset = L"\x1b[0m";
    stateMachine->ProcessString(reset, std::wcslen(reset));
}

void TextBufferTests::TestMixedRgbAndLegacyUnderline()
{
    CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    SCREEN_INFORMATION& si = gci.GetActiveOutputBuffer().GetActiveBuffer();
    const TextBuffer& tbi = si.GetTextBuffer();
    StateMachine* const stateMachine = si.GetStateMachine();
    const Cursor& cursor = tbi.GetCursor();
    VERIFY_IS_NOT_NULL(stateMachine);

    // Case 3 -
    //      '\E[48;2;64;128;255mX\E[4mX\E[m'
    //      Make sure that the second X has RGB attributes AND underline
    Log::Comment(L"Case 3 \"\\E[48;2;64;128;255mX\\E[4mX\\E[m\"");
    wchar_t* sequence = L"\x1b[48;2;64;128;255mX\x1b[4mX\x1b[m";
    stateMachine->ProcessString(sequence, std::wcslen(sequence));
    const auto x = cursor.GetPosition().X;
    const auto y = cursor.GetPosition().Y;
    const auto& row = tbi.GetRowByOffset(y);
    const auto attrRow = &row.GetAttrRow();
    const std::vector<TextAttribute> attrs{ attrRow->begin(), attrRow->end() };
    const auto attrA = attrs[x-2];
    const auto attrB = attrs[x-1];
    Log::Comment(NoThrowString().Format(
        L"cursor={X:%d,Y:%d}",
        x, y
    ));

    LOG_ATTR(attrA);
    LOG_ATTR(attrB);

    VERIFY_ARE_EQUAL(attrA.IsLegacy(), false);
    VERIFY_ARE_EQUAL(attrB.IsLegacy(), false);

    VERIFY_ARE_EQUAL(attrA.CalculateRgbBackground(), RGB(64,128,255));
    VERIFY_ARE_EQUAL(attrA.CalculateRgbForeground(), si.GetAttributes().CalculateRgbForeground());

    VERIFY_ARE_EQUAL(attrB.CalculateRgbBackground(), RGB(64,128,255));
    VERIFY_ARE_EQUAL(attrB.CalculateRgbForeground(), si.GetAttributes().CalculateRgbForeground());

    VERIFY_ARE_EQUAL(attrA.GetLegacyAttributes()&COMMON_LVB_UNDERSCORE, 0);
    VERIFY_ARE_EQUAL(attrB.GetLegacyAttributes()&COMMON_LVB_UNDERSCORE, COMMON_LVB_UNDERSCORE);

    wchar_t* reset = L"\x1b[0m";
    stateMachine->ProcessString(reset, std::wcslen(reset));

}

void TextBufferTests::TestMixedRgbAndLegacyBrightness()
{
    CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    SCREEN_INFORMATION& si = gci.GetActiveOutputBuffer().GetActiveBuffer();
    const TextBuffer& tbi = si.GetTextBuffer();
    StateMachine* const stateMachine = si.GetStateMachine();
    const Cursor& cursor = tbi.GetCursor();
    VERIFY_IS_NOT_NULL(stateMachine);
    // Case 4 -
    //      '\E[32mX\E[1mX'
    //      Make sure that the second X is a BRIGHT green, not white.
    Log::Comment(L"Case 4 ;\"\\E[32mX\\E[1mX\"");
    const auto dark_green = gci.GetColorTableEntry(2);
    const auto bright_green = gci.GetColorTableEntry(10);
    VERIFY_ARE_NOT_EQUAL(dark_green, bright_green);

    wchar_t* sequence = L"\x1b[32mX\x1b[1mX";
    stateMachine->ProcessString(sequence, std::wcslen(sequence));
    const auto x = cursor.GetPosition().X;
    const auto y = cursor.GetPosition().Y;
    const auto& row = tbi.GetRowByOffset(y);
    const auto attrRow = &row.GetAttrRow();
    const std::vector<TextAttribute> attrs{ attrRow->begin(), attrRow->end() };
    const auto attrA = attrs[x-2];
    const auto attrB = attrs[x-1];
    Log::Comment(NoThrowString().Format(
        L"cursor={X:%d,Y:%d}",
        x, y
    ));

    LOG_ATTR(attrA);
    LOG_ATTR(attrB);

    VERIFY_ARE_EQUAL(attrA.IsLegacy(), false);
    VERIFY_ARE_EQUAL(attrB.IsLegacy(), false);

    VERIFY_ARE_EQUAL(attrA.CalculateRgbForeground(), dark_green);
    VERIFY_ARE_EQUAL(attrB.CalculateRgbForeground(), bright_green);

    wchar_t* reset = L"\x1b[0m";
    stateMachine->ProcessString(reset, std::wcslen(reset));
}

void TextBufferTests::TestRgbEraseLine()
{
    CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    SCREEN_INFORMATION& si = gci.GetActiveOutputBuffer().GetActiveBuffer();
    TextBuffer& tbi = si.GetTextBuffer();
    StateMachine* const stateMachine = si.GetStateMachine();
    Cursor& cursor = tbi.GetCursor();
    SetFlag(si.OutputMode, ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    VERIFY_IS_NOT_NULL(stateMachine);

    cursor.SetXPosition(0);
    // Case 1 -
    //      Write '\E[48;2;64;128;255X\E[48;2;128;128;255\E[KX'
    //      Make sure that all the characters after the first have the rgb attrs
    //      BG = rgb(128;128;255)
    {
        std::wstring sequence = L"\x1b[48;2;64;128;255mX\x1b[48;2;128;128;255m\x1b[KX";
        stateMachine->ProcessString(&sequence[0], sequence.length());

        const auto x = cursor.GetPosition().X;
        const auto y = cursor.GetPosition().Y;

        Log::Comment(NoThrowString().Format(
            L"cursor={X:%d,Y:%d}",
            x, y
        ));
        VERIFY_ARE_EQUAL(x, 2);
        VERIFY_ARE_EQUAL(y, 0);

        const auto& row = tbi.GetRowByOffset(y);
        const auto attrRow = &row.GetAttrRow();
        const auto len = tbi._coordBufferSize.X;
        const std::vector<TextAttribute> attrs{ attrRow->begin(), attrRow->end() };

        const auto attr0 = attrs[0];

        VERIFY_ARE_EQUAL(attr0.IsLegacy(), false);
        VERIFY_ARE_EQUAL(attr0.CalculateRgbBackground(), RGB(64,128,255));
        for (auto i = 1; i < len; i++)
        {
            const auto attr = attrs[i];
            LOG_ATTR(attr);
            VERIFY_ARE_EQUAL(attr.IsLegacy(), false);
            VERIFY_ARE_EQUAL(attr.CalculateRgbBackground(), RGB(128,128,255));

        }
        std::wstring reset = L"\x1b[0m";
        stateMachine->ProcessString(&reset[0], reset.length());
    }
}

void TextBufferTests::TestUnBold()
{
    CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    SCREEN_INFORMATION& si = gci.GetActiveOutputBuffer().GetActiveBuffer();
    TextBuffer& tbi = si.GetTextBuffer();
    StateMachine* const stateMachine = si.GetStateMachine();
    Cursor& cursor = tbi.GetCursor();
    SetFlag(si.OutputMode, ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    VERIFY_IS_NOT_NULL(stateMachine);

    cursor.SetXPosition(0);
    // Case 1 -
    //      Write '\E[1;32mX\E[22mX'
    //      The first X should be bright green.
    //      The second x should be dark green.
    std::wstring sequence = L"\x1b[1;32mX\x1b[22mX";
    stateMachine->ProcessString(&sequence[0], sequence.length());

    const auto x = cursor.GetPosition().X;
    const auto y = cursor.GetPosition().Y;
    const auto dark_green = gci.GetColorTableEntry(2);
    const auto bright_green = gci.GetColorTableEntry(10);

    Log::Comment(NoThrowString().Format(
        L"cursor={X:%d,Y:%d}",
        x, y
    ));
    VERIFY_ARE_EQUAL(x, 2);
    VERIFY_ARE_EQUAL(y, 0);

    const auto& row = tbi.GetRowByOffset(y);
    const auto attrRow = &row.GetAttrRow();
    const auto len = tbi._coordBufferSize.X;
    const std::vector<TextAttribute> attrs{ attrRow->begin(), attrRow->end() };
    const auto attrA = attrs[x-2];
    const auto attrB = attrs[x-1];

    Log::Comment(NoThrowString().Format(
        L"cursor={X:%d,Y:%d}",
        x, y
    ));

    LOG_ATTR(attrA);
    LOG_ATTR(attrB);

    VERIFY_ARE_EQUAL(attrA.CalculateRgbForeground(), bright_green);
    VERIFY_ARE_EQUAL(attrB.CalculateRgbForeground(), dark_green);

    std::wstring reset = L"\x1b[0m";
    stateMachine->ProcessString(&reset[0], reset.length());
}

void TextBufferTests::TestUnBoldRgb()
{
    CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    SCREEN_INFORMATION& si = gci.GetActiveOutputBuffer().GetActiveBuffer();
    TextBuffer& tbi = si.GetTextBuffer();
    StateMachine* const stateMachine = si.GetStateMachine();
    Cursor& cursor = tbi.GetCursor();
    SetFlag(si.OutputMode, ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    VERIFY_IS_NOT_NULL(stateMachine);

    cursor.SetXPosition(0);
    // Case 2 -
    //      Write '\E[1;32m\E[48;2;1;2;3mX\E[22mX'
    //      The first X should be bright green, and not legacy.
    //      The second X should be dark green, and not legacy.
    //      BG = rgb(1;2;3)
    std::wstring sequence = L"\x1b[1;32m\x1b[48;2;1;2;3mX\x1b[22mX";
    stateMachine->ProcessString(&sequence[0], sequence.length());

    const auto x = cursor.GetPosition().X;
    const auto y = cursor.GetPosition().Y;
    const auto dark_green = gci.GetColorTableEntry(2);
    const auto bright_green = gci.GetColorTableEntry(10);

    Log::Comment(NoThrowString().Format(
        L"cursor={X:%d,Y:%d}",
        x, y
    ));
    VERIFY_ARE_EQUAL(x, 2);
    VERIFY_ARE_EQUAL(y, 0);

    const auto& row = tbi.GetRowByOffset(y);
    const auto attrRow = &row.GetAttrRow();
    const auto len = tbi._coordBufferSize.X;
    const std::vector<TextAttribute> attrs{ attrRow->begin(), attrRow->end() };
    const auto attrA = attrs[x-2];
    const auto attrB = attrs[x-1];

    Log::Comment(NoThrowString().Format(
        L"cursor={X:%d,Y:%d}",
        x, y
    ));

    LOG_ATTR(attrA);
    LOG_ATTR(attrB);

    VERIFY_ARE_EQUAL(attrA.IsLegacy(), false);
    VERIFY_ARE_EQUAL(attrB.IsLegacy(), false);

    VERIFY_ARE_EQUAL(attrA.CalculateRgbForeground(), bright_green);
    VERIFY_ARE_EQUAL(attrB.CalculateRgbForeground(), dark_green);

    std::wstring reset = L"\x1b[0m";
    stateMachine->ProcessString(&reset[0], reset.length());
}

void TextBufferTests::TestComplexUnBold()
{
    CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    SCREEN_INFORMATION& si = gci.GetActiveOutputBuffer().GetActiveBuffer();
    TextBuffer& tbi = si.GetTextBuffer();
    StateMachine* const stateMachine = si.GetStateMachine();
    Cursor& cursor = tbi.GetCursor();
    SetFlag(si.OutputMode, ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    VERIFY_IS_NOT_NULL(stateMachine);

    cursor.SetXPosition(0);
    // Case 3 -
    //      Write '\E[1;32m\E[48;2;1;2;3mA\E[22mB\E[38;2;32;32;32mC\E[1mD\E[38;2;64;64;64mE\E[22mF'
    //      The A should be bright green, and not legacy.
    //      The B should be dark green, and not legacy.
    //      The C should be rgb(32, 32, 32), and not legacy.
    //      The D should be unchanged from the third.
    //      The E should be rgb(64, 64, 64), and not legacy.
    //      The F should be rgb(64, 64, 64), and not legacy.
    //      BG = rgb(1;2;3)
    std::wstring sequence = L"\x1b[1;32m\x1b[48;2;1;2;3mA\x1b[22mB\x1b[38;2;32;32;32mC\x1b[1mD\x1b[38;2;64;64;64mE\x1b[22mF";
    Log::Comment(NoThrowString().Format(sequence.c_str()));
    stateMachine->ProcessString(&sequence[0], sequence.length());

    const auto x = cursor.GetPosition().X;
    const auto y = cursor.GetPosition().Y;
    const auto dark_green = gci.GetColorTableEntry(2);
    const auto bright_green = gci.GetColorTableEntry(10);

    Log::Comment(NoThrowString().Format(
        L"cursor={X:%d,Y:%d}",
        x, y
    ));
    VERIFY_ARE_EQUAL(x, 6);
    VERIFY_ARE_EQUAL(y, 0);

    const auto& row = tbi.GetRowByOffset(y);
    const auto attrRow = &row.GetAttrRow();
    const auto len = tbi._coordBufferSize.X;
    const std::vector<TextAttribute> attrs{ attrRow->begin(), attrRow->end() };
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
        L"attrA=%s", VerifyOutputTraits<TextAttribute>::ToString(attrA).GetBuffer()
    ));
    LOG_ATTR(attrA);
    LOG_ATTR(attrB);
    LOG_ATTR(attrC);
    LOG_ATTR(attrD);
    LOG_ATTR(attrE);
    LOG_ATTR(attrF);

    VERIFY_ARE_EQUAL(attrA.IsLegacy(), false);
    VERIFY_ARE_EQUAL(attrB.IsLegacy(), false);
    VERIFY_ARE_EQUAL(attrC.IsLegacy(), false);
    VERIFY_ARE_EQUAL(attrD.IsLegacy(), false);
    VERIFY_ARE_EQUAL(attrE.IsLegacy(), false);
    VERIFY_ARE_EQUAL(attrF.IsLegacy(), false);

    VERIFY_ARE_EQUAL(attrA.CalculateRgbForeground(), bright_green);
    VERIFY_ARE_EQUAL(attrA.CalculateRgbBackground(), RGB(1,2,3));
    VERIFY_IS_TRUE(attrA.IsBold());

    VERIFY_ARE_EQUAL(attrB.CalculateRgbForeground(), dark_green);
    VERIFY_ARE_EQUAL(attrB.CalculateRgbBackground(), RGB(1,2,3));
    VERIFY_IS_FALSE(attrB.IsBold());

    VERIFY_ARE_EQUAL(attrC.CalculateRgbForeground(), RGB(32,32,32));
    VERIFY_ARE_EQUAL(attrC.CalculateRgbBackground(), RGB(1,2,3));
    VERIFY_IS_FALSE(attrC.IsBold());

    VERIFY_ARE_EQUAL(attrD.CalculateRgbForeground(), attrC.CalculateRgbForeground());
    VERIFY_ARE_EQUAL(attrD.CalculateRgbBackground(), attrC.CalculateRgbBackground());
    VERIFY_IS_TRUE(attrD.IsBold());

    VERIFY_ARE_EQUAL(attrE.CalculateRgbForeground(), RGB(64,64,64));
    VERIFY_ARE_EQUAL(attrE.CalculateRgbBackground(), RGB(1,2,3));
    VERIFY_IS_TRUE(attrE.IsBold());

    VERIFY_ARE_EQUAL(attrF.CalculateRgbForeground(), RGB(64,64,64));
    VERIFY_ARE_EQUAL(attrF.CalculateRgbBackground(), RGB(1,2,3));
    VERIFY_IS_FALSE(attrF.IsBold());

    std::wstring reset = L"\x1b[0m";
    stateMachine->ProcessString(&reset[0], reset.length());
}


void TextBufferTests::CopyAttrs()
{
    CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    SCREEN_INFORMATION& si = gci.GetActiveOutputBuffer().GetActiveBuffer();
    TextBuffer& tbi = si.GetTextBuffer();
    StateMachine* const stateMachine = si.GetStateMachine();
    Cursor& cursor = tbi.GetCursor();
    SetFlag(si.OutputMode, ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    VERIFY_IS_NOT_NULL(stateMachine);

    cursor.SetXPosition(0);
    cursor.SetYPosition(0);
    // Write '\E[32mX\E[33mX\n\E[34mX\E[35mX\E[H\E[M'
    // The first two X's should get deleted.
    // The third X should be blue
    // The fourth X should be magenta
    std::wstring sequence = L"\x1b[32mX\x1b[33mX\n\x1b[34mX\x1b[35mX\x1b[H\x1b[M";
    stateMachine->ProcessString(&sequence[0], sequence.length());

    const auto x = cursor.GetPosition().X;
    const auto y = cursor.GetPosition().Y;
    const auto dark_blue = gci.GetColorTableEntry(1);
    const auto dark_magenta = gci.GetColorTableEntry(5);

    Log::Comment(NoThrowString().Format(
        L"cursor={X:%d,Y:%d}",
        x, y
    ));
    VERIFY_ARE_EQUAL(x, 0);
    VERIFY_ARE_EQUAL(y, 0);

    const auto& row = tbi.GetRowByOffset(0);
    const auto attrRow = &row.GetAttrRow();
    const auto len = tbi._coordBufferSize.X;
    const std::vector<TextAttribute> attrs{ attrRow->begin(), attrRow->end() };
    const auto attrA = attrs[0];
    const auto attrB = attrs[1];

    Log::Comment(NoThrowString().Format(
        L"cursor={X:%d,Y:%d}",
        x, y
    ));

    LOG_ATTR(attrA);
    LOG_ATTR(attrB);

    VERIFY_ARE_EQUAL(attrA.CalculateRgbForeground(), dark_blue);
    VERIFY_ARE_EQUAL(attrB.CalculateRgbForeground(), dark_magenta);

}

void TextBufferTests::EmptySgrTest()
{
    CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    SCREEN_INFORMATION& si = gci.GetActiveOutputBuffer().GetActiveBuffer();
    TextBuffer& tbi = si.GetTextBuffer();
    StateMachine* const stateMachine = si.GetStateMachine();
    Cursor& cursor = tbi.GetCursor();

    SetFlag(si.OutputMode, ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    VERIFY_IS_NOT_NULL(stateMachine);
    cursor.SetXPosition(0);
    cursor.SetYPosition(0);

    std::wstring reset = L"\x1b[0m";
    stateMachine->ProcessString(&reset[0], reset.length());
    const COLORREF defaultFg = si.GetAttributes().CalculateRgbForeground();
    const COLORREF defaultBg = si.GetAttributes().CalculateRgbBackground();

    // Case 1 -
    //      Write '\x1b[0mX\x1b[31mX\x1b[31;m'
    //      The first X should be default colors.
    //      The second X should be (darkRed,default).
    //      The third X should be default colors.
    std::wstring sequence = L"\x1b[0mX\x1b[31mX\x1b[31;mX";
    stateMachine->ProcessString(&sequence[0], sequence.length());

    const auto x = cursor.GetPosition().X;
    const auto y = cursor.GetPosition().Y;
    const COLORREF darkRed = gci.GetColorTableEntry(4);
    Log::Comment(NoThrowString().Format(
        L"cursor={X:%d,Y:%d}",
        x, y
    ));
    VERIFY_IS_TRUE(x >= 3);

    const auto& row = tbi.GetRowByOffset(y);
    const auto attrRow = &row.GetAttrRow();
    const auto len = tbi._coordBufferSize.X;
    const std::vector<TextAttribute> attrs{ attrRow->begin(), attrRow->end() };
    const auto attrA = attrs[x-3];
    const auto attrB = attrs[x-2];
    const auto attrC = attrs[x-1];

    Log::Comment(NoThrowString().Format(
        L"cursor={X:%d,Y:%d}",
        x, y
    ));

    LOG_ATTR(attrA);
    LOG_ATTR(attrB);
    LOG_ATTR(attrC);

    VERIFY_ARE_EQUAL(attrA.CalculateRgbForeground(), defaultFg);
    VERIFY_ARE_EQUAL(attrA.CalculateRgbBackground(), defaultBg);

    VERIFY_ARE_EQUAL(attrB.CalculateRgbForeground(), darkRed);
    VERIFY_ARE_EQUAL(attrB.CalculateRgbBackground(), defaultBg);

    VERIFY_ARE_EQUAL(attrC.CalculateRgbForeground(), defaultFg);
    VERIFY_ARE_EQUAL(attrC.CalculateRgbBackground(), defaultBg);

    stateMachine->ProcessString(&reset[0], reset.length());
}

void TextBufferTests::TestReverseReset()
{
    CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    SCREEN_INFORMATION& si = gci.GetActiveOutputBuffer().GetActiveBuffer();
    TextBuffer& tbi = si.GetTextBuffer();
    StateMachine* const stateMachine = si.GetStateMachine();
    Cursor& cursor = tbi.GetCursor();

    SetFlag(si.OutputMode, ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    VERIFY_IS_NOT_NULL(stateMachine);

    cursor.SetXPosition(0);
    cursor.SetYPosition(0);

    std::wstring reset = L"\x1b[0m";
    stateMachine->ProcessString(&reset[0], reset.length());
    const COLORREF defaultFg = si.GetAttributes().CalculateRgbForeground();
    const COLORREF defaultBg = si.GetAttributes().CalculateRgbBackground();

    // Case 1 -
    //      Write '\E[42m\E[38;2;128;5;255mX\E[7mX\E[27mX'
    //      The first X should be (fg,bg) = (rgb(128;5;255), dark_green)
    //      The second X should be (fg,bg) = (dark_green, rgb(128;5;255))
    //      The third X should be (fg,bg) = (rgb(128;5;255), dark_green)
    std::wstring sequence = L"\x1b[42m\x1b[38;2;128;5;255mX\x1b[7mX\x1b[27mX";
    stateMachine->ProcessString(&sequence[0], sequence.length());

    const auto x = cursor.GetPosition().X;
    const auto y = cursor.GetPosition().Y;
    const auto dark_green = gci.GetColorTableEntry(2);
    const COLORREF rgbColor = RGB(128, 5, 255);

    Log::Comment(NoThrowString().Format(
        L"cursor={X:%d,Y:%d}",
        x, y
    ));
    VERIFY_IS_TRUE(x >= 3);

    const auto& row = tbi.GetRowByOffset(y);
    const auto attrRow = &row.GetAttrRow();
    const auto len = tbi._coordBufferSize.X;
    const std::vector<TextAttribute> attrs{ attrRow->begin(), attrRow->end() };
    const auto attrA = attrs[x-3];
    const auto attrB = attrs[x-2];
    const auto attrC = attrs[x-1];

    Log::Comment(NoThrowString().Format(
        L"cursor={X:%d,Y:%d}",
        x, y
    ));

    LOG_ATTR(attrA);
    LOG_ATTR(attrB);
    LOG_ATTR(attrC);

    VERIFY_ARE_EQUAL(attrA.CalculateRgbForeground(), rgbColor);
    VERIFY_ARE_EQUAL(attrA.CalculateRgbBackground(), dark_green);

    VERIFY_ARE_EQUAL(attrB.CalculateRgbForeground(), dark_green);
    VERIFY_ARE_EQUAL(attrB.CalculateRgbBackground(), rgbColor);

    VERIFY_ARE_EQUAL(attrC.CalculateRgbForeground(), rgbColor);
    VERIFY_ARE_EQUAL(attrC.CalculateRgbBackground(), dark_green);

    stateMachine->ProcessString(&reset[0], reset.length());
}

void LogTextAttribute(const TextAttribute& attr, const std::wstring& name)
{
    Log::Comment(NoThrowString().Format(
        L"%s={IsLegacy:%d, GetLegacyAttributes:0x%x, FG:0x%x, BG:0x%x}",
        name.c_str(),
        attr.IsLegacy(), attr.GetLegacyAttributes(), attr.CalculateRgbForeground(), attr.CalculateRgbBackground()
    ));
}

void TextBufferTests::CopyLastAttr()
{
    DisableVerifyExceptions disable;

    CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    SCREEN_INFORMATION& si = gci.GetActiveOutputBuffer().GetActiveBuffer();
    TextBuffer& tbi = si.GetTextBuffer();
    StateMachine* const stateMachine = si.GetStateMachine();
    Cursor& cursor = tbi.GetCursor();

    SetFlag(si.OutputMode, ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    VERIFY_IS_NOT_NULL(stateMachine);

    cursor.SetXPosition(0);
    cursor.SetYPosition(0);

    std::wstring reset = L"\x1b[0m";
    stateMachine->ProcessString(&reset[0], reset.length());
    const COLORREF defaultFg = si.GetAttributes().CalculateRgbForeground();
    const COLORREF defaultBg = si.GetAttributes().CalculateRgbBackground();

    const COLORREF solFg = RGB(101, 123, 131);
    const COLORREF solBg = RGB(0, 43, 54);
    const COLORREF solCyan = RGB(42, 161, 152);

    std::wstring solFgSeq = L"\x1b[38;2;101;123;131m";
    std::wstring solBgSeq = L"\x1b[48;2;0;43;54m";
    std::wstring solCyanSeq = L"\x1b[38;2;42;161;152m";

    // Make sure that the color table has certain values we expect
    const COLORREF defaultBrightBlack = RGB(118, 118, 118);
    const COLORREF defaultBrightYellow = RGB(249, 241, 165);
    const COLORREF defaultBrightCyan = RGB(97, 214, 214);

    gci.SetColorTableEntry(8, defaultBrightBlack);
    gci.SetColorTableEntry(14, defaultBrightYellow);
    gci.SetColorTableEntry(11, defaultBrightCyan);

    // Write (solFg, solBG) X \n
    //       (solFg, solBG) X (solCyan, solBG) X \n
    //       (solFg, solBG) X (solCyan, solBG) X (solFg, solBG) X
    // then go home, and insert a line.

    // Row 1
    stateMachine->ProcessString(&solFgSeq[0], solFgSeq.length());
    stateMachine->ProcessString(&solBgSeq[0], solBgSeq.length());
    stateMachine->ProcessString(L"X", 1);
    stateMachine->ProcessString(L"\n", 1);

    // Row 2
    // Remember that the colors from before persist here too, so we don't need
    //      to emit both the FG and BG if they haven't changed.
    stateMachine->ProcessString(L"X", 1);
    stateMachine->ProcessString(&solCyanSeq[0], solCyanSeq.length());
    stateMachine->ProcessString(L"X", 1);
    stateMachine->ProcessString(L"\n", 1);

    // Row 3
    stateMachine->ProcessString(&solFgSeq[0], solFgSeq.length());
    stateMachine->ProcessString(&solBgSeq[0], solBgSeq.length());
    stateMachine->ProcessString(L"X", 1);
    stateMachine->ProcessString(&solCyanSeq[0], solCyanSeq.length());
    stateMachine->ProcessString(L"X", 1);
    stateMachine->ProcessString(&solFgSeq[0], solFgSeq.length());
    stateMachine->ProcessString(L"X", 1);

    std::wstring insertLineAtHome = L"\x1b[H\x1b[L";
    stateMachine->ProcessString(&insertLineAtHome[0], insertLineAtHome.length());

    const auto x = cursor.GetPosition().X;
    const auto y = cursor.GetPosition().Y;

    Log::Comment(NoThrowString().Format(
        L"cursor={X:%d,Y:%d}",
        x, y
    ));

    const ROW& row1 = tbi.GetRowByOffset(y + 1);
    const ROW& row2 = tbi.GetRowByOffset(y + 2);
    const ROW& row3 = tbi.GetRowByOffset(y + 3);
    const auto len = tbi._coordBufferSize.X;

    const std::vector<TextAttribute> attrs1{ row1.GetAttrRow().begin(), row1.GetAttrRow().end() };
    const std::vector<TextAttribute> attrs2{ row2.GetAttrRow().begin(), row2.GetAttrRow().end() };
    const std::vector<TextAttribute> attrs3{ row3.GetAttrRow().begin(), row3.GetAttrRow().end() };

    const auto attr1A = attrs1[0];

    const auto attr2A = attrs2[0];
    const auto attr2B = attrs2[1];

    const auto attr3A = attrs3[0];
    const auto attr3B = attrs3[1];
    const auto attr3C = attrs3[2];

    Log::Comment(NoThrowString().Format(
        L"cursor={X:%d,Y:%d}",
        x, y
    ));

    LOG_ATTR(attr1A);
    LOG_ATTR(attr2A);
    LOG_ATTR(attr2A);
    LOG_ATTR(attr3A);
    LOG_ATTR(attr3B);
    LOG_ATTR(attr3C);

    VERIFY_ARE_EQUAL(attr1A.CalculateRgbForeground(), solFg);
    VERIFY_ARE_EQUAL(attr1A.CalculateRgbBackground(), solBg);


    VERIFY_ARE_EQUAL(attr2A.CalculateRgbForeground(), solFg);
    VERIFY_ARE_EQUAL(attr2A.CalculateRgbBackground(), solBg);

    VERIFY_ARE_EQUAL(attr2B.CalculateRgbForeground(), solCyan);
    VERIFY_ARE_EQUAL(attr2B.CalculateRgbBackground(), solBg);


    VERIFY_ARE_EQUAL(attr3A.CalculateRgbForeground(), solFg);
    VERIFY_ARE_EQUAL(attr3A.CalculateRgbBackground(), solBg);

    VERIFY_ARE_EQUAL(attr3B.CalculateRgbForeground(), solCyan);
    VERIFY_ARE_EQUAL(attr3B.CalculateRgbBackground(), solBg);

    VERIFY_ARE_EQUAL(attr3C.CalculateRgbForeground(), solFg);
    VERIFY_ARE_EQUAL(attr3C.CalculateRgbBackground(), solBg);

    stateMachine->ProcessString(&reset[0], reset.length());
}

void TextBufferTests::TestTextAttributeColorGetters()
{
    const COLORREF red = RGB(255, 0, 0);
    const COLORREF green = RGB(0, 255, 0);
    TextAttribute textAttribute(red, green);

    // verify that calculated foreground/background are the same as the direct values when reverse video is
    // not set
    VERIFY_IS_FALSE(textAttribute._IsReverseVideo());

    VERIFY_ARE_EQUAL(red, textAttribute.GetRgbForeground());
    VERIFY_ARE_EQUAL(red, textAttribute.CalculateRgbForeground());

    VERIFY_ARE_EQUAL(green, textAttribute.GetRgbBackground());
    VERIFY_ARE_EQUAL(green, textAttribute.CalculateRgbBackground());

    // with reverse video set, calucated foreground/background values should be switched while getters stay
    // the same
    textAttribute.SetMetaAttributes(COMMON_LVB_REVERSE_VIDEO);

    VERIFY_ARE_EQUAL(red, textAttribute.GetRgbForeground());
    VERIFY_ARE_EQUAL(green, textAttribute.CalculateRgbForeground());

    VERIFY_ARE_EQUAL(green, textAttribute.GetRgbBackground());
    VERIFY_ARE_EQUAL(red, textAttribute.CalculateRgbBackground());
}

void TextBufferTests::TestRgbThenBold()
{
    CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    SCREEN_INFORMATION& si = gci.GetActiveOutputBuffer().GetActiveBuffer();
    const TextBuffer& tbi = si.GetTextBuffer();
    StateMachine* const stateMachine = si.GetStateMachine();
    const Cursor& cursor = tbi.GetCursor();
    VERIFY_IS_NOT_NULL(stateMachine);
    // See MSFT:16398982
    Log::Comment(NoThrowString().Format(
        L"Test that a bold following a RGB color doesn't remove the RGB color"
    ));
    Log::Comment(L"\"\\x1b[38;2;40;40;40m\\x1b[48;2;168;153;132mX\\x1b[1mX\\x1b[m\"");
    const auto foreground = RGB(40, 40, 40);
    const auto background = RGB(168, 153, 132);

    const wchar_t* const sequence = L"\x1b[38;2;40;40;40m\x1b[48;2;168;153;132mX\x1b[1mX\x1b[m";
    stateMachine->ProcessString(sequence, std::wcslen(sequence));
    const auto x = cursor.GetPosition().X;
    const auto y = cursor.GetPosition().Y;
    const auto& row = tbi.GetRowByOffset(y);
    const auto attrRow = &row.GetAttrRow();
    const std::vector<TextAttribute> attrs{ attrRow->begin(), attrRow->end() };
    const auto attrA = attrs[x-2];
    const auto attrB = attrs[x-1];
    Log::Comment(NoThrowString().Format(
        L"cursor={X:%d,Y:%d}",
        x, y
    ));
    Log::Comment(NoThrowString().Format(
        L"attrA should be RGB, and attrB should be the same as attrA, NOT bolded"
    ));

    LOG_ATTR(attrA);
    LOG_ATTR(attrB);

    VERIFY_ARE_EQUAL(attrA.IsLegacy(), false);
    VERIFY_ARE_EQUAL(attrB.IsLegacy(), false);

    VERIFY_ARE_EQUAL(attrA.CalculateRgbForeground(), foreground);
    VERIFY_ARE_EQUAL(attrA.CalculateRgbBackground(), background);
    VERIFY_ARE_EQUAL(attrB.CalculateRgbForeground(), foreground);
    VERIFY_ARE_EQUAL(attrB.CalculateRgbBackground(), background);

    wchar_t* reset = L"\x1b[0m";
    stateMachine->ProcessString(reset, std::wcslen(reset));
}

void TextBufferTests::TestResetClearsBoldness()
{
    CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    SCREEN_INFORMATION& si = gci.GetActiveOutputBuffer().GetActiveBuffer();
    const TextBuffer& tbi = si.GetTextBuffer();
    StateMachine* const stateMachine = si.GetStateMachine();
    const Cursor& cursor = tbi.GetCursor();
    VERIFY_IS_NOT_NULL(stateMachine);
    Log::Comment(NoThrowString().Format(
        L"Test that resetting bold attributes clears the boldness."
    ));
    const auto x0 = cursor.GetPosition().X;
    const COLORREF defaultFg = si.GetAttributes().CalculateRgbForeground();
    const COLORREF defaultBg = si.GetAttributes().CalculateRgbBackground();
    const auto dark_green = gci.GetColorTableEntry(2);
    const auto bright_green = gci.GetColorTableEntry(10);

    wchar_t* sequence = L"\x1b[32mA\x1b[1mB\x1b[0mC\x1b[32mD";
    Log::Comment(NoThrowString().Format(sequence));
    stateMachine->ProcessString(sequence, std::wcslen(sequence));

    const auto x = cursor.GetPosition().X;
    const auto y = cursor.GetPosition().Y;
    const auto& row = tbi.GetRowByOffset(y);
    const auto attrRow = &row.GetAttrRow();
    const std::vector<TextAttribute> attrs{ attrRow->begin(), attrRow->end() };
    const auto attrA = attrs[x0];
    const auto attrB = attrs[x0+1];
    const auto attrC = attrs[x0+2];
    const auto attrD = attrs[x0+3];
    Log::Comment(NoThrowString().Format(
        L"cursor={X:%d,Y:%d}",
        x, y
    ));
    Log::Comment(NoThrowString().Format(
        L"attrA should be RGB, and attrB should be the same as attrA, NOT bolded"
    ));

    LOG_ATTR(attrA);
    LOG_ATTR(attrB);
    LOG_ATTR(attrC);
    LOG_ATTR(attrD);

    VERIFY_ARE_EQUAL(attrA.CalculateRgbForeground(), dark_green);
    VERIFY_ARE_EQUAL(attrB.CalculateRgbForeground(), bright_green);
    VERIFY_ARE_EQUAL(attrC.CalculateRgbForeground(), defaultFg);
    VERIFY_ARE_EQUAL(attrD.CalculateRgbForeground(), dark_green);

    VERIFY_IS_FALSE(attrA.IsBold());
    VERIFY_IS_TRUE(attrB.IsBold());
    VERIFY_IS_FALSE(attrC.IsBold());
    VERIFY_IS_FALSE(attrD.IsBold());

    wchar_t* reset = L"\x1b[0m";
    stateMachine->ProcessString(reset, std::wcslen(reset));
}
