/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/
#include "precomp.h"

// some assumptions have been made on this value. only change it if you have a good reason to.
#define NUMBER_OF_SCENARIO_INPUTS 10
#define READ_BATCH 3

// This class is intended to test:
// FlushConsoleInputBuffer
// PeekConsoleInput
// ReadConsoleInput
// WriteConsoleInput
// GetNumberOfConsoleInputEvents
// GetNumberOfConsoleMouseButtons
class InputTests
{
    TEST_CLASS(InputTests);

    TEST_CLASS_SETUP(TestSetup);
    TEST_CLASS_CLEANUP(TestCleanup);

    TEST_METHOD(TestGetMouseButtonsValid); // note: GetNumberOfConsoleMouseButtons crashes with nullptr, so there's no
                                           // negative test
    TEST_METHOD(TestInputScenario);
    TEST_METHOD(TestFlushValid);
    TEST_METHOD(TestFlushInvalid);
    TEST_METHOD(TestPeekConsoleInvalid);
    TEST_METHOD(TestReadConsoleInvalid);
    TEST_METHOD(TestWriteConsoleInvalid);

    TEST_METHOD(TestMouseWheelReadConsoleMouseInput);
    TEST_METHOD(TestMouseHorizWheelReadConsoleMouseInput);
    TEST_METHOD(TestMouseWheelReadConsoleNoMouseInput);
    TEST_METHOD(TestMouseHorizWheelReadConsoleNoMouseInput);
    TEST_METHOD(TestMouseWheelReadConsoleInputQuickEdit);
    TEST_METHOD(TestMouseHorizWheelReadConsoleInputQuickEdit);
};

void VerifyNumberOfInputRecords(_In_ const HANDLE hConsoleInput, _In_ DWORD nInputs)
{
    SetVerifyOutput verifySettings(VerifyOutputSettings::LogOnlyFailures);
    DWORD nInputEvents = (DWORD)-1;
    VERIFY_WIN32_BOOL_SUCCEEDED(GetNumberOfConsoleInputEvents(hConsoleInput, &nInputEvents));
    VERIFY_ARE_EQUAL(nInputEvents,
                     nInputs,
                     L"Verify number of input events");
}

bool InputTests::TestSetup()
{
    const bool fRet = Common::TestBufferSetup();

    HANDLE hConsoleInput = GetStdInputHandle();
    VERIFY_WIN32_BOOL_SUCCEEDED(FlushConsoleInputBuffer(hConsoleInput));
    VerifyNumberOfInputRecords(hConsoleInput, 0);

    return fRet;
}

bool InputTests::TestCleanup()
{
    return Common::TestBufferCleanup();
}

void InputTests::TestGetMouseButtonsValid()
{
    DWORD nMouseButtons = (DWORD)-1;
    VERIFY_WIN32_BOOL_SUCCEEDED(GetNumberOfConsoleMouseButtons(&nMouseButtons));
    VERIFY_ARE_EQUAL(nMouseButtons, (DWORD)GetSystemMetrics(SM_CMOUSEBUTTONS));
}

void GenerateAndWriteInputRecords(_In_ const HANDLE hConsoleInput,
                                  _In_ const UINT cRecordsToGenerate,
                                  _Out_writes_(cRecs) INPUT_RECORD *prgRecs,
                                  _In_ const DWORD cRecs,
                                  _Out_ PDWORD pdwWritten)
{
    Log::Comment(String().Format(L"Generating %d input events", cRecordsToGenerate));
    for (UINT iRecord = 0; iRecord < cRecs; iRecord++)
    {
        prgRecs[iRecord].EventType = KEY_EVENT;
        prgRecs[iRecord].Event.KeyEvent.bKeyDown = FALSE;
        prgRecs[iRecord].Event.KeyEvent.wRepeatCount = 1;
        prgRecs[iRecord].Event.KeyEvent.wVirtualKeyCode = ('A' + (WORD)iRecord);
    }

    Log::Comment(L"Writing events");
    VERIFY_WIN32_BOOL_SUCCEEDED(WriteConsoleInput(hConsoleInput, prgRecs, cRecs, pdwWritten));
    VERIFY_ARE_EQUAL(*pdwWritten,
                     cRecs,
                     L"verify number written");
}

