/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"
#include "WexTestClass.h"
#include "..\..\inc\consoletaeftemplates.hpp"

using namespace WEX::Logging;

class InputBufferTests
{
    TEST_CLASS(InputBufferTests);

    INPUT_RECORD MakeKeyEvent(BOOL bKeyDown,
                              WORD wRepeatCount,
                              WORD wVirtualKeyCode,
                              WORD wVirtualScanCode,
                              WCHAR UnicodeChar,
                              DWORD dwControlKeyState)
    {
        INPUT_RECORD retval;
        retval.EventType = KEY_EVENT;
        retval.Event.KeyEvent.bKeyDown = bKeyDown;
        retval.Event.KeyEvent.wRepeatCount = wRepeatCount;
        retval.Event.KeyEvent.wVirtualKeyCode = wVirtualKeyCode;
        retval.Event.KeyEvent.wVirtualScanCode = wVirtualScanCode;
        retval.Event.KeyEvent.uChar.UnicodeChar = UnicodeChar;
        retval.Event.KeyEvent.dwControlKeyState = dwControlKeyState;
        return retval;
    }

    // this needs to be done because some of the input buffer functions
    // refer to the global input buffer rather than their own
    // instance. This should be removed once these functions are fixed.
    TEST_CLASS_SETUP(ClassSetup)
    {
        if (g_ciConsoleInformation.pInputBuffer == nullptr)
        {
            g_ciConsoleInformation.pInputBuffer = new INPUT_INFORMATION(0);
        }
        return true;
    }

    TEST_CLASS_CLEANUP(ClassCleanup)
    {
        if (g_ciConsoleInformation.pInputBuffer != nullptr)
        {
            delete g_ciConsoleInformation.pInputBuffer;
            g_ciConsoleInformation.pInputBuffer = nullptr;
        }
        return true;
    }

    TEST_METHOD_CLEANUP(MethodCleanup)
    {
        g_ciConsoleInformation.pInputBuffer->FlushInputBuffer();
        return true;
    }

    TEST_METHOD(CanGetNumberOfReadyEvents)
    {
        INPUT_INFORMATION& inputBuffer = *g_ciConsoleInformation.pInputBuffer;
        INPUT_RECORD record = MakeKeyEvent(true, 1, 'a', 0, 'a', 0);
        inputBuffer.WriteInputBuffer(&record, 1);
        ULONG outNum;
        inputBuffer.GetNumberOfReadyEvents(&outNum);
        VERIFY_ARE_EQUAL(outNum, 1);
        // add another event, check again
        INPUT_RECORD record2;
        record2.EventType = MENU_EVENT;
        inputBuffer.WriteInputBuffer(&record2, 1);
        inputBuffer.GetNumberOfReadyEvents(&outNum);
        VERIFY_ARE_EQUAL(outNum, 2);
    }

    TEST_METHOD(CanInsertIntoInputBufferIndividually)
    {
        INPUT_INFORMATION& inputBuffer = *g_ciConsoleInformation.pInputBuffer;
        const size_t inputRecordsInserted = 12;
        for (size_t i = 0; i < inputRecordsInserted; ++i)
        {
            INPUT_RECORD record;
            record.EventType = MENU_EVENT;
            inputBuffer.WriteInputBuffer(&record, 1);
        }
        ULONG outNum;
        inputBuffer.GetNumberOfReadyEvents(&outNum);
        VERIFY_ARE_EQUAL(outNum, inputRecordsInserted);
    }

    TEST_METHOD(CanBulkInsertIntoInputBuffer)
    {
        INPUT_INFORMATION& inputBuffer = *g_ciConsoleInformation.pInputBuffer;
        const size_t inputRecordsInserted = 12;
        INPUT_RECORD records[inputRecordsInserted] = { 0 };
        for (size_t i = 0; i < inputRecordsInserted; ++i)
        {
            records[i].EventType = MENU_EVENT;
        }
        inputBuffer.WriteInputBuffer(records, inputRecordsInserted);
        ULONG outNum;
        inputBuffer.GetNumberOfReadyEvents(&outNum);
        VERIFY_ARE_EQUAL(outNum, inputRecordsInserted);
    }

