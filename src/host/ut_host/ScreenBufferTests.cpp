/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/
#include "precomp.h"
#include "WexTestClass.h"
#include "..\..\inc\consoletaeftemplates.hpp"

#include "CommonState.hpp"

#include "globals.h"
#include "screenInfo.hpp"

#include "input.h"
#include "getset.h"

#include "..\interactivity\inc\ServiceLocator.hpp"
#include "..\..\inc\conattrs.hpp"

using namespace WEX::Common;
using namespace WEX::Logging;
using namespace WEX::TestExecution;

#define VERIFY_SUCCESS_NTSTATUS(x) VERIFY_IS_TRUE(SUCCEEDED_NTSTATUS(x))

class ScreenBufferTests
{
    CommonState* m_state;

    TEST_CLASS(ScreenBufferTests);

    TEST_CLASS_SETUP(ClassSetup)
    {
        m_state = new CommonState();

        m_state->PrepareGlobalFont();
        m_state->PrepareGlobalScreenBuffer();
        m_state->PrepareGlobalInputBuffer();

        return true;
    }

    TEST_CLASS_CLEANUP(ClassCleanup)
    {
        m_state->CleanupGlobalScreenBuffer();
        m_state->CleanupGlobalFont();
        m_state->CleanupGlobalInputBuffer();

        delete m_state;

        return true;
    }

    TEST_METHOD_SETUP(MethodSetup)
    {
        const CONSOLE_INFORMATION* const gci = ServiceLocator::LocateGlobals()->getConsoleInformation();
        m_state->PrepareNewTextBufferInfo();
        gci->CurrentScreenBuffer->SetViewportOrigin(true, {0,0});

        return true;
    }

    TEST_METHOD_CLEANUP(MethodCleanup)
    {
        m_state->CleanupNewTextBufferInfo();

        return true;
    }

    TEST_METHOD(SingleAlternateBufferCreationTest);

    TEST_METHOD(MultipleAlternateBufferCreationTest);

    TEST_METHOD(MultipleAlternateBuffersFromMainCreationTest);

    TEST_METHOD(TestReverseLineFeed);

    size_t const cSampleListTabs = 5;
    SHORT rgSampleListValues[5] = { 2, 3, 7, 7, 14 };

    SCREEN_INFORMATION::TabStop** CreateSampleList();

    void FreeSampleList(SCREEN_INFORMATION::TabStop** rgList);

    TEST_METHOD(TestAddTabStop);

    TEST_METHOD(TestClearTabStops);

    TEST_METHOD(TestClearTabStop);

    TEST_METHOD(TestGetForwardTab);

    TEST_METHOD(TestGetReverseTab);

    TEST_METHOD(TestAreTabsSet);

    TEST_METHOD(EraseAllTests);

    TEST_METHOD(VtResize);
    
    TEST_METHOD(VtSoftResetCursorPosition);
    
    TEST_METHOD(VtSetColorTable);

    TEST_METHOD(ResizeTraditionalDoesntDoubleFreeAttrRows);
};

SCREEN_INFORMATION::TabStop** ScreenBufferTests::CreateSampleList()
{
    SCREEN_INFORMATION::TabStop** rgpTabs = new SCREEN_INFORMATION::TabStop*[cSampleListTabs];

    // create tab stop items and fill with values
    for (size_t i = 0; i < cSampleListTabs; i++)
    {
        rgpTabs[i] = new SCREEN_INFORMATION::TabStop();
        rgpTabs[i]->sColumn = rgSampleListValues[i];
    }

    // link up the list
    for (size_t i = 0; i < cSampleListTabs - 1; i++)
    {
        rgpTabs[i]->ptsNext = rgpTabs[i + 1];
    }

    return rgpTabs;
}

void ScreenBufferTests::FreeSampleList(SCREEN_INFORMATION::TabStop** rgList)
{
    for (size_t i = 0; i < cSampleListTabs; i++)
    {
        if (rgList[i] != nullptr)
        {
            delete rgList[i];
        }
    }

    delete[] rgList;
}


void ScreenBufferTests::SingleAlternateBufferCreationTest()
{
    const CONSOLE_INFORMATION* const gci = ServiceLocator::LocateGlobals()->getConsoleInformation();
    Log::Comment(L"Testing creating one alternate buffer, then returning to the main buffer.");
    SCREEN_INFORMATION* const psiOriginal = gci->CurrentScreenBuffer;
    VERIFY_IS_NULL(psiOriginal->_psiAlternateBuffer);
    VERIFY_IS_NULL(psiOriginal->_psiMainBuffer);

    NTSTATUS Status = psiOriginal->UseAlternateScreenBuffer();
    if(VERIFY_IS_TRUE(NT_SUCCESS(Status)))
    {
        Log::Comment(L"First alternate buffer successfully created");
        SCREEN_INFORMATION* psiFirstAlternate = gci->CurrentScreenBuffer;
        VERIFY_ARE_NOT_EQUAL(psiOriginal, psiFirstAlternate);
        VERIFY_ARE_EQUAL(psiFirstAlternate, psiOriginal->_psiAlternateBuffer);
        VERIFY_ARE_EQUAL(psiOriginal, psiFirstAlternate->_psiMainBuffer);
        VERIFY_IS_NULL(psiOriginal->_psiMainBuffer);
        VERIFY_IS_NULL(psiFirstAlternate->_psiAlternateBuffer);

        Status = psiFirstAlternate->UseMainScreenBuffer();
        if(VERIFY_IS_TRUE(NT_SUCCESS(Status)))
        {
            Log::Comment(L"successfully swapped to the main buffer");
            SCREEN_INFORMATION* psiFinal = gci->CurrentScreenBuffer;
            VERIFY_ARE_NOT_EQUAL(psiFinal, psiFirstAlternate);
            VERIFY_ARE_EQUAL(psiFinal, psiOriginal);
            VERIFY_IS_NULL(psiFinal->_psiMainBuffer);
            VERIFY_IS_NULL(psiFinal->_psiAlternateBuffer);
        }
    }
}

