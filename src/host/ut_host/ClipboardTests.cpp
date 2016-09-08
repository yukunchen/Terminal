/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/
#include "precomp.h"
#include "WexTestClass.h"

#include "CommonState.hpp"

#include "globals.h"
#include "clipboard.hpp"

using namespace WEX::Common;
using namespace WEX::Logging;
using namespace WEX::TestExecution;

#include "UnicodeLiteral.hpp"

class ClipboardTests
{
    TEST_CLASS(ClipboardTests);

    CommonState* m_state;

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

        return true;
    }

    TEST_METHOD_SETUP(MethodSetup)
    {
        m_state->FillTextBuffer();
        return true;
    }

    TEST_METHOD_CLEANUP(MethodCleanup)
    {
        return true;
    }

    const UINT cRectsSelected = 4;

    void SetupRetrieveFromBuffers(bool fLineSelection, SMALL_RECT** prgsrSelection, PWCHAR** prgTempRows, size_t** prgTempRowLengths)
    {
        // NOTE: This test requires innate knowledge of how the common buffer text is emitted in order to test all cases
        // Please see CommonState.cpp for information on the buffer state per row, the row contents, etc.

        // set up and try to retrieve the first 4 rows from the buffer
        SCREEN_INFORMATION* pScreenInfo = g_ciConsoleInformation.CurrentScreenBuffer;

        SMALL_RECT* rgsrSelection = new SMALL_RECT[cRectsSelected];

        rgsrSelection[0].Top = rgsrSelection[0].Bottom = 0;
        rgsrSelection[0].Left = 0;
        rgsrSelection[0].Right = 8;
        rgsrSelection[1].Top = rgsrSelection[1].Bottom = 1;
        rgsrSelection[1].Left = 0;
        rgsrSelection[1].Right = 14;
        rgsrSelection[2].Top = rgsrSelection[2].Bottom = 2;
        rgsrSelection[2].Left = 0;
        rgsrSelection[2].Right = 14;
        rgsrSelection[3].Top = rgsrSelection[3].Bottom = 3;
        rgsrSelection[3].Left = 0;
        rgsrSelection[3].Right = 8;

        PWCHAR* rgTempRows = new PWCHAR[cRectsSelected];
        size_t* rgTempRowLengths = new size_t[cRectsSelected];

        NTSTATUS status = Clipboard::s_RetrieveTextFromBuffer(pScreenInfo,
                                                              fLineSelection,
                                                              cRectsSelected,
                                                              rgsrSelection,
                                                              rgTempRows,
                                                              rgTempRowLengths);

        // function successful
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        // verify text lengths match the rows

        VERIFY_ARE_EQUAL(wcslen(rgTempRows[0]), rgTempRowLengths[0]);
        VERIFY_ARE_EQUAL(wcslen(rgTempRows[1]), rgTempRowLengths[1]);
        VERIFY_ARE_EQUAL(wcslen(rgTempRows[2]), rgTempRowLengths[2]);
        VERIFY_ARE_EQUAL(wcslen(rgTempRows[3]), rgTempRowLengths[3]);

        *prgsrSelection = rgsrSelection;
        *prgTempRows = rgTempRows;
        *prgTempRowLengths = rgTempRowLengths;
    }

    void CleanupRetrieveFromBuffers(SMALL_RECT** prgsrSelection, PWCHAR** prgTempRows, size_t** prgTempRowLengths)
    {
        if (*prgsrSelection != nullptr)
        {
            delete[](*prgsrSelection);
            *prgsrSelection = nullptr;
        }

        if (*prgTempRows != nullptr)
        {
            PWCHAR* rgTempRows = *prgTempRows;

            for (UINT iRow = 0; iRow < cRectsSelected; iRow++)
            {
                PWCHAR pwszRowData = rgTempRows[iRow];
                if (pwszRowData != nullptr)
                {
                    delete[] pwszRowData;
                    rgTempRows[iRow] = nullptr;
                }
            }

            delete[] rgTempRows;
            *prgTempRows = nullptr;
        }

        if (*prgTempRowLengths != nullptr)
        {
            delete[](*prgTempRowLengths);
            *prgTempRowLengths = nullptr;
        }
    }