    TEST_METHOD(InputBufferCoalescesMouseEvents)
    {
        INPUT_INFORMATION& inputBuffer = *g_ciConsoleInformation.pInputBuffer;
        const size_t inputRecordsInserted = 12;

        INPUT_RECORD mouseRecord;
        mouseRecord.EventType = MOUSE_EVENT;
        mouseRecord.Event.MouseEvent.dwEventFlags = MOUSE_MOVED;

        // add a bunch of mouse event records
        for (size_t i = 0; i < inputRecordsInserted; ++i)
        {
            inputBuffer.WriteInputBuffer(&mouseRecord, 1);
        }

        // check that they coalesced
        ULONG outNum;
        inputBuffer.GetNumberOfReadyEvents(&outNum);
        VERIFY_ARE_EQUAL(outNum, 1);

        // add a key event and another mouse event to make sure that
        // an event between two mouse events stopped the coalescing.
        INPUT_RECORD keyRecord;
        keyRecord.EventType = KEY_EVENT;
        inputBuffer.WriteInputBuffer(&keyRecord, 1);
        inputBuffer.WriteInputBuffer(&mouseRecord, 1);

        // verify
        inputBuffer.GetNumberOfReadyEvents(&outNum);
        VERIFY_ARE_EQUAL(outNum, 3);
    }

    TEST_METHOD(InputBufferDoesNotCoalesceBulkMouseEvents)
    {
        Log::Comment(L"The input buffer should not coalesce mouse events if more than one event is sent at a time");

        INPUT_INFORMATION& inputBuffer = *g_ciConsoleInformation.pInputBuffer;
        const size_t inputRecordsInserted = 12;
        INPUT_RECORD mouseRecords[inputRecordsInserted];

        for (size_t i = 0; i < inputRecordsInserted; ++i)
        {
            mouseRecords[i].EventType = MOUSE_EVENT;
            mouseRecords[i].Event.MouseEvent.dwEventFlags = MOUSE_MOVED;
        }
        inputBuffer.FlushInputBuffer();
        // send one mouse event to possibly coalesce into later
        inputBuffer.WriteInputBuffer(mouseRecords, 1);
        // write the others in bulk
        inputBuffer.WriteInputBuffer(mouseRecords, inputRecordsInserted);
        // no events should have been coalesced
        ULONG outNum;
        inputBuffer.GetNumberOfReadyEvents(&outNum);
        VERIFY_ARE_EQUAL(inputRecordsInserted + 1, outNum);
    }

    TEST_METHOD(InputBufferCoalescesKeyEvents)
    {
        Log::Comment(L"The input buffer should coalesce identical key events if they are send one at a time");
        INPUT_INFORMATION& inputBuffer = *g_ciConsoleInformation.pInputBuffer;
        const size_t inputRecordsInserted = 12;
        INPUT_RECORD record = MakeKeyEvent(true, 1, 'a', 0, L'a', 0);

        // send a bunch of identical events
        inputBuffer.FlushInputBuffer();
        for (size_t i = 0; i < inputRecordsInserted; ++i)
        {
            inputBuffer.WriteInputBuffer(&record, 1);
        }

        // all events should have been coalesced into one
        ULONG outNum;
        inputBuffer.GetNumberOfReadyEvents(&outNum);
        VERIFY_ARE_EQUAL(1, outNum);

        // the single event should have a repeat count for each
        // coalesced event
        INPUT_RECORD outRecord;
        DWORD length = 1;
        inputBuffer.ReadInputBuffer(&outRecord,
                                    &length,
                                    true,
                                    false,
                                    false,
                                    nullptr,
                                    nullptr,
                                    nullptr,
                                    nullptr,
                                    0,
                                    false,
                                    false);
        VERIFY_ARE_EQUAL(outRecord.Event.KeyEvent.wRepeatCount, inputRecordsInserted);
    }