void ScreenBufferTests::MultipleAlternateBufferCreationTest()
{
    const CONSOLE_INFORMATION* const gci = ServiceLocator::LocateGlobals()->getConsoleInformation();
    Log::Comment(L"Testing creating one alternate buffer, then creating another alternate from that first alternate, before returning to the main buffer.");
    SCREEN_INFORMATION* const psiOriginal = gci->CurrentScreenBuffer;
    NTSTATUS Status = psiOriginal->UseAlternateScreenBuffer();
    if(VERIFY_IS_TRUE(NT_SUCCESS(Status)))
    {
        Log::Comment(L"First alternate buffer successfully created");
        SCREEN_INFORMATION* psiFirstAlternate = gci->CurrentScreenBuffer;
        VERIFY_ARE_NOT_EQUAL(psiOriginal, psiFirstAlternate);
        VERIFY_ARE_EQUAL(psiFirstAlternate, psiOriginal->_psiAlternateBuffer);
        VERIFY_ARE_EQUAL(psiOriginal, psiFirstAlternate->_psiMainBuffer);
        VERIFY_IS_NULL(psiOriginal->_psiMainBuffer);
        VERIFY_IS_NULL(psiFirstAlternate->_psiAlternateBuffer);

        Status = psiFirstAlternate->UseAlternateScreenBuffer();
        if(VERIFY_IS_TRUE(NT_SUCCESS(Status)))
        {
            Log::Comment(L"Second alternate buffer successfully created");
            SCREEN_INFORMATION* psiSecondAlternate = gci->CurrentScreenBuffer;
            VERIFY_ARE_NOT_EQUAL(psiOriginal, psiSecondAlternate);
            VERIFY_ARE_NOT_EQUAL(psiSecondAlternate, psiFirstAlternate);
            VERIFY_ARE_EQUAL(psiSecondAlternate, psiOriginal->_psiAlternateBuffer);
            VERIFY_ARE_EQUAL(psiOriginal, psiSecondAlternate->_psiMainBuffer);
            VERIFY_IS_NULL(psiOriginal->_psiMainBuffer);
            VERIFY_IS_NULL(psiSecondAlternate->_psiAlternateBuffer);

            Status = psiSecondAlternate->UseMainScreenBuffer();
            if(VERIFY_IS_TRUE(NT_SUCCESS(Status)))
            {
                Log::Comment(L"successfully swapped to the main buffer");
                SCREEN_INFORMATION* psiFinal = gci->CurrentScreenBuffer;
                VERIFY_ARE_NOT_EQUAL(psiFinal, psiFirstAlternate);
                VERIFY_ARE_NOT_EQUAL(psiFinal, psiSecondAlternate);
                VERIFY_ARE_EQUAL(psiFinal, psiOriginal);
                VERIFY_IS_NULL(psiFinal->_psiMainBuffer);
                VERIFY_IS_NULL(psiFinal->_psiAlternateBuffer);
            }
        }
    }
}

void ScreenBufferTests::MultipleAlternateBuffersFromMainCreationTest()
{
    const CONSOLE_INFORMATION* const gci = ServiceLocator::LocateGlobals()->getConsoleInformation();
    Log::Comment(L"Testing creating one alternate buffer, then creating another alternate from the main, before returning to the main buffer.");
    SCREEN_INFORMATION* const psiOriginal = gci->CurrentScreenBuffer;
    NTSTATUS Status = psiOriginal->UseAlternateScreenBuffer();
    if(VERIFY_IS_TRUE(NT_SUCCESS(Status)))
    {
        Log::Comment(L"First alternate buffer successfully created");
        SCREEN_INFORMATION* psiFirstAlternate = gci->CurrentScreenBuffer;
        VERIFY_ARE_NOT_EQUAL(psiOriginal, psiFirstAlternate);
        VERIFY_ARE_EQUAL(psiFirstAlternate, psiOriginal->_psiAlternateBuffer);
        VERIFY_ARE_EQUAL(psiOriginal, psiFirstAlternate->_psiMainBuffer);
        VERIFY_IS_NULL(psiOriginal->_psiMainBuffer);
        VERIFY_IS_NULL(psiFirstAlternate->_psiAlternateBuffer);

        Status = psiOriginal->UseAlternateScreenBuffer();
        if(VERIFY_IS_TRUE(NT_SUCCESS(Status)))
        {
            Log::Comment(L"Second alternate buffer successfully created");
            SCREEN_INFORMATION* psiSecondAlternate = gci->CurrentScreenBuffer;
            VERIFY_ARE_NOT_EQUAL(psiOriginal, psiSecondAlternate);
            VERIFY_ARE_NOT_EQUAL(psiSecondAlternate, psiFirstAlternate);
            VERIFY_ARE_EQUAL(psiSecondAlternate, psiOriginal->_psiAlternateBuffer);
            VERIFY_ARE_EQUAL(psiOriginal, psiSecondAlternate->_psiMainBuffer);
            VERIFY_IS_NULL(psiOriginal->_psiMainBuffer);
            VERIFY_IS_NULL(psiSecondAlternate->_psiAlternateBuffer);

            Status = psiSecondAlternate->UseMainScreenBuffer();
            if(VERIFY_IS_TRUE(NT_SUCCESS(Status)))
            {
                Log::Comment(L"successfully swapped to the main buffer");
                SCREEN_INFORMATION* psiFinal = gci->CurrentScreenBuffer;
                VERIFY_ARE_NOT_EQUAL(psiFinal, psiFirstAlternate);
                VERIFY_ARE_NOT_EQUAL(psiFinal, psiSecondAlternate);
                VERIFY_ARE_EQUAL(psiFinal, psiOriginal);
                VERIFY_IS_NULL(psiFinal->_psiMainBuffer);
                VERIFY_IS_NULL(psiFinal->_psiAlternateBuffer);
            }
        }
    }
}