void InputTests::TestInputScenario()
{
    Log::Comment(L"Get input handle");
    HANDLE hConsoleInput = GetStdInputHandle();

    DWORD nWrittenEvents = (DWORD)-1;
    INPUT_RECORD rgInputRecords[NUMBER_OF_SCENARIO_INPUTS] = {0};
    GenerateAndWriteInputRecords(hConsoleInput, NUMBER_OF_SCENARIO_INPUTS, rgInputRecords, ARRAYSIZE(rgInputRecords), &nWrittenEvents);

    VerifyNumberOfInputRecords(hConsoleInput, ARRAYSIZE(rgInputRecords));

    Log::Comment(L"Peeking events");
    INPUT_RECORD rgPeekedRecords[NUMBER_OF_SCENARIO_INPUTS] = {0};
    DWORD nPeekedEvents = (DWORD)-1;
    VERIFY_WIN32_BOOL_SUCCEEDED(PeekConsoleInput(hConsoleInput, rgPeekedRecords, ARRAYSIZE(rgPeekedRecords), &nPeekedEvents));
    VERIFY_ARE_EQUAL(nPeekedEvents, nWrittenEvents, L"We should be able to peek at all of the records we've written");
    for (UINT iPeekedRecord = 0; iPeekedRecord < nPeekedEvents; iPeekedRecord++)
    {
        VERIFY_ARE_EQUAL(rgPeekedRecords[iPeekedRecord],
                         rgInputRecords[iPeekedRecord],
                         L"make sure our peeked records match what we input");
    }

    // read inputs 3 at a time until we've read them all. since the number we're batching by doesn't match the number of
    // total events, we need to account for the last incomplete read we'll perform.
    const UINT cIterations = (NUMBER_OF_SCENARIO_INPUTS / READ_BATCH) + ((NUMBER_OF_SCENARIO_INPUTS % READ_BATCH > 0) ? 1 : 0);
    for (UINT iIteration = 0; iIteration < cIterations; iIteration++)
    {
        const bool fIsLastIteration = (iIteration + 1) > (NUMBER_OF_SCENARIO_INPUTS / READ_BATCH);
        Log::Comment(String().Format(L"Reading inputs (iteration %d/%d)%s",
                                     iIteration+1,
                                     cIterations,
                                     fIsLastIteration ? L" (last one)" : L""));

        INPUT_RECORD rgReadRecords[READ_BATCH] = {0};
        DWORD nReadEvents = (DWORD)-1;
        VERIFY_WIN32_BOOL_SUCCEEDED(ReadConsoleInput(hConsoleInput, rgReadRecords, ARRAYSIZE(rgReadRecords), &nReadEvents));

        DWORD dwExpectedEventsRead = READ_BATCH;
        if (fIsLastIteration)
        {
            // on the last iteration, we'll have an incomplete read. account for it here.
            dwExpectedEventsRead = NUMBER_OF_SCENARIO_INPUTS % READ_BATCH;
        }

        VERIFY_ARE_EQUAL(nReadEvents, dwExpectedEventsRead);
        for (UINT iReadRecord = 0; iReadRecord < nReadEvents; iReadRecord++)
        {
            const UINT iInputRecord = iReadRecord+(iIteration*READ_BATCH);
            VERIFY_ARE_EQUAL(rgReadRecords[iReadRecord],
                             rgInputRecords[iInputRecord],
                             String().Format(L"verify record %d", iInputRecord));
        }

        DWORD nInputEventsAfterRead = (DWORD)-1;
        VERIFY_WIN32_BOOL_SUCCEEDED(GetNumberOfConsoleInputEvents(hConsoleInput, &nInputEventsAfterRead));

        DWORD dwExpectedEventsAfterRead = (NUMBER_OF_SCENARIO_INPUTS - (READ_BATCH*(iIteration+1)));
        if (fIsLastIteration)
        {
            dwExpectedEventsAfterRead = 0;
        }
        VERIFY_ARE_EQUAL(dwExpectedEventsAfterRead,
                         nInputEventsAfterRead,
                         L"verify number of remaining inputs");
    }
}