    TEST_METHOD(InputBufferDoesNotCoalesceBulkKeyEvents)
    {
        Log::Comment(L"The input buffer should not coalesce key events if more than one event is sent at a time");

        INPUT_INFORMATION& inputBuffer = *g_ciConsoleInformation.pInputBuffer;
        const size_t inputRecordsInserted = 12;
        INPUT_RECORD keyRecords[inputRecordsInserted];

        for (size_t i = 0; i < inputRecordsInserted; ++i)
        {
            keyRecords[i] = MakeKeyEvent(true, 1, 'a', 0, L'a', 0);
        }
        inputBuffer.FlushInputBuffer();
        // send one key event to possibly coalesce into later
        inputBuffer.WriteInputBuffer(keyRecords, 1);
        // write the others in bulk
        inputBuffer.WriteInputBuffer(keyRecords, inputRecordsInserted);
        // no events should have been coalesced
        ULONG outNum;
        inputBuffer.GetNumberOfReadyEvents(&outNum);
        VERIFY_ARE_EQUAL(inputRecordsInserted + 1, outNum);
    }

    TEST_METHOD(InputBufferDoesNotCoalesceFullWidthChars)
    {
        INPUT_INFORMATION& inputBuffer = *g_ciConsoleInformation.pInputBuffer;
        const size_t inputRecordsInserted = 12;
        WCHAR hiraganaA = 0x3042; // U+3042 hiragana A
        INPUT_RECORD record = MakeKeyEvent(true, 1, hiraganaA, 0, hiraganaA, 0);

        // send a bunch of identical events
        inputBuffer.FlushInputBuffer();
        for (size_t i = 0; i < inputRecordsInserted; ++i)
        {
            inputBuffer.WriteInputBuffer(&record, 1);
        }

        // The events shouldn't be coalesced
        ULONG outNum;
        inputBuffer.GetNumberOfReadyEvents(&outNum);
        VERIFY_ARE_EQUAL(inputRecordsInserted, outNum);
    }

    TEST_METHOD(CanFlushAllOutput)
    {
        INPUT_INFORMATION& inputBuffer = *g_ciConsoleInformation.pInputBuffer;
        const size_t inputRecordsInserted = 12;
        INPUT_RECORD records[inputRecordsInserted];

        // put some events in the buffer so we can remove them
        for (size_t i = 0; i < inputRecordsInserted; ++i)
        {
            records[i].EventType = MENU_EVENT;
        }
        inputBuffer.WriteInputBuffer(records, inputRecordsInserted);
        ULONG outNum;
        inputBuffer.GetNumberOfReadyEvents(&outNum);
        VERIFY_ARE_EQUAL(outNum, inputRecordsInserted);

        // remove them
        inputBuffer.FlushInputBuffer();
        inputBuffer.GetNumberOfReadyEvents(&outNum);
        VERIFY_ARE_EQUAL(outNum, 0);
    }

    TEST_METHOD(CanFlushAllButKeys)
    {
        INPUT_INFORMATION& inputBuffer = *g_ciConsoleInformation.pInputBuffer;
        const size_t inputRecordsInserted = 12;
        INPUT_RECORD records[inputRecordsInserted];

        // create alternating mouse and key events
        for (size_t i = 0; i < inputRecordsInserted; ++i)
        {
            records[i].EventType = (i % 2 == 0) ? MENU_EVENT : KEY_EVENT;
        }
        inputBuffer.WriteInputBuffer(records, inputRecordsInserted);
        ULONG outNum;
        inputBuffer.GetNumberOfReadyEvents(&outNum);
        VERIFY_ARE_EQUAL(outNum, inputRecordsInserted);

        // remove them
        inputBuffer.FlushAllButKeys();
        inputBuffer.GetNumberOfReadyEvents(&outNum);
        VERIFY_ARE_EQUAL(outNum, inputRecordsInserted / 2);
    }