void ScreenBufferTests::TestReverseLineFeed()
{
    const CONSOLE_INFORMATION* const gci = ServiceLocator::LocateGlobals()->getConsoleInformation();
    SCREEN_INFORMATION* const psi = gci->CurrentScreenBuffer;
    auto bufferWriter = psi->GetBufferWriter();
    auto cursor = psi->TextInfo->GetCursor();
    auto viewport = psi->GetBufferViewport();
    VERIFY_IS_NOT_NULL(bufferWriter);
    VERIFY_IS_NOT_NULL(cursor);

    VERIFY_ARE_EQUAL(psi->GetBufferViewport().Top, 0);

    ////////////////////////////////////////////////////////////////////////
    Log::Comment(L"Case 1: RI from below top of viewport");

    bufferWriter->PrintString(L"foo\nfoo", 7);
    VERIFY_ARE_EQUAL(cursor->GetPosition().X, 3);
    VERIFY_ARE_EQUAL(cursor->GetPosition().Y, 1);
    VERIFY_ARE_EQUAL(psi->GetBufferViewport().Top, 0);

    DoSrvPrivateReverseLineFeed(psi);

    VERIFY_ARE_EQUAL(cursor->GetPosition().X, 3);
    VERIFY_ARE_EQUAL(cursor->GetPosition().Y, 0);
    viewport = psi->GetBufferViewport();
    VERIFY_ARE_EQUAL(viewport.Top, 0);
    Log::Comment(NoThrowString().Format(
        L"viewport={L:%d,T:%d,R:%d,B:%d}",
        viewport.Left, viewport.Top, viewport.Right, viewport.Bottom
    ));

    ////////////////////////////////////////////////////////////////////////
    Log::Comment(L"Case 2: RI from top of viewport");
    cursor->SetPosition({0, 0});
    bufferWriter->PrintString(L"123456789", 9);
    VERIFY_ARE_EQUAL(cursor->GetPosition().X, 9);
    VERIFY_ARE_EQUAL(cursor->GetPosition().Y, 0);
    VERIFY_ARE_EQUAL(psi->GetBufferViewport().Top, 0);

    DoSrvPrivateReverseLineFeed(psi);

    VERIFY_ARE_EQUAL(cursor->GetPosition().X, 9);
    VERIFY_ARE_EQUAL(cursor->GetPosition().Y, 0);
    viewport = psi->GetBufferViewport();
    VERIFY_ARE_EQUAL(viewport.Top, 0);
    Log::Comment(NoThrowString().Format(
        L"viewport={L:%d,T:%d,R:%d,B:%d}",
        viewport.Left, viewport.Top, viewport.Right, viewport.Bottom
    ));
    auto c = psi->TextInfo->GetLastNonSpaceCharacter();
    VERIFY_ARE_EQUAL(c.Y, 2); // This is the coordinates of the second "foo" from before.

    ////////////////////////////////////////////////////////////////////////
    Log::Comment(L"Case 3: RI from top of viewport, when viewport is below top of buffer");

    cursor->SetPosition({0, 5});
    psi->SetViewportOrigin(TRUE, {0, 5});
    bufferWriter->PrintString(L"ABCDEFGH", 9);
    VERIFY_ARE_EQUAL(cursor->GetPosition().X, 9);
    VERIFY_ARE_EQUAL(cursor->GetPosition().Y, 5);
    VERIFY_ARE_EQUAL(psi->GetBufferViewport().Top, 5);

    DoSrvPrivateReverseLineFeed(psi);

    VERIFY_ARE_EQUAL(cursor->GetPosition().X, 9);
    VERIFY_ARE_EQUAL(cursor->GetPosition().Y, 5);
    viewport = psi->GetBufferViewport();
    VERIFY_ARE_EQUAL(viewport.Top, 5);
    Log::Comment(NoThrowString().Format(
        L"viewport={L:%d,T:%d,R:%d,B:%d}",
        viewport.Left, viewport.Top, viewport.Right, viewport.Bottom
    ));
    c = psi->TextInfo->GetLastNonSpaceCharacter();
    VERIFY_ARE_EQUAL(c.Y, 6);
}

void ScreenBufferTests::TestAddTabStop()
{
    const CONSOLE_INFORMATION* const gci = ServiceLocator::LocateGlobals()->getConsoleInformation();
    SCREEN_INFORMATION* const psi = gci->CurrentScreenBuffer;
    psi->_ptsTabs = nullptr;

    Log::Comment(L"Add tab to empty list.");
    VERIFY_SUCCEEDED(HRESULT_FROM_NT(psi->AddTabStop(12)));
    VERIFY_IS_NOT_NULL(psi->_ptsTabs);

    Log::Comment(L"Add tab to head of existing list.");
    VERIFY_SUCCEEDED(HRESULT_FROM_NT(psi->AddTabStop(4)));
    VERIFY_IS_NOT_NULL(psi->_ptsTabs);
    VERIFY_ARE_EQUAL(4, psi->_ptsTabs->sColumn);
    VERIFY_ARE_EQUAL(12, psi->_ptsTabs->ptsNext->sColumn);

    Log::Comment(L"Add tab to tail of existing list.");
    VERIFY_SUCCEEDED(HRESULT_FROM_NT(psi->AddTabStop(30)));
    VERIFY_IS_NOT_NULL(psi->_ptsTabs);
    VERIFY_ARE_EQUAL(4, psi->_ptsTabs->sColumn);
    VERIFY_ARE_EQUAL(12, psi->_ptsTabs->ptsNext->sColumn);
    VERIFY_ARE_EQUAL(30, psi->_ptsTabs->ptsNext->ptsNext->sColumn);

    Log::Comment(L"Add tab to middle of existing list.");
    VERIFY_SUCCEEDED(HRESULT_FROM_NT(psi->AddTabStop(24)));
    VERIFY_IS_NOT_NULL(psi->_ptsTabs);
    VERIFY_ARE_EQUAL(4, psi->_ptsTabs->sColumn);
    VERIFY_ARE_EQUAL(12, psi->_ptsTabs->ptsNext->sColumn);
    VERIFY_ARE_EQUAL(24, psi->_ptsTabs->ptsNext->ptsNext->sColumn);
    VERIFY_ARE_EQUAL(30, psi->_ptsTabs->ptsNext->ptsNext->ptsNext->sColumn);

    Log::Comment(L"Add tab that duplicates an item in the existing list.");
    VERIFY_FAILED(HRESULT_FROM_NT(psi->AddTabStop(24)));
    VERIFY_IS_NOT_NULL(psi->_ptsTabs);
    VERIFY_ARE_EQUAL(4, psi->_ptsTabs->sColumn);
    VERIFY_ARE_EQUAL(12, psi->_ptsTabs->ptsNext->sColumn);
    VERIFY_ARE_EQUAL(24, psi->_ptsTabs->ptsNext->ptsNext->sColumn);
    VERIFY_ARE_EQUAL(30, psi->_ptsTabs->ptsNext->ptsNext->ptsNext->sColumn);

    //global free should clean this one up successfully since we created it with all class methods.
}

void ScreenBufferTests::TestClearTabStops()
{
    const CONSOLE_INFORMATION* const gci = ServiceLocator::LocateGlobals()->getConsoleInformation();
    SCREEN_INFORMATION* const psi = gci->CurrentScreenBuffer;
    psi->_ptsTabs = nullptr;

    Log::Comment(L"Clear non-existant tab stops.");
    {
        psi->ClearTabStops();
        VERIFY_IS_NULL(psi->_ptsTabs);
    }

    Log::Comment(L"Clear handful of tab stops.");
    {
        SCREEN_INFORMATION::TabStop** rgpTabs = CreateSampleList();

        psi->_ptsTabs = rgpTabs[0];

        psi->ClearTabStops();
        VERIFY_IS_NULL(psi->_ptsTabs);

        // They should have all been freed by the operation above, don't double free.
        for (size_t i = 0; i < cSampleListTabs; i++)
        {
            rgpTabs[i] = nullptr;
        }
        FreeSampleList(rgpTabs);
    }

    psi->_ptsTabs = nullptr; // don't let global free try to clean this up
}