void InputTests::TestFlushValid()
{
    Log::Comment(L"Get input handle");
    HANDLE hConsoleInput = GetStdInputHandle();

    DWORD nWrittenEvents = (DWORD)-1;
    INPUT_RECORD rgInputRecords[NUMBER_OF_SCENARIO_INPUTS] = {0};
    GenerateAndWriteInputRecords(hConsoleInput, NUMBER_OF_SCENARIO_INPUTS, rgInputRecords, ARRAYSIZE(rgInputRecords), &nWrittenEvents);

    VerifyNumberOfInputRecords(hConsoleInput, ARRAYSIZE(rgInputRecords));

    VERIFY_WIN32_BOOL_SUCCEEDED(FlushConsoleInputBuffer(hConsoleInput));

    VerifyNumberOfInputRecords(hConsoleInput, 0);
}

void InputTests::TestFlushInvalid()
{
    // NOTE: FlushConsoleInputBuffer(nullptr) crashes, so we don't verify that here.
    VERIFY_WIN32_BOOL_FAILED(FlushConsoleInputBuffer(INVALID_HANDLE_VALUE));
}

void InputTests::TestPeekConsoleInvalid()
{
    DWORD nPeeked = (DWORD)-1;
    VERIFY_WIN32_BOOL_FAILED(PeekConsoleInput(INVALID_HANDLE_VALUE, nullptr, 0, &nPeeked)); // NOTE: nPeeked is required
    VERIFY_ARE_EQUAL(nPeeked, (DWORD)0);

    HANDLE hConsoleInput = GetStdInputHandle();

    nPeeked = (DWORD)-1;
    VERIFY_WIN32_BOOL_FAILED(PeekConsoleInput(hConsoleInput, nullptr, 5, &nPeeked));
    VERIFY_ARE_EQUAL(nPeeked, (DWORD)0);

    DWORD nWritten = (DWORD)-1;
    INPUT_RECORD ir = {0};
    GenerateAndWriteInputRecords(hConsoleInput, 1, &ir, 1, &nWritten);

    VerifyNumberOfInputRecords(hConsoleInput, 1);

    nPeeked = (DWORD)-1;
    INPUT_RECORD irPeeked = {0};
    VERIFY_WIN32_BOOL_SUCCEEDED(PeekConsoleInput(hConsoleInput, &irPeeked, 0, &nPeeked));
    VERIFY_ARE_EQUAL(nPeeked, (DWORD)0, L"Verify that an empty array doesn't cause peeks to get written");

    VerifyNumberOfInputRecords(hConsoleInput, 1);

    VERIFY_WIN32_BOOL_SUCCEEDED(FlushConsoleInputBuffer(hConsoleInput));
}

void InputTests::TestReadConsoleInvalid()
{
    DWORD nRead = (DWORD)-1;
    VERIFY_WIN32_BOOL_FAILED(ReadConsoleInput(0, nullptr, 0, &nRead));
    VERIFY_ARE_EQUAL(nRead, (DWORD)0);

    nRead = (DWORD)-1;
    VERIFY_WIN32_BOOL_FAILED(ReadConsoleInput(INVALID_HANDLE_VALUE, nullptr, 0, &nRead));
    VERIFY_ARE_EQUAL(nRead, (DWORD)0);

    // NOTE: ReadConsoleInput blocks until at least one input event is read, even if the operation would result in no
    // records actually being read (e.g. valid handle, NULL lpBuffer)

    HANDLE hConsoleInput = GetStdInputHandle();

    DWORD nWritten = (DWORD)-1;
    INPUT_RECORD irWrite = {0};
    GenerateAndWriteInputRecords(hConsoleInput, 1, &irWrite, 1, &nWritten);
    VerifyNumberOfInputRecords(hConsoleInput, 1);

    nRead = (DWORD)-1;
    VERIFY_WIN32_BOOL_SUCCEEDED(ReadConsoleInput(hConsoleInput, nullptr, 0, &nRead));
    VERIFY_ARE_EQUAL(nRead, (DWORD)0);

    INPUT_RECORD irRead = {0};
    nRead = (DWORD)-1;
    VERIFY_WIN32_BOOL_SUCCEEDED(ReadConsoleInput(hConsoleInput, &irRead, 0, &nRead));
    VERIFY_ARE_EQUAL(nRead, (DWORD)0);

    VERIFY_WIN32_BOOL_SUCCEEDED(FlushConsoleInputBuffer(hConsoleInput));
}