    TEST_METHOD(CanReadInput)
    {
        INPUT_INFORMATION& inputBuffer = *g_ciConsoleInformation.pInputBuffer;
        const unsigned int inputRecordsInserted = 12;
        INPUT_RECORD records[inputRecordsInserted];

        // write some input records
        for (unsigned int i = 0; i < inputRecordsInserted; ++i)
        {
            records[i] = MakeKeyEvent(TRUE, 1, 'A' + i, 0, 'A' + i, 0);
        }
        inputBuffer.WriteInputBuffer(records, inputRecordsInserted);

        // read them back out
        INPUT_RECORD outRecords[inputRecordsInserted];
        DWORD length;
        inputBuffer.ReadInputBuffer(outRecords,
                                    &length,
                                    false,
                                    false,
                                    false,
                                    nullptr,
                                    nullptr,
                                    nullptr,
                                    nullptr,
                                    0,
                                    false,
                                    false);
        ULONG outNum;
        inputBuffer.GetNumberOfReadyEvents(&outNum);
        VERIFY_ARE_EQUAL(outNum, 0);
        for (size_t i = 0; i < inputRecordsInserted; ++i)
        {
            VERIFY_ARE_EQUAL(records[i], outRecords[i]);
        }
    }

    TEST_METHOD(CanPeekAtEvents)
    {
        INPUT_INFORMATION& inputBuffer = *g_ciConsoleInformation.pInputBuffer;

        // add some events so that we have something to peek at
        const unsigned int inputRecordsInserted = 12;
        INPUT_RECORD records[inputRecordsInserted];
        for (unsigned int i = 0; i < inputRecordsInserted; ++i)
        {
            records[i] = MakeKeyEvent(TRUE, 1, 'A' + i, 0, 'A' + i, 0);
        }
        inputBuffer.WriteInputBuffer(records, inputRecordsInserted);

        // peek at events
        INPUT_RECORD outRecords[inputRecordsInserted];
        DWORD length = inputRecordsInserted;
        inputBuffer.ReadInputBuffer(outRecords,
                                    &length,
                                    true,
                                    false,
                                    false,
                                    nullptr,
                                    nullptr,
                                    nullptr,
                                    nullptr,
                                    0,
                                    false,
                                    false);
        VERIFY_ARE_EQUAL(length, inputRecordsInserted);
        ULONG outNum;
        inputBuffer.GetNumberOfReadyEvents(&outNum);
        VERIFY_ARE_EQUAL(outNum, inputRecordsInserted);
        for (unsigned int i = 0; i < inputRecordsInserted; ++i)
        {
            VERIFY_ARE_EQUAL(records[i], outRecords[i]);
        }
    }

    TEST_METHOD(EmptyingBufferDuringReadSetsResetWaitEvent)
    {
        Log::Comment(L"ResetWaitEvent should be true if a read to the buffer completely empties it");
        // write some input records to the buffer
        INPUT_INFORMATION& inputBuffer = *g_ciConsoleInformation.pInputBuffer;

        // add some events so that we have something to stick in front of
        const unsigned int inputRecordsInserted = 12;
        INPUT_RECORD records[inputRecordsInserted];
        for (unsigned int i = 0; i < inputRecordsInserted; ++i)
        {
            records[i] = MakeKeyEvent(TRUE, 1, 'A' + i, 0, 'A' + i, 0);
        }
        inputBuffer.WriteInputBuffer(records, inputRecordsInserted);

        // read one record, make sure ResetWaitEvent isn't set
        INPUT_RECORD outRecord;
        ULONG eventsRead = 0;
        BOOL resetWaitEvent = false;
        inputBuffer.ReadBuffer(&outRecord,
                               1,
                               &eventsRead,
                               false,
                               false,
                               &resetWaitEvent,
                               true);
        VERIFY_ARE_EQUAL(eventsRead, 1);
        VERIFY_IS_FALSE(!!resetWaitEvent);

        // read the rest, resetWaitEvent should be set to true
        INPUT_RECORD outBuffer[inputRecordsInserted - 1];
        inputBuffer.ReadBuffer(outBuffer,
                               inputRecordsInserted - 1,
                               &eventsRead,
                               false,
                               false,
                               &resetWaitEvent,
                               true);
        VERIFY_ARE_EQUAL(eventsRead, inputRecordsInserted - 1);
        VERIFY_IS_TRUE(!!resetWaitEvent);
    }