void ScreenBufferTests::TestClearTabStop()
{
    const CONSOLE_INFORMATION* const gci = ServiceLocator::LocateGlobals()->getConsoleInformation();
    SCREEN_INFORMATION* const psi = gci->CurrentScreenBuffer;
    psi->_ptsTabs = nullptr;

    Log::Comment(L"Try to clear nonexistant list.");
    {
        psi->ClearTabStop(0);

        VERIFY_ARE_EQUAL(nullptr, psi->_ptsTabs, L"List should remain non-existant.");
    }

    Log::Comment(L"Allocate 1 list item and clear it.");
    {
        SCREEN_INFORMATION::TabStop* tabTest = new SCREEN_INFORMATION::TabStop();
        tabTest->ptsNext = nullptr;
        tabTest->sColumn = 0;

        psi->_ptsTabs = tabTest;

        psi->ClearTabStop(0);

        VERIFY_IS_NULL(psi->_ptsTabs);

        // no free, the ClearTabStop method did it for us.
    }

    Log::Comment(L"Allocate 1 list item and clear non-existant.");
    {
        SCREEN_INFORMATION::TabStop* tabTest = new SCREEN_INFORMATION::TabStop();
        tabTest->ptsNext = nullptr;
        tabTest->sColumn = 0;

        psi->_ptsTabs = tabTest;

        Log::Comment(L"Free greater");
        psi->ClearTabStop(1);

        VERIFY_IS_NOT_NULL(psi->_ptsTabs);

        Log::Comment(L"Free less than");
        psi->ClearTabStop(-1);

        VERIFY_IS_NOT_NULL(psi->_ptsTabs);

        delete tabTest;
    }

    Log::Comment(L"Allocate many (5) list items and clear head.");
    {
        SCREEN_INFORMATION::TabStop** rgpTabListTest = CreateSampleList();

        psi->_ptsTabs = rgpTabListTest[0];

        psi->ClearTabStop(rgSampleListValues[0]);

        VERIFY_ARE_EQUAL(rgpTabListTest[1], psi->_ptsTabs, L"1st item should take over as head.");
        Log::Comment(L"Remaining items should continue pointing to each other and have remaining values");

        VERIFY_ARE_EQUAL(rgpTabListTest[1]->ptsNext, rgpTabListTest[2]);
        VERIFY_ARE_EQUAL(rgpTabListTest[2]->ptsNext, rgpTabListTest[3]);
        VERIFY_ARE_EQUAL(rgpTabListTest[3]->ptsNext, rgpTabListTest[4]);
        VERIFY_ARE_EQUAL(rgpTabListTest[4]->ptsNext, nullptr);

        VERIFY_ARE_EQUAL(rgpTabListTest[1]->sColumn, rgSampleListValues[1]);
        VERIFY_ARE_EQUAL(rgpTabListTest[2]->sColumn, rgSampleListValues[2]);
        VERIFY_ARE_EQUAL(rgpTabListTest[3]->sColumn, rgSampleListValues[3]);
        VERIFY_ARE_EQUAL(rgpTabListTest[4]->sColumn, rgSampleListValues[4]);

        rgpTabListTest[0] = nullptr; // don't try to free already freed item.
        FreeSampleList(rgpTabListTest); // this will throw an exception in the test if the frees are incorrect
    }

    Log::Comment(L"Allocate many (5) list items and clear middle.");
    {
        SCREEN_INFORMATION::TabStop** rgpTabListTest = CreateSampleList();

        psi->_ptsTabs = rgpTabListTest[0];

        psi->ClearTabStop(rgSampleListValues[1]);

        Log::Comment(L"List should be reassembled without item 1.");
        VERIFY_ARE_EQUAL(rgpTabListTest[0], psi->_ptsTabs, L"0th item should stay as head.");
        VERIFY_ARE_EQUAL(rgpTabListTest[0]->ptsNext, rgpTabListTest[2]);
        VERIFY_ARE_EQUAL(rgpTabListTest[2]->ptsNext, rgpTabListTest[3]);
        VERIFY_ARE_EQUAL(rgpTabListTest[3]->ptsNext, rgpTabListTest[4]);
        VERIFY_ARE_EQUAL(rgpTabListTest[4]->ptsNext, nullptr);

        VERIFY_ARE_EQUAL(rgpTabListTest[0]->sColumn, rgSampleListValues[0]);
        VERIFY_ARE_EQUAL(rgpTabListTest[2]->sColumn, rgSampleListValues[2]);
        VERIFY_ARE_EQUAL(rgpTabListTest[3]->sColumn, rgSampleListValues[3]);
        VERIFY_ARE_EQUAL(rgpTabListTest[4]->sColumn, rgSampleListValues[4]);

        rgpTabListTest[1] = nullptr; // don't try to free already freed item.
        FreeSampleList(rgpTabListTest); // this will throw an exception in the test if the frees are incorrect
    }

    Log::Comment(L"Allocate many (5) list items and clear middle duplicate.");
    {
        SCREEN_INFORMATION::TabStop** rgpTabListTest = CreateSampleList();

        psi->_ptsTabs = rgpTabListTest[0];

        psi->ClearTabStop(rgSampleListValues[2]);

        Log::Comment(L"List should be reassembled without items 2 or 3.");
        VERIFY_ARE_EQUAL(rgpTabListTest[0], psi->_ptsTabs, L"0th item should stay as head.");
        VERIFY_ARE_EQUAL(rgpTabListTest[0]->ptsNext, rgpTabListTest[1]);
        VERIFY_ARE_EQUAL(rgpTabListTest[1]->ptsNext, rgpTabListTest[4]);
        VERIFY_ARE_EQUAL(rgpTabListTest[4]->ptsNext, nullptr);

        VERIFY_ARE_EQUAL(rgpTabListTest[0]->sColumn, rgSampleListValues[0]);
        VERIFY_ARE_EQUAL(rgpTabListTest[1]->sColumn, rgSampleListValues[1]);
        VERIFY_ARE_EQUAL(rgpTabListTest[4]->sColumn, rgSampleListValues[4]);

        rgpTabListTest[2] = nullptr; // don't try to free already freed item.
        rgpTabListTest[3] = nullptr; // don't try to free already freed item.
        FreeSampleList(rgpTabListTest); // this will throw an exception in the test if the frees are incorrect
    }

    Log::Comment(L"Allocate many (5) list items and clear tail.");
    {
        SCREEN_INFORMATION::TabStop** rgpTabListTest = CreateSampleList();

        psi->_ptsTabs = rgpTabListTest[0];

        psi->ClearTabStop(rgSampleListValues[4]);

        Log::Comment(L"List should be reassembled without item 4.");
        VERIFY_ARE_EQUAL(rgpTabListTest[0], psi->_ptsTabs, L"0th item should stay as head.");
        VERIFY_ARE_EQUAL(rgpTabListTest[0]->ptsNext, rgpTabListTest[1]);
        VERIFY_ARE_EQUAL(rgpTabListTest[1]->ptsNext, rgpTabListTest[2]);
        VERIFY_ARE_EQUAL(rgpTabListTest[2]->ptsNext, rgpTabListTest[3]);
        VERIFY_ARE_EQUAL(rgpTabListTest[3]->ptsNext, nullptr);

        VERIFY_ARE_EQUAL(rgpTabListTest[0]->sColumn, rgSampleListValues[0]);
        VERIFY_ARE_EQUAL(rgpTabListTest[1]->sColumn, rgSampleListValues[1]);
        VERIFY_ARE_EQUAL(rgpTabListTest[2]->sColumn, rgSampleListValues[2]);
        VERIFY_ARE_EQUAL(rgpTabListTest[3]->sColumn, rgSampleListValues[3]);

        rgpTabListTest[4] = nullptr; // don't try to free already freed item.
        FreeSampleList(rgpTabListTest); // this will throw an exception in the test if the frees are incorrect
    }

    Log::Comment(L"Allocate many (5) list items and clear non-existant item.");
    {
        SCREEN_INFORMATION::TabStop** rgpTabListTest = CreateSampleList();

        psi->_ptsTabs = rgpTabListTest[0];

        psi->ClearTabStop(9000);

        Log::Comment(L"List should remain the same.");
        VERIFY_ARE_EQUAL(rgpTabListTest[0], psi->_ptsTabs, L"0th item should stay as head.");
        VERIFY_ARE_EQUAL(rgpTabListTest[0]->ptsNext, rgpTabListTest[1]);
        VERIFY_ARE_EQUAL(rgpTabListTest[1]->ptsNext, rgpTabListTest[2]);
        VERIFY_ARE_EQUAL(rgpTabListTest[2]->ptsNext, rgpTabListTest[3]);
        VERIFY_ARE_EQUAL(rgpTabListTest[3]->ptsNext, rgpTabListTest[4]);
        VERIFY_ARE_EQUAL(rgpTabListTest[4]->ptsNext, nullptr);

        VERIFY_ARE_EQUAL(rgpTabListTest[0]->sColumn, rgSampleListValues[0]);
        VERIFY_ARE_EQUAL(rgpTabListTest[1]->sColumn, rgSampleListValues[1]);
        VERIFY_ARE_EQUAL(rgpTabListTest[2]->sColumn, rgSampleListValues[2]);
        VERIFY_ARE_EQUAL(rgpTabListTest[3]->sColumn, rgSampleListValues[3]);
        VERIFY_ARE_EQUAL(rgpTabListTest[4]->sColumn, rgSampleListValues[4]);

        FreeSampleList(rgpTabListTest); // this will throw an exception in the test if the frees are incorrect
    }

    psi->_ptsTabs = nullptr; // prevent global cleanup of this, we already did it.
}