void InputTests::TestWriteConsoleInvalid()
{
    DWORD nWrite = (DWORD)-1;
    VERIFY_WIN32_BOOL_FAILED(WriteConsoleInput(0, nullptr, 0, &nWrite));
    VERIFY_ARE_EQUAL(nWrite, (DWORD)0);

    // weird: WriteConsoleInput with INVALID_HANDLE_VALUE writes garbage to lpNumberOfEventsWritten, whereas
    // [Read|Peek]ConsoleInput don't. This is a legacy behavior that we don't want to change.
    nWrite = (DWORD)-1;
    VERIFY_WIN32_BOOL_FAILED(WriteConsoleInput(INVALID_HANDLE_VALUE, nullptr, 0, &nWrite));

    HANDLE hConsoleInput = GetStdInputHandle();

    nWrite = (DWORD)-1;
    VERIFY_WIN32_BOOL_SUCCEEDED(WriteConsoleInput(hConsoleInput, nullptr, 0, &nWrite));
    VERIFY_ARE_EQUAL(nWrite, (DWORD)0);

    nWrite = (DWORD)-1;
    INPUT_RECORD irWrite = {0};
    VERIFY_WIN32_BOOL_SUCCEEDED(WriteConsoleInput(hConsoleInput, &irWrite, 0, &nWrite));
    VERIFY_ARE_EQUAL(nWrite, (DWORD)0);
}

void TestMouseWheelReadConsoleInputHelper(_In_ UINT const msg, _In_ DWORD const dwEventFlagsExpected, _In_ DWORD const dwConsoleMode)
{
    HWND const hwnd = GetConsoleWindow();
    VERIFY_IS_NOT_NULL(hwnd, L"Get console window handle to inject wheel messages.");

    HANDLE const hConsoleInput = GetStdInputHandle();
    VERIFY_WIN32_BOOL_SUCCEEDED(SetConsoleMode(hConsoleInput, dwConsoleMode), L"Apply the requested console mode");

    // We don't generate mouse console event in QuickEditMode or if MouseInput is not enabled
    DWORD dwExpectedEvents = 1;
    if (dwConsoleMode & ENABLE_QUICK_EDIT_MODE || !(dwConsoleMode & ENABLE_MOUSE_INPUT))
    {
        Log::Comment(L"QuickEditMode is set or MouseInput is not set, not expecting events");
        dwExpectedEvents = 0;
    }

    VERIFY_WIN32_BOOL_SUCCEEDED(FlushConsoleInputBuffer(hConsoleInput), L"Flush input queue to make sure no one else is in the way.");

    // WM_MOUSEWHEEL params
    // https://msdn.microsoft.com/en-us/library/windows/desktop/ms645617(v=vs.85).aspx

    // WPARAM is HIWORD the wheel delta and LOWORD the keystate (keys pressed with it)
    // We want no keys pressed in the loword (0) and we want one tick of the wheel in the high word.
    WPARAM wParam = 0;
    short sKeyState = 0;
    short sWheelDelta = -WHEEL_DELTA; // scroll down is negative, up is positive.
    wParam = ((sWheelDelta << 16) | sKeyState) & 0xFFFFFFFF; // we only use the lower 32-bits (in case of 64-bit system)

                                                             // LPARAM is positioning information. We don't care so we'll leave it 0x0
    LPARAM lParam = 0;

    Log::Comment(L"Send scroll down message into console window queue.");
    SendMessageW(hwnd, msg, wParam, lParam);

    Sleep(250); // give message time to sink in

    DWORD dwAvailable = 0;
    VERIFY_WIN32_BOOL_SUCCEEDED(GetNumberOfConsoleInputEvents(hConsoleInput, &dwAvailable), L"Retrieve number of events in queue.");
    VERIFY_ARE_EQUAL(dwExpectedEvents, dwAvailable,
        NoThrowString().Format(L"We expected %i event from our scroll message.", dwExpectedEvents));

    INPUT_RECORD ir;
    DWORD dwRead = 0;
    if (dwExpectedEvents == 1)
    {
        VERIFY_WIN32_BOOL_SUCCEEDED(ReadConsoleInputW(hConsoleInput, &ir, 1, &dwRead), L"Read the event out.");
        VERIFY_ARE_EQUAL(1u, dwRead);

        Log::Comment(L"Verify the event is what we expected. We only verify the fields relevant to this test.");
        VERIFY_ARE_EQUAL(MOUSE_EVENT, ir.EventType);
        VERIFY_ARE_EQUAL((DWORD)wParam, ir.Event.MouseEvent.dwButtonState); // hard cast OK. only using lower 32-bits (see above)
                                                                            // Don't care about ctrl key state. Can be messed with by caps lock/numlock state. Not checking this.
        VERIFY_ARE_EQUAL(dwEventFlagsExpected, ir.Event.MouseEvent.dwEventFlags);
        // Don't care about mouse position for ensuring scroll message went through.
    }
}