    TEST_METHOD(ReadingDbcsCharsPadsOutputArray)
    {
        Log::Comment(L"During a non-unicode read, the output array should have a blank entry at the end of the array for each dbcs key event");
        // write a mouse event, key event, dbcs key event, mouse event
        INPUT_INFORMATION& inputBuffer = *g_ciConsoleInformation.pInputBuffer;
        const unsigned int inputRecordsInserted = 4;
        INPUT_RECORD inRecords[inputRecordsInserted];
        inRecords[0].EventType = MOUSE_EVENT;
        inRecords[1] = MakeKeyEvent(TRUE, 1, 'A', 0, 'A', 0);
        inRecords[2] = MakeKeyEvent(TRUE, 1, 0x3042, 0, 0x3042, 0); // U+3042 hiragana A
        inRecords[3].EventType = MOUSE_EVENT;

        inputBuffer.FlushInputBuffer();
        inputBuffer.WriteInputBuffer(inRecords, inputRecordsInserted);

        // read them out non-unicode style and compare
        INPUT_RECORD outRecords[inputRecordsInserted] = { 0 };
        INPUT_RECORD emptyRecord = outRecords[0];
        ULONG eventsRead = 0;
        BOOL resetWaitEvent = false;
        inputBuffer.ReadBuffer(outRecords,
                               inputRecordsInserted,
                               &eventsRead,
                               false,
                               false,
                               &resetWaitEvent,
                               false);
        // the dbcs record should have counted for two elements int
        // the array, making it so that we get less events read than
        // the size of the array
        VERIFY_ARE_EQUAL(eventsRead, inputRecordsInserted - 1);
        for (size_t i = 0; i < eventsRead; ++i)
        {
            VERIFY_ARE_EQUAL(outRecords[i], inRecords[i]);
        }
        VERIFY_ARE_NOT_EQUAL(outRecords[3], inRecords[3]);
    }