void ScreenBufferTests::TestGetForwardTab()
{
    const CONSOLE_INFORMATION* const gci = ServiceLocator::LocateGlobals()->getConsoleInformation();
    SCREEN_INFORMATION* const psi = gci->CurrentScreenBuffer;
    psi->_ptsTabs = nullptr;

    SCREEN_INFORMATION::TabStop** rgpTabs = CreateSampleList();
    {
        psi->_ptsTabs = rgpTabs[0];

        const COORD coordScreenBufferSize = psi->GetScreenBufferSize();
        COORD coordCursor;
        coordCursor.Y = coordScreenBufferSize.Y / 2; // in the middle of the buffer, it doesn't make a difference.

        Log::Comment(L"Find next tab from before front.");
        {
            coordCursor.X = 0;

            COORD coordCursorExpected;
            coordCursorExpected = coordCursor;
            coordCursorExpected.X = rgSampleListValues[0];

            COORD const coordCursorResult = psi->GetForwardTab(coordCursor);
            VERIFY_ARE_EQUAL(coordCursorExpected, coordCursorResult, L"Cursor advanced to first tab stop from sample list.");
        }

        Log::Comment(L"Find next tab from in the middle.");
        {
            coordCursor.X = 6;

            COORD coordCursorExpected;
            coordCursorExpected = coordCursor;
            coordCursorExpected.X = rgSampleListValues[2];

            COORD const coordCursorResult = psi->GetForwardTab(coordCursor);
            VERIFY_ARE_EQUAL(coordCursorExpected, coordCursorResult, L"Cursor advanced to middle tab stop from sample list.");
        }

        Log::Comment(L"Find next tab from end.");
        {
            coordCursor.X = 30;

            COORD coordCursorExpected;
            coordCursorExpected = coordCursor;
            coordCursorExpected.X = coordScreenBufferSize.X - 1;

            COORD const coordCursorResult = psi->GetForwardTab(coordCursor);
            VERIFY_ARE_EQUAL(coordCursorExpected, coordCursorResult, L"Cursor advanced to end of screen buffer.");
        }

        FreeSampleList(rgpTabs);
        psi->_ptsTabs = nullptr; // don't let global free try to clean this up
    }
}

void ScreenBufferTests::TestGetReverseTab()
{
    const CONSOLE_INFORMATION* const gci = ServiceLocator::LocateGlobals()->getConsoleInformation();
    SCREEN_INFORMATION* const psi = gci->CurrentScreenBuffer;
    psi->_ptsTabs = nullptr;

    SCREEN_INFORMATION::TabStop** rgpTabs = CreateSampleList();
    {
        psi->_ptsTabs = rgpTabs[0];

        COORD coordCursor;
        coordCursor.Y = psi->GetScreenBufferSize().Y / 2; // in the middle of the buffer, it doesn't make a difference.

        Log::Comment(L"Find previous tab from before front.");
        {
            coordCursor.X = 1;

            COORD coordCursorExpected;
            coordCursorExpected = coordCursor;
            coordCursorExpected.X = 0;

            COORD const coordCursorResult = psi->GetReverseTab(coordCursor);
            VERIFY_ARE_EQUAL(coordCursorExpected, coordCursorResult, L"Cursor adjusted to beginning of the buffer when it started before sample list.");
        }

        Log::Comment(L"Find previous tab from in the middle.");
        {
            coordCursor.X = 6;

            COORD coordCursorExpected;
            coordCursorExpected = coordCursor;
            coordCursorExpected.X = rgSampleListValues[1];

            COORD const coordCursorResult = psi->GetReverseTab(coordCursor);
            VERIFY_ARE_EQUAL(coordCursorExpected, coordCursorResult, L"Cursor adjusted back one tab spot from middle of sample list.");
        }

        Log::Comment(L"Find next tab from end.");
        {
            coordCursor.X = 30;

            COORD coordCursorExpected;
            coordCursorExpected = coordCursor;
            coordCursorExpected.X = rgSampleListValues[4];

            COORD const coordCursorResult = psi->GetReverseTab(coordCursor);
            VERIFY_ARE_EQUAL(coordCursorExpected, coordCursorResult, L"Cursor adjusted to last item in the sample list from position beyond end.");
        }

        FreeSampleList(rgpTabs);
        psi->_ptsTabs = nullptr; // don't let global free try to clean this up
    }
}