#pragma prefast(push)
#pragma prefast(disable:26006, "Specifically trying to check unterminated strings in this test.")
    TEST_METHOD(TestRetrieveFromBuffer)
    {
        // NOTE: This test requires innate knowledge of how the common buffer text is emitted in order to test all cases
        // Please see CommonState.cpp for information on the buffer state per row, the row contents, etc.

        SMALL_RECT* rgsrSelection = nullptr;
        PWCHAR* rgTempRows = nullptr;
        size_t* rgTempRowLengths = nullptr;

        SetupRetrieveFromBuffers(false, &rgsrSelection, &rgTempRows, &rgTempRowLengths);

        // verify trailing bytes were trimmed
        // there are 2 double-byte characters in our sample string (see CommonState.cpp for sample)
        // the width is right - left
        VERIFY_ARE_EQUAL((short)wcslen(rgTempRows[0]), rgsrSelection[0].Right - rgsrSelection[0].Left);

        // since we're not in line selection, the line should be \r\n terminated
        PWCHAR tempPtr = rgTempRows[0];
        tempPtr += rgTempRowLengths[0];
        tempPtr -= 2;
        VERIFY_ARE_EQUAL(String(tempPtr), String(L"\r\n"));

        // since we're not in line selection, spaces should be trimmed from the end
        tempPtr = rgTempRows[0];
        tempPtr += rgsrSelection[0].Right - rgsrSelection[0].Left - 2;
        tempPtr++;
        VERIFY_IS_NULL(wcsrchr(tempPtr, L' '));

        // final line of selection should not contain CR/LF
        tempPtr = rgTempRows[3];
        tempPtr += rgTempRowLengths[3];
        tempPtr -= 2;
        VERIFY_ARE_NOT_EQUAL(String(tempPtr), String(L"\r\n"));

        CleanupRetrieveFromBuffers(&rgsrSelection, &rgTempRows, &rgTempRowLengths);
    }
#pragma prefast(pop)

    TEST_METHOD(TestRetrieveLineSelectionFromBuffer)
    {
        // NOTE: This test requires innate knowledge of how the common buffer text is emitted in order to test all cases
        // Please see CommonState.cpp for information on the buffer state per row, the row contents, etc.

        SMALL_RECT* rgsrSelection = nullptr;
        PWCHAR* rgTempRows = nullptr;
        size_t* rgTempRowLengths = nullptr;

        SetupRetrieveFromBuffers(true, &rgsrSelection, &rgTempRows, &rgTempRowLengths);

        // row 2, no wrap
        // no wrap row before the end should have CR/LF
        PWCHAR tempPtr = rgTempRows[2];
        tempPtr += rgTempRowLengths[2];
        tempPtr -= 2;
        VERIFY_ARE_EQUAL(String(tempPtr), String(L"\r\n"));

        // no wrap row should trim spaces at the end
        tempPtr = rgTempRows[2];
        VERIFY_IS_NULL(wcsrchr(tempPtr, L' '));

        // row 1, wrap
        // wrap row before the end should *not* have CR/LF
        tempPtr = rgTempRows[1];
        tempPtr += rgTempRowLengths[1];
        tempPtr -= 2;
        VERIFY_ARE_NOT_EQUAL(String(tempPtr), String(L"\r\n"));

        // wrap row should have spaces at the end
        tempPtr = rgTempRows[1];
        wchar_t* ptr = wcsrchr(tempPtr, L' ');
        VERIFY_IS_NOT_NULL(ptr);

        CleanupRetrieveFromBuffers(&rgsrSelection, &rgTempRows, &rgTempRowLengths);
    }
};