    TEST_METHOD(CanPrependEvents)
    {
        INPUT_INFORMATION& inputBuffer = *g_ciConsoleInformation.pInputBuffer;

        // add some events so that we have something to stick in front of
        const unsigned int inputRecordsInserted = 12;
        INPUT_RECORD records[inputRecordsInserted];
        for (unsigned int i = 0; i < inputRecordsInserted; ++i)
        {
            records[i] = MakeKeyEvent(TRUE, 1, 'A' + i, 0, 'A' + i, 0);
        }
        inputBuffer.WriteInputBuffer(records, inputRecordsInserted);

        // prepend some other events
        INPUT_RECORD prependRecords[inputRecordsInserted];
        for (unsigned int i = 0; i < inputRecordsInserted; ++i)
        {
            prependRecords[i] = MakeKeyEvent(TRUE, 1, 'a' + i, 0, 'a' + i, 0);
        }
        DWORD prependCount = inputRecordsInserted;
        inputBuffer.PrependInputBuffer(prependRecords, &prependCount);
        VERIFY_ARE_EQUAL(prependCount, inputRecordsInserted);

        // grab the first set of events and ensure they match prependRecords
        INPUT_RECORD outRecords[inputRecordsInserted];
        DWORD length = inputRecordsInserted;
        inputBuffer.ReadInputBuffer(outRecords,
                                    &length,
                                    false,
                                    false,
                                    false,
                                    nullptr,
                                    nullptr,
                                    nullptr,
                                    nullptr,
                                    0,
                                    false,
                                    false);
        VERIFY_ARE_EQUAL(length, inputRecordsInserted);
        ULONG outNum;
        inputBuffer.GetNumberOfReadyEvents(&outNum);
        VERIFY_ARE_EQUAL(outNum, inputRecordsInserted);
        for (unsigned int i = 0; i < inputRecordsInserted; ++i)
        {
            VERIFY_ARE_EQUAL(prependRecords[i], outRecords[i]);
        }

        // verify the rest of the records
        inputBuffer.ReadInputBuffer(outRecords,
                                    &length,
                                    false,
                                    false,
                                    false,
                                    nullptr,
                                    nullptr,
                                    nullptr,
                                    nullptr,
                                    0,
                                    false,
                                    false);
        inputBuffer.GetNumberOfReadyEvents(&outNum);
        VERIFY_ARE_EQUAL(outNum, 0);
        VERIFY_ARE_EQUAL(length, inputRecordsInserted);
        for (unsigned int i = 0; i < inputRecordsInserted; ++i)
        {
            VERIFY_ARE_EQUAL(records[i], outRecords[i]);
        }
    }

    TEST_METHOD(CanReinitializeInputBuffer)
    {
        INPUT_INFORMATION& inputBuffer = *g_ciConsoleInformation.pInputBuffer;
        DWORD originalInputMode = inputBuffer.InputMode;

        // change the buffer's state a bit
        INPUT_RECORD record;
        record.EventType = MENU_EVENT;
        inputBuffer.WriteInputBuffer(&record, 1);
        ULONG outNum;
        inputBuffer.GetNumberOfReadyEvents(&outNum);
        VERIFY_ARE_EQUAL(outNum, 1);
        inputBuffer.InputMode = 0x0;
        inputBuffer.ReinitializeInputBuffer();

        // check that the changes were reverted
        VERIFY_ARE_EQUAL(originalInputMode, inputBuffer.InputMode);
        inputBuffer.GetNumberOfReadyEvents(&outNum);
        VERIFY_ARE_EQUAL(outNum, 0);
    }

    TEST_METHOD(CanChangeInputBufferSize)
    {
        INPUT_INFORMATION& inputBuffer = *g_ciConsoleInformation.pInputBuffer;
        // get original size
        ULONG_PTR originalSize = (inputBuffer.Last - inputBuffer.First) / sizeof(INPUT_RECORD);
        // change it
        inputBuffer.SetInputBufferSize(static_cast<ULONG>(originalSize) * 2);
        // make sure it's bigger now
        ULONG_PTR newSize = (inputBuffer.Last - inputBuffer.First) / sizeof(INPUT_RECORD);
        VERIFY_IS_LESS_THAN(originalSize, newSize);
    }