void ScreenBufferTests::TestAreTabsSet()
{
    const CONSOLE_INFORMATION* const gci = ServiceLocator::LocateGlobals()->getConsoleInformation();
    SCREEN_INFORMATION* const psi = gci->CurrentScreenBuffer;
    psi->_ptsTabs = nullptr;

    VERIFY_IS_FALSE(psi->AreTabsSet());

    SCREEN_INFORMATION::TabStop stop;
    psi->_ptsTabs = &stop;

    VERIFY_IS_TRUE(psi->AreTabsSet());

    psi->_ptsTabs = nullptr; // don't let global free try to clean this up
}

void ScreenBufferTests::EraseAllTests()
{
    const CONSOLE_INFORMATION* const gci = ServiceLocator::LocateGlobals()->getConsoleInformation();
    SCREEN_INFORMATION* const psi = gci->CurrentScreenBuffer;
    auto bufferWriter = psi->GetBufferWriter();
    auto cursor = psi->TextInfo->GetCursor();
    VERIFY_IS_NOT_NULL(bufferWriter);
    VERIFY_IS_NOT_NULL(cursor);

    VERIFY_ARE_EQUAL(psi->GetBufferViewport().Top, 0);

    ////////////////////////////////////////////////////////////////////////
    Log::Comment(L"Case 1: Erase a single line of text in the buffer\n");

    bufferWriter->PrintString(L"foo", 3);
    VERIFY_ARE_EQUAL(cursor->GetPosition().X, 3);
    VERIFY_ARE_EQUAL(cursor->GetPosition().Y, 0);
    VERIFY_ARE_EQUAL(psi->GetBufferViewport().Top, 0);

    psi->VtEraseAll();

    VERIFY_ARE_EQUAL(cursor->GetPosition().X, 0);
    VERIFY_ARE_EQUAL(cursor->GetPosition().Y, 1);
    auto viewport = psi->GetBufferViewport();
    VERIFY_ARE_EQUAL(viewport.Top, 1);
    Log::Comment(NoThrowString().Format(
        L"viewport={L:%d,T:%d,R:%d,B:%d}",
        viewport.Left, viewport.Top, viewport.Right, viewport.Bottom
    ));

    ////////////////////////////////////////////////////////////////////////
    Log::Comment(L"Case 2: Erase multiple lines, below the top of the buffer\n");

    bufferWriter->PrintString(L"bar\nbar\nbar", 11);
    VERIFY_ARE_EQUAL(cursor->GetPosition().X, 3);
    VERIFY_ARE_EQUAL(cursor->GetPosition().Y, 3);
    viewport = psi->GetBufferViewport();
    VERIFY_ARE_EQUAL(viewport.Top, 1);
    Log::Comment(NoThrowString().Format(
        L"viewport={L:%d,T:%d,R:%d,B:%d}",
        viewport.Left, viewport.Top, viewport.Right, viewport.Bottom
    ));

    psi->VtEraseAll();
    VERIFY_ARE_EQUAL(cursor->GetPosition().X, 0);
    VERIFY_ARE_EQUAL(cursor->GetPosition().Y, 4);
    viewport = psi->GetBufferViewport();
    VERIFY_ARE_EQUAL(viewport.Top, 4);
    Log::Comment(NoThrowString().Format(
        L"viewport={L:%d,T:%d,R:%d,B:%d}",
        viewport.Left, viewport.Top, viewport.Right, viewport.Bottom
    ));


    ////////////////////////////////////////////////////////////////////////
    Log::Comment(L"Case 3: multiple lines at the bottom of the buffer\n");

    cursor->SetPosition({0, 275});
    bufferWriter->PrintString(L"bar\nbar\nbar", 11);
    VERIFY_ARE_EQUAL(cursor->GetPosition().X, 3);
    VERIFY_ARE_EQUAL(cursor->GetPosition().Y, 277);
    viewport = psi->GetBufferViewport();
    Log::Comment(NoThrowString().Format(
        L"viewport={L:%d,T:%d,R:%d,B:%d}",
        viewport.Left, viewport.Top, viewport.Right, viewport.Bottom
    ));
    psi->VtEraseAll();

    viewport = psi->GetBufferViewport();
    auto heightFromBottom = psi->GetScreenBufferSize().Y - (viewport.Bottom - viewport.Top + 1);
    VERIFY_ARE_EQUAL(cursor->GetPosition().X, 0);
    VERIFY_ARE_EQUAL(cursor->GetPosition().Y, heightFromBottom);
    VERIFY_ARE_EQUAL(viewport.Top, heightFromBottom);
    Log::Comment(NoThrowString().Format(
        L"viewport={L:%d,T:%d,R:%d,B:%d}",
        viewport.Left, viewport.Top, viewport.Right, viewport.Bottom
    ));
}