void InputTests::TestMouseWheelReadConsoleMouseInput()
{
    const DWORD dwInputMode = ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT | ENABLE_MOUSE_INPUT | ENABLE_EXTENDED_FLAGS;
    TestMouseWheelReadConsoleInputHelper(WM_MOUSEWHEEL, MOUSE_WHEELED, dwInputMode);
}

void InputTests::TestMouseHorizWheelReadConsoleMouseInput()
{
    const DWORD dwInputMode = ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT | ENABLE_MOUSE_INPUT | ENABLE_EXTENDED_FLAGS;
    TestMouseWheelReadConsoleInputHelper(WM_MOUSEHWHEEL, MOUSE_HWHEELED, dwInputMode);
}

void InputTests::TestMouseWheelReadConsoleNoMouseInput()
{
    const DWORD dwInputMode = ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT | ENABLE_EXTENDED_FLAGS;
    TestMouseWheelReadConsoleInputHelper(WM_MOUSEWHEEL, MOUSE_WHEELED, dwInputMode);
}

void InputTests::TestMouseHorizWheelReadConsoleNoMouseInput()
{
    const DWORD dwInputMode = ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT | ENABLE_EXTENDED_FLAGS;
    TestMouseWheelReadConsoleInputHelper(WM_MOUSEHWHEEL, MOUSE_HWHEELED, dwInputMode);
}

void InputTests::TestMouseWheelReadConsoleInputQuickEdit()
{
    const DWORD dwInputMode = ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT | ENABLE_MOUSE_INPUT | ENABLE_EXTENDED_FLAGS | ENABLE_INSERT_MODE | ENABLE_QUICK_EDIT_MODE;
    TestMouseWheelReadConsoleInputHelper(WM_MOUSEWHEEL, MOUSE_WHEELED, dwInputMode);
}

void InputTests::TestMouseHorizWheelReadConsoleInputQuickEdit()
{
    const DWORD dwInputMode = ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT | ENABLE_MOUSE_INPUT | ENABLE_EXTENDED_FLAGS | ENABLE_INSERT_MODE | ENABLE_QUICK_EDIT_MODE;
    TestMouseWheelReadConsoleInputHelper(WM_MOUSEHWHEEL, MOUSE_HWHEELED, dwInputMode);
}