    TEST_METHOD(PreprocessInputRemovesPauseKeys)
    {
        INPUT_INFORMATION& inputBuffer = *g_ciConsoleInformation.pInputBuffer;
        INPUT_RECORD pauseRecord = MakeKeyEvent(true, 1, VK_PAUSE, 0, 0, 0);
        ULONG outNum;

        // make sure we aren't currently paused and have an empty buffer
        VERIFY_IS_FALSE(IsFlagSet(g_ciConsoleInformation.Flags, CONSOLE_OUTPUT_SUSPENDED));
        inputBuffer.GetNumberOfReadyEvents(&outNum);
        VERIFY_ARE_EQUAL(outNum, 0);

        inputBuffer.WriteInputBuffer(&pauseRecord, 1);

        // we should now be paused and the input record should be discarded
        VERIFY_IS_TRUE(IsFlagSet(g_ciConsoleInformation.Flags, CONSOLE_OUTPUT_SUSPENDED));
        inputBuffer.GetNumberOfReadyEvents(&outNum);
        VERIFY_ARE_EQUAL(outNum, 0);

        // the next key press should unpause us but be discarded
        INPUT_RECORD unpauseRecord = MakeKeyEvent(true, 1, 'a', 0, 'a', 0);
        inputBuffer.WriteInputBuffer(&unpauseRecord, 1);

        VERIFY_IS_FALSE(IsFlagSet(g_ciConsoleInformation.Flags, CONSOLE_OUTPUT_SUSPENDED));
        inputBuffer.GetNumberOfReadyEvents(&outNum);
        VERIFY_ARE_EQUAL(outNum, 0);
    }

    TEST_METHOD(SystemKeysDontUnpauseConsole)
    {
        INPUT_INFORMATION& inputBuffer = *g_ciConsoleInformation.pInputBuffer;
        INPUT_RECORD pauseRecord = MakeKeyEvent(true, 1, VK_PAUSE, 0, 0, 0);
        ULONG outNum;

        // make sure we aren't currently paused and have an empty buffer
        VERIFY_IS_FALSE(IsFlagSet(g_ciConsoleInformation.Flags, CONSOLE_OUTPUT_SUSPENDED));
        inputBuffer.GetNumberOfReadyEvents(&outNum);
        VERIFY_ARE_EQUAL(outNum, 0);

        // pause the screen
        inputBuffer.WriteInputBuffer(&pauseRecord, 1);

        // we should now be paused and the input record should be discarded
        VERIFY_IS_TRUE(IsFlagSet(g_ciConsoleInformation.Flags, CONSOLE_OUTPUT_SUSPENDED));
        inputBuffer.GetNumberOfReadyEvents(&outNum);
        VERIFY_ARE_EQUAL(outNum, 0);

        // sending a system key event should not stop the pause and
        // the record should be stored in the input buffer
        INPUT_RECORD systemRecord = MakeKeyEvent(true, 1, VK_CONTROL, 0, 0, 0);
        inputBuffer.WriteInputBuffer(&systemRecord, 1);

        VERIFY_IS_TRUE(IsFlagSet(g_ciConsoleInformation.Flags, CONSOLE_OUTPUT_SUSPENDED));
        inputBuffer.GetNumberOfReadyEvents(&outNum);

        INPUT_RECORD outRecords[2];
        DWORD length = 2;
        inputBuffer.ReadInputBuffer(outRecords,
                                    &length,
                                    true,
                                    false,
                                    false,
                                    nullptr,
                                    nullptr,
                                    nullptr,
                                    nullptr,
                                    0,
                                    false,
                                    false);
        VERIFY_ARE_EQUAL(outNum, 1);
    }

    TEST_METHOD(WritingToEmptyBufferSignalsWaitEvent)
    {
        INPUT_INFORMATION& inputBuffer = *g_ciConsoleInformation.pInputBuffer;
        INPUT_RECORD record = MakeKeyEvent(true, 1, VK_PAUSE, 0, 0, 0);
        ULONG eventsWritten;
        BOOL waitEvent = FALSE;
        inputBuffer.FlushInputBuffer();
        // write one event to an empty buffer
        inputBuffer.WriteBuffer(&record, 1, &eventsWritten, &waitEvent);
        VERIFY_IS_TRUE(!!waitEvent);
        // write another, it shouldn't signal this time
        INPUT_RECORD record2 = MakeKeyEvent(true, 1, 'b', 0, 'b', 0);
        // write another event to a non-empty buffer
        waitEvent = FALSE;
        inputBuffer.WriteBuffer(&record2, 1, &eventsWritten, &waitEvent);
        VERIFY_IS_FALSE(!!waitEvent);
    }

};