void ScreenBufferTests::VtResize()
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

    auto initialSbHeight = psi->GetScreenBufferSize().Y;
    auto initialSbWidth = psi->GetScreenBufferSize().X;
    // The viewport is an inclusive rect, so we need +1's
    auto initialViewHeight = psi->GetBufferViewport().Bottom - psi->GetBufferViewport().Top + 1;
    auto initialViewWidth = psi->GetBufferViewport().Right - psi->GetBufferViewport().Left + 1;

    Log::Comment(NoThrowString().Format(
        L"Write '\x1b[8;30;80t'"
        L" The Screen buffer height should remain unchanged, but the width should be 80 columns"
        L" The viewport should be w,h=80,30"
    ));

    std::wstring sequence = L"\x1b[8;30;80t";
    stateMachine->ProcessString(&sequence[0], sequence.length());

    auto newSbHeight = psi->GetScreenBufferSize().Y;
    auto newSbWidth = psi->GetScreenBufferSize().X;
    // The viewport is an inclusive rect, so we need +1's
    auto newViewHeight = psi->GetBufferViewport().Bottom - psi->GetBufferViewport().Top + 1;
    auto newViewWidth = psi->GetBufferViewport().Right - psi->GetBufferViewport().Left + 1;

    VERIFY_ARE_EQUAL(initialSbHeight, newSbHeight);
    VERIFY_ARE_EQUAL(80, newSbWidth);
    VERIFY_ARE_EQUAL(30, newViewHeight);
    VERIFY_ARE_EQUAL(80, newViewWidth);

    initialSbHeight = newSbHeight;
    initialSbWidth = newSbWidth;
    initialViewHeight = newViewHeight;
    initialViewWidth = newViewWidth;

    Log::Comment(NoThrowString().Format(
        L"Write '\x1b[8;40;80t'"
        L" The Screen buffer height should remain unchanged, but the width should be 80 columns"
        L" The viewport should be w,h=80,40"
    ));

    sequence = L"\x1b[8;40;80t";
    stateMachine->ProcessString(&sequence[0], sequence.length());

    newSbHeight = psi->GetScreenBufferSize().Y;
    newSbWidth = psi->GetScreenBufferSize().X;
    newViewHeight = psi->GetBufferViewport().Bottom - psi->GetBufferViewport().Top + 1;
    newViewWidth = psi->GetBufferViewport().Right - psi->GetBufferViewport().Left + 1;

    VERIFY_ARE_EQUAL(initialSbHeight, newSbHeight);
    VERIFY_ARE_EQUAL(80, newSbWidth);
    VERIFY_ARE_EQUAL(40, newViewHeight);
    VERIFY_ARE_EQUAL(80, newViewWidth);

    initialSbHeight = newSbHeight;
    initialSbWidth = newSbWidth;
    initialViewHeight = newViewHeight;
    initialViewWidth = newViewWidth;

    Log::Comment(NoThrowString().Format(
        L"Write '\x1b[8;40;90t'"
        L" The Screen buffer height should remain unchanged, but the width should be 90 columns"
        L" The viewport should be w,h=90,40"
    ));

    sequence = L"\x1b[8;40;90t";
    stateMachine->ProcessString(&sequence[0], sequence.length());

    newSbHeight = psi->GetScreenBufferSize().Y;
    newSbWidth = psi->GetScreenBufferSize().X;
    newViewHeight = psi->GetBufferViewport().Bottom - psi->GetBufferViewport().Top + 1;
    newViewWidth = psi->GetBufferViewport().Right - psi->GetBufferViewport().Left + 1;

    VERIFY_ARE_EQUAL(initialSbHeight, newSbHeight);
    VERIFY_ARE_EQUAL(90, newSbWidth);
    VERIFY_ARE_EQUAL(40, newViewHeight);
    VERIFY_ARE_EQUAL(90, newViewWidth);

    initialSbHeight = newSbHeight;
    initialSbWidth = newSbWidth;
    initialViewHeight = newViewHeight;
    initialViewWidth = newViewWidth;

    Log::Comment(NoThrowString().Format(
        L"Write '\x1b[8;12;12t'"
        L" The Screen buffer height should remain unchanged, but the width should be 12 columns"
        L" The viewport should be w,h=12,12"
    ));

    sequence = L"\x1b[8;12;12t";
    stateMachine->ProcessString(&sequence[0], sequence.length());

    newSbHeight = psi->GetScreenBufferSize().Y;
    newSbWidth = psi->GetScreenBufferSize().X;
    newViewHeight = psi->GetBufferViewport().Bottom - psi->GetBufferViewport().Top + 1;
    newViewWidth = psi->GetBufferViewport().Right - psi->GetBufferViewport().Left + 1;

    VERIFY_ARE_EQUAL(initialSbHeight, newSbHeight);
    VERIFY_ARE_EQUAL(12, newSbWidth);
    VERIFY_ARE_EQUAL(12, newViewHeight);
    VERIFY_ARE_EQUAL(12, newViewWidth);

    initialSbHeight = newSbHeight;
    initialSbWidth = newSbWidth;
    initialViewHeight = newViewHeight;
    initialViewWidth = newViewWidth;

    Log::Comment(NoThrowString().Format(
        L"Write '\x1b[8;0;0t'"
        L" Nothing should change"
    ));

    sequence = L"\x1b[8;0;0t";
    stateMachine->ProcessString(&sequence[0], sequence.length());

    newSbHeight = psi->GetScreenBufferSize().Y;
    newSbWidth = psi->GetScreenBufferSize().X;
    newViewHeight = psi->GetBufferViewport().Bottom - psi->GetBufferViewport().Top + 1;
    newViewWidth = psi->GetBufferViewport().Right - psi->GetBufferViewport().Left + 1;

    VERIFY_ARE_EQUAL(initialSbHeight, newSbHeight);
    VERIFY_ARE_EQUAL(initialSbWidth, newSbWidth);
    VERIFY_ARE_EQUAL(initialViewHeight, newViewHeight);
    VERIFY_ARE_EQUAL(initialViewWidth, newViewWidth);

}

void ScreenBufferTests::VtSoftResetCursorPosition()
{
    CONSOLE_INFORMATION* const gci = ServiceLocator::LocateGlobals()->getConsoleInformation();
    SCREEN_INFORMATION* const psi = gci->CurrentScreenBuffer->GetActiveBuffer();
    const TEXT_BUFFER_INFO* const tbi = psi->TextInfo;
    StateMachine* const stateMachine = psi->GetStateMachine();
    Cursor* const cursor = tbi->GetCursor();

    Log::Comment(NoThrowString().Format(
        L"Make sure the viewport is at 0,0"
    ));
    psi->SetViewportOrigin(true, COORD({0, 0}));

    Log::Comment(NoThrowString().Format(
        L"Move the cursor to 2,2, then execute a soft reset.\n"
        L"The cursor should not move."
    ));

    std::wstring seq = L"\x1b[2;2H";
    stateMachine->ProcessString(&seq[0], seq.length());
    VERIFY_ARE_EQUAL( COORD({1, 1}), cursor->GetPosition());

    seq = L"\x1b[!p";
    stateMachine->ProcessString(&seq[0], seq.length());
    VERIFY_ARE_EQUAL( COORD({1, 1}), cursor->GetPosition());

    Log::Comment(NoThrowString().Format(
        L"Set some margins. The cursor should move home."
    ));

    seq = L"\x1b[2;10r";
    stateMachine->ProcessString(&seq[0], seq.length());
    VERIFY_ARE_EQUAL( COORD({0, 0}), cursor->GetPosition());

    Log::Comment(NoThrowString().Format(
        L"Move the cursor to 2,2, then execute a soft reset.\n"
        L"The cursor should not move, even though there are margins."
    ));
    seq = L"\x1b[2;2H";
    stateMachine->ProcessString(&seq[0], seq.length());
    VERIFY_ARE_EQUAL( COORD({1, 1}), cursor->GetPosition());
    seq = L"\x1b[!p";
    stateMachine->ProcessString(&seq[0], seq.length());
    VERIFY_ARE_EQUAL( COORD({1, 1}), cursor->GetPosition());
}


void ScreenBufferTests::VtSetColorTable()
{
    CONSOLE_INFORMATION* const gci = ServiceLocator::LocateGlobals()->getConsoleInformation();
    SCREEN_INFORMATION* const psi = gci->CurrentScreenBuffer->GetActiveBuffer();
    StateMachine* const stateMachine = psi->GetStateMachine();

    // Start with a known value
    gci->SetColorTableEntry(0, RGB(0, 0, 0));
    
    Log::Comment(NoThrowString().Format(
        L"Process some valid sequences for setting the table"
    ));

    std::wstring seq = L"\x1b]4;0;rgb:1/1/1\x7";
    stateMachine->ProcessString(&seq[0], seq.length());
    VERIFY_ARE_EQUAL(RGB(1,1,1), gci->GetColorTableEntry(::XtermToWindowsIndex(0)));

    seq = L"\x1b]4;1;rgb:1/23/1\x7";
    stateMachine->ProcessString(&seq[0], seq.length());
    VERIFY_ARE_EQUAL(RGB(1,0x23,1), gci->GetColorTableEntry(::XtermToWindowsIndex(1)));

    seq = L"\x1b]4;2;rgb:1/23/12\x7";
    stateMachine->ProcessString(&seq[0], seq.length());
    VERIFY_ARE_EQUAL(RGB(1,0x23,0x12), gci->GetColorTableEntry(::XtermToWindowsIndex(2)));

    seq = L"\x1b]4;3;rgb:12/23/12\x7";
    stateMachine->ProcessString(&seq[0], seq.length());
    VERIFY_ARE_EQUAL(RGB(0x12,0x23,0x12), gci->GetColorTableEntry(::XtermToWindowsIndex(3)));
    
    seq = L"\x1b]4;4;rgb:ff/a1/1b\x7";
    stateMachine->ProcessString(&seq[0], seq.length());
    VERIFY_ARE_EQUAL(RGB(0xff,0xa1,0x1b), gci->GetColorTableEntry(::XtermToWindowsIndex(4)));

    seq = L"\x1b]4;5;rgb:ff/a1/1b\x1b\\";
    stateMachine->ProcessString(&seq[0], seq.length());
    VERIFY_ARE_EQUAL(RGB(0xff,0xa1,0x1b), gci->GetColorTableEntry(::XtermToWindowsIndex(5)));

    Log::Comment(NoThrowString().Format(
        L"Try a bunch of invalid sequences."
    ));
    Log::Comment(NoThrowString().Format(
        L"First start by setting an entry to a known value to compare to."
    ));
    seq = L"\x1b]4;5;rgb:9/9/9\x1b\\";
    stateMachine->ProcessString(&seq[0], seq.length());
    VERIFY_ARE_EQUAL(RGB(9,9,9), gci->GetColorTableEntry(::XtermToWindowsIndex(5)));

    Log::Comment(NoThrowString().Format(
        L"invalid: Missing the first component"
    ));
    seq = L"\x1b]4;5;rgb:/1/1\x1b\\";
    stateMachine->ProcessString(&seq[0], seq.length());
    VERIFY_ARE_EQUAL(RGB(9,9,9), gci->GetColorTableEntry(::XtermToWindowsIndex(5)));

    Log::Comment(NoThrowString().Format(
        L"invalid: too many characters in a component"
    ));
    seq = L"\x1b]4;5;rgb:111/1/1\x1b\\";
    stateMachine->ProcessString(&seq[0], seq.length());
    VERIFY_ARE_EQUAL(RGB(9,9,9), gci->GetColorTableEntry(::XtermToWindowsIndex(5)));

    Log::Comment(NoThrowString().Format(
        L"invalid: too many componenets"
    ));
    seq = L"\x1b]4;5;rgb:1/1/1/1\x1b\\";
    stateMachine->ProcessString(&seq[0], seq.length());
    VERIFY_ARE_EQUAL(RGB(9,9,9), gci->GetColorTableEntry(::XtermToWindowsIndex(5)));

    Log::Comment(NoThrowString().Format(
        L"invalid: no second component"
    ));
    seq = L"\x1b]4;5;rgb:1//1\x1b\\";
    stateMachine->ProcessString(&seq[0], seq.length());
    VERIFY_ARE_EQUAL(RGB(9,9,9), gci->GetColorTableEntry(::XtermToWindowsIndex(5)));

    Log::Comment(NoThrowString().Format(
        L"invalid: no components"
    ));
    seq = L"\x1b]4;5;rgb://\x1b\\";
    stateMachine->ProcessString(&seq[0], seq.length());
    VERIFY_ARE_EQUAL(RGB(9,9,9), gci->GetColorTableEntry(::XtermToWindowsIndex(5)));

    Log::Comment(NoThrowString().Format(
        L"invalid: no third component"
    ));
    seq = L"\x1b]4;5;rgb:1/11/\x1b\\";
    stateMachine->ProcessString(&seq[0], seq.length());
    VERIFY_ARE_EQUAL(RGB(9,9,9), gci->GetColorTableEntry(::XtermToWindowsIndex(5)));

    Log::Comment(NoThrowString().Format(
        L"invalid: rgbi is not a supported color space"
    ));
    seq = L"\x1b]4;5;rgbi:1/1/1\x1b\\";
    stateMachine->ProcessString(&seq[0], seq.length());
    VERIFY_ARE_EQUAL(RGB(9,9,9), gci->GetColorTableEntry(::XtermToWindowsIndex(5)));

    Log::Comment(NoThrowString().Format(
        L"invalid: cmyk is not a supported color space"
    ));
    seq = L"\x1b]4;5;cmyk:1/1/1\x1b\\";
    stateMachine->ProcessString(&seq[0], seq.length());
    VERIFY_ARE_EQUAL(RGB(9,9,9), gci->GetColorTableEntry(::XtermToWindowsIndex(5)));

    Log::Comment(NoThrowString().Format(
        L"invalid: no table index should do nothing"
    ));
    seq = L"\x1b]4;;rgb:1/1/1\x1b\\";
    stateMachine->ProcessString(&seq[0], seq.length());
    VERIFY_ARE_EQUAL(RGB(9,9,9), gci->GetColorTableEntry(::XtermToWindowsIndex(5)));

    Log::Comment(NoThrowString().Format(
        L"invalid: need to specify a color space"
    ));
    seq = L"\x1b]4;5;1/1/1\x1b\\";
    stateMachine->ProcessString(&seq[0], seq.length());
    VERIFY_ARE_EQUAL(RGB(9,9,9), gci->GetColorTableEntry(::XtermToWindowsIndex(5)));
}

void ScreenBufferTests::ResizeTraditionalDoesntDoubleFreeAttrRows()
{
    // there is not much to verify here, this test passes if the console doesn't crash.
    CONSOLE_INFORMATION* const gci = ServiceLocator::LocateGlobals()->getConsoleInformation();
    SCREEN_INFORMATION* const psi = gci->CurrentScreenBuffer->GetActiveBuffer();

    gci->SetWrapText(false);
    COORD newBufferSize = psi->_coordScreenBufferSize;
    newBufferSize.Y--;

    VERIFY_SUCCESS_NTSTATUS(psi->ResizeTraditional(newBufferSize));

}
