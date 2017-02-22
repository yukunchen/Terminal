/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"
#include "WexTestClass.h"
#include "..\..\inc\consoletaeftemplates.hpp"

#define VERIFY_SUCCESS_NTSTATUS(x) VERIFY_IS_TRUE(SUCCEEDED_NTSTATUS(x))

using namespace WEX::Logging;

class InputBufferTests
{
    TEST_CLASS(InputBufferTests);

    static const size_t RECORD_INSERT_COUNT = 12;

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

    TEST_METHOD(CanGetNumberOfReadyEvents)
    {
        InputBuffer inputBuffer{ 0 };
        INPUT_RECORD record = MakeKeyEvent(true, 1, 'a', 0, 'a', 0);
        VERIFY_IS_GREATER_THAN(inputBuffer.WriteInputBuffer(&record, 1), 0u);
        VERIFY_ARE_EQUAL(inputBuffer.GetNumberOfReadyEvents(), 1);
        // add another event, check again
        INPUT_RECORD record2;
        record2.EventType = MENU_EVENT;
        VERIFY_IS_GREATER_THAN(inputBuffer.WriteInputBuffer(&record2, 1), 0u);
        VERIFY_ARE_EQUAL(inputBuffer.GetNumberOfReadyEvents(), 2);
    }

    TEST_METHOD(CanInsertIntoInputBufferIndividually)
    {
        InputBuffer inputBuffer{ 0 };
        for (size_t i = 0; i < RECORD_INSERT_COUNT; ++i)
        {
            INPUT_RECORD record;
            record.EventType = MENU_EVENT;
            VERIFY_IS_GREATER_THAN(inputBuffer.WriteInputBuffer(&record, 1), 0u);
        }
    VERIFY_ARE_EQUAL(inputBuffer.GetNumberOfReadyEvents(), RECORD_INSERT_COUNT);
    }

    TEST_METHOD(CanBulkInsertIntoInputBuffer)
    {
        InputBuffer inputBuffer{ 0 };
        INPUT_RECORD records[RECORD_INSERT_COUNT] = { 0 };
        for (size_t i = 0; i < RECORD_INSERT_COUNT; ++i)
        {
            records[i].EventType = MENU_EVENT;
        }
        VERIFY_IS_GREATER_THAN(inputBuffer.WriteInputBuffer(records, RECORD_INSERT_COUNT), 0u);
        VERIFY_ARE_EQUAL(inputBuffer.GetNumberOfReadyEvents(), RECORD_INSERT_COUNT);
    }

    TEST_METHOD(InputBufferCoalescesMouseEvents)
    {
        InputBuffer inputBuffer{ 0 };

        INPUT_RECORD mouseRecord;
        mouseRecord.EventType = MOUSE_EVENT;
        mouseRecord.Event.MouseEvent.dwEventFlags = MOUSE_MOVED;

        // add a bunch of mouse event records
        for (size_t i = 0; i < RECORD_INSERT_COUNT; ++i)
        {
            mouseRecord.Event.MouseEvent.dwMousePosition.X = static_cast<SHORT>(i + 1);
            mouseRecord.Event.MouseEvent.dwMousePosition.Y = static_cast<SHORT>(i + 1) * 2;
            VERIFY_IS_GREATER_THAN(inputBuffer.WriteInputBuffer(&mouseRecord, 1), 0u);
        }

        // check that they coalesced
        VERIFY_ARE_EQUAL(inputBuffer.GetNumberOfReadyEvents(), 1);
        // check that the mouse position is being updated correctly
        INPUT_RECORD outRecord = inputBuffer._storage.front();
        VERIFY_ARE_EQUAL(outRecord.Event.MouseEvent.dwMousePosition.X, RECORD_INSERT_COUNT);
        VERIFY_ARE_EQUAL(outRecord.Event.MouseEvent.dwMousePosition.Y, RECORD_INSERT_COUNT * 2);

        // add a key event and another mouse event to make sure that
        // an event between two mouse events stopped the coalescing.
        INPUT_RECORD keyRecord;
        keyRecord.EventType = KEY_EVENT;
        VERIFY_IS_GREATER_THAN(inputBuffer.WriteInputBuffer(&keyRecord, 1), 0u);
        VERIFY_IS_GREATER_THAN(inputBuffer.WriteInputBuffer(&mouseRecord, 1), 0u);

        // verify
        VERIFY_ARE_EQUAL(inputBuffer.GetNumberOfReadyEvents(), 3);
    }

    TEST_METHOD(InputBufferDoesNotCoalesceBulkMouseEvents)
    {
        Log::Comment(L"The input buffer should not coalesce mouse events if more than one event is sent at a time");

        InputBuffer inputBuffer{ 0 };
        INPUT_RECORD mouseRecords[RECORD_INSERT_COUNT];

        for (size_t i = 0; i < RECORD_INSERT_COUNT; ++i)
        {
            mouseRecords[i].EventType = MOUSE_EVENT;
            mouseRecords[i].Event.MouseEvent.dwEventFlags = MOUSE_MOVED;
        }
        inputBuffer.FlushInputBuffer();
        // send one mouse event to possibly coalesce into later
        VERIFY_IS_GREATER_THAN(inputBuffer.WriteInputBuffer(mouseRecords, 1), 0u);
        // write the others in bulk
        VERIFY_IS_GREATER_THAN(inputBuffer.WriteInputBuffer(mouseRecords, RECORD_INSERT_COUNT), 0u);
        // no events should have been coalesced
        VERIFY_ARE_EQUAL(inputBuffer.GetNumberOfReadyEvents(), RECORD_INSERT_COUNT + 1);
    }

    TEST_METHOD(InputBufferCoalescesKeyEvents)
    {
        Log::Comment(L"The input buffer should coalesce identical key events if they are send one at a time");

        InputBuffer inputBuffer{ 0 };
        INPUT_RECORD record = MakeKeyEvent(true, 1, 'a', 0, L'a', 0);

        // send a bunch of identical events
        inputBuffer.FlushInputBuffer();
        for (size_t i = 0; i < RECORD_INSERT_COUNT; ++i)
        {
            VERIFY_IS_GREATER_THAN(inputBuffer.WriteInputBuffer(&record, 1), 0u);
        }

        // all events should have been coalesced into one
        VERIFY_ARE_EQUAL(inputBuffer.GetNumberOfReadyEvents(), 1);

        // the single event should have a repeat count for each
        // coalesced event
        INPUT_RECORD outRecord;
        DWORD length = 1;
        VERIFY_SUCCESS_NTSTATUS(inputBuffer.ReadInputBuffer(&outRecord,
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
                                                            false));
        VERIFY_ARE_EQUAL(outRecord.Event.KeyEvent.wRepeatCount, RECORD_INSERT_COUNT);
    }

    TEST_METHOD(InputBufferDoesNotCoalesceBulkKeyEvents)
    {
        Log::Comment(L"The input buffer should not coalesce key events if more than one event is sent at a time");

        InputBuffer inputBuffer{ 0 };
        INPUT_RECORD keyRecords[RECORD_INSERT_COUNT];

        for (size_t i = 0; i < RECORD_INSERT_COUNT; ++i)
        {
            keyRecords[i] = MakeKeyEvent(true, 1, 'a', 0, L'a', 0);
        }
        inputBuffer.FlushInputBuffer();
        // send one key event to possibly coalesce into later
        VERIFY_IS_GREATER_THAN(inputBuffer.WriteInputBuffer(keyRecords, 1), 0u);
        // write the others in bulk
        VERIFY_IS_GREATER_THAN(inputBuffer.WriteInputBuffer(keyRecords, RECORD_INSERT_COUNT), 0u);
        // no events should have been coalesced
        VERIFY_ARE_EQUAL(inputBuffer.GetNumberOfReadyEvents(), RECORD_INSERT_COUNT + 1);
    }

    TEST_METHOD(InputBufferDoesNotCoalesceFullWidthChars)
    {
        InputBuffer inputBuffer{ 0 };
        WCHAR hiraganaA = 0x3042; // U+3042 hiragana A
        INPUT_RECORD record = MakeKeyEvent(true, 1, hiraganaA, 0, hiraganaA, 0);

        // send a bunch of identical events
        inputBuffer.FlushInputBuffer();
        for (size_t i = 0; i < RECORD_INSERT_COUNT; ++i)
        {
            VERIFY_IS_GREATER_THAN(inputBuffer.WriteInputBuffer(&record, 1), 0u);
        }

        // The events shouldn't be coalesced
        VERIFY_ARE_EQUAL(inputBuffer.GetNumberOfReadyEvents(), RECORD_INSERT_COUNT);
    }

    TEST_METHOD(CanFlushAllOutput)
    {
        InputBuffer inputBuffer{ 0 };
        INPUT_RECORD records[RECORD_INSERT_COUNT];

        // put some events in the buffer so we can remove them
        for (size_t i = 0; i < RECORD_INSERT_COUNT; ++i)
        {
            records[i].EventType = MENU_EVENT;
        }
        VERIFY_IS_GREATER_THAN(inputBuffer.WriteInputBuffer(records, RECORD_INSERT_COUNT), 0u);
        VERIFY_ARE_EQUAL(inputBuffer.GetNumberOfReadyEvents(), RECORD_INSERT_COUNT);

        // remove them
        inputBuffer.FlushInputBuffer();
        VERIFY_ARE_EQUAL(inputBuffer.GetNumberOfReadyEvents(), 0);
    }

    TEST_METHOD(CanFlushAllButKeys)
    {
        InputBuffer inputBuffer{ 0 };
        INPUT_RECORD records[RECORD_INSERT_COUNT];

        // create alternating mouse and key events
        for (size_t i = 0; i < RECORD_INSERT_COUNT; ++i)
        {
            records[i].EventType = (i % 2 == 0) ? MENU_EVENT : KEY_EVENT;
        }
        VERIFY_IS_GREATER_THAN(inputBuffer.WriteInputBuffer(records, RECORD_INSERT_COUNT), 0u);
        VERIFY_ARE_EQUAL(inputBuffer.GetNumberOfReadyEvents(), RECORD_INSERT_COUNT);

        // remove them
        VERIFY_SUCCEEDED(inputBuffer.FlushAllButKeys());
        VERIFY_ARE_EQUAL(inputBuffer.GetNumberOfReadyEvents(), RECORD_INSERT_COUNT / 2);

        // make sure that the non key events were the ones removed
        INPUT_RECORD outRecords[RECORD_INSERT_COUNT / 2];
        DWORD length = RECORD_INSERT_COUNT / 2;
        VERIFY_SUCCESS_NTSTATUS(inputBuffer.ReadInputBuffer(outRecords,
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
                                                            false));
        for (size_t i = 0; i < length; ++i)
        {
            VERIFY_ARE_EQUAL(outRecords[i].EventType, KEY_EVENT);
        }
    }

    TEST_METHOD(CanReadInput)
    {
        InputBuffer inputBuffer{ 0 };
        INPUT_RECORD records[RECORD_INSERT_COUNT];

        // write some input records
        for (unsigned int i = 0; i < RECORD_INSERT_COUNT; ++i)
        {
            records[i] = MakeKeyEvent(TRUE, 1, 'A' + i, 0, 'A' + i, 0);
        }
        VERIFY_IS_GREATER_THAN(inputBuffer.WriteInputBuffer(records, RECORD_INSERT_COUNT), 0u);

        // read them back out
        INPUT_RECORD outRecords[RECORD_INSERT_COUNT];
        DWORD length = RECORD_INSERT_COUNT;
        VERIFY_SUCCESS_NTSTATUS(inputBuffer.ReadInputBuffer(outRecords,
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
                                                            false));
        VERIFY_ARE_EQUAL(inputBuffer.GetNumberOfReadyEvents(), 0);
        for (size_t i = 0; i < RECORD_INSERT_COUNT; ++i)
        {
            VERIFY_ARE_EQUAL(records[i], outRecords[i]);
        }
    }

    TEST_METHOD(CanPeekAtEvents)
    {
        InputBuffer inputBuffer{ 0 };

        // add some events so that we have something to peek at
        INPUT_RECORD records[RECORD_INSERT_COUNT];
        for (unsigned int i = 0; i < RECORD_INSERT_COUNT; ++i)
        {
            records[i] = MakeKeyEvent(TRUE, 1, 'A' + i, 0, 'A' + i, 0);
        }
        VERIFY_IS_GREATER_THAN(inputBuffer.WriteInputBuffer(records, RECORD_INSERT_COUNT), 0u);

        // peek at events
        INPUT_RECORD outRecords[RECORD_INSERT_COUNT];
        DWORD length = RECORD_INSERT_COUNT;
        VERIFY_SUCCESS_NTSTATUS(inputBuffer.ReadInputBuffer(outRecords,
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
                                                            false));
        VERIFY_ARE_EQUAL(length, RECORD_INSERT_COUNT);
        VERIFY_ARE_EQUAL(inputBuffer.GetNumberOfReadyEvents(), RECORD_INSERT_COUNT);
        for (unsigned int i = 0; i < RECORD_INSERT_COUNT; ++i)
        {
            VERIFY_ARE_EQUAL(records[i], outRecords[i]);
        }
    }

    TEST_METHOD(EmptyingBufferDuringReadSetsResetWaitEvent)
    {
        Log::Comment(L"ResetWaitEvent should be true if a read to the buffer completely empties it");

        InputBuffer inputBuffer{ 0 };

        // add some events so that we have something to stick in front of
        INPUT_RECORD records[RECORD_INSERT_COUNT];
        for (unsigned int i = 0; i < RECORD_INSERT_COUNT; ++i)
        {
            records[i] = MakeKeyEvent(TRUE, 1, 'A' + i, 0, 'A' + i, 0);
        }
        VERIFY_IS_GREATER_THAN(inputBuffer.WriteInputBuffer(records, RECORD_INSERT_COUNT), 0u);

        // read one record, make sure ResetWaitEvent isn't set
        INPUT_RECORD outRecord;
        ULONG eventsRead = 0;
        BOOL resetWaitEvent = false;
        VERIFY_SUCCESS_NTSTATUS(inputBuffer._ReadBuffer(&outRecord,
                                                        1,
                                                        &eventsRead,
                                                        false,
                                                        false,
                                                        &resetWaitEvent,
                                                        true));
        VERIFY_ARE_EQUAL(eventsRead, 1);
        VERIFY_IS_FALSE(!!resetWaitEvent);

        // read the rest, resetWaitEvent should be set to true
        INPUT_RECORD outBuffer[RECORD_INSERT_COUNT - 1];
        VERIFY_SUCCESS_NTSTATUS(inputBuffer._ReadBuffer(outBuffer,
                                                        RECORD_INSERT_COUNT - 1,
                                                        &eventsRead,
                                                        false,
                                                        false,
                                                        &resetWaitEvent,
                                                        true));
        VERIFY_ARE_EQUAL(eventsRead, RECORD_INSERT_COUNT - 1);
        VERIFY_IS_TRUE(!!resetWaitEvent);
    }

    TEST_METHOD(ReadingDbcsCharsPadsOutputArray)
    {
        Log::Comment(L"During a non-unicode read, the output array should have a blank entry at the end of the array for each dbcs key event");

        // write a mouse event, key event, dbcs key event, mouse event
        InputBuffer inputBuffer{ 0 };
        const unsigned int RECORD_INSERT_COUNT = 4;
        INPUT_RECORD inRecords[RECORD_INSERT_COUNT];
        inRecords[0].EventType = MOUSE_EVENT;
        inRecords[1] = MakeKeyEvent(TRUE, 1, 'A', 0, 'A', 0);
        inRecords[2] = MakeKeyEvent(TRUE, 1, 0x3042, 0, 0x3042, 0); // U+3042 hiragana A
        inRecords[3].EventType = MOUSE_EVENT;

        inputBuffer.FlushInputBuffer();
        VERIFY_IS_GREATER_THAN(inputBuffer.WriteInputBuffer(inRecords, RECORD_INSERT_COUNT), 0u);

        // read them out non-unicode style and compare
        INPUT_RECORD outRecords[RECORD_INSERT_COUNT] = { 0 };
        INPUT_RECORD emptyRecord = outRecords[0];
        ULONG eventsRead = 0;
        BOOL resetWaitEvent = false;
        VERIFY_SUCCESS_NTSTATUS(inputBuffer._ReadBuffer(outRecords,
                                                        RECORD_INSERT_COUNT,
                                                        &eventsRead,
                                                        false,
                                                        false,
                                                        &resetWaitEvent,
                                                        false));
        // the dbcs record should have counted for two elements int
        // the array, making it so that we get less events read than
        // the size of the array
        VERIFY_ARE_EQUAL(eventsRead, RECORD_INSERT_COUNT - 1);
        for (size_t i = 0; i < eventsRead; ++i)
        {
            VERIFY_ARE_EQUAL(outRecords[i], inRecords[i]);
        }
        VERIFY_ARE_NOT_EQUAL(outRecords[3], inRecords[3]);
    }

    TEST_METHOD(CanPrependEvents)
    {
        InputBuffer inputBuffer{ 0 };

        // add some events so that we have something to stick in front of
        INPUT_RECORD records[RECORD_INSERT_COUNT];
        for (unsigned int i = 0; i < RECORD_INSERT_COUNT; ++i)
        {
            records[i] = MakeKeyEvent(TRUE, 1, 'A' + i, 0, 'A' + i, 0);
        }
        VERIFY_IS_GREATER_THAN(inputBuffer.WriteInputBuffer(records, RECORD_INSERT_COUNT), 0u);

        // prepend some other events
        INPUT_RECORD prependRecords[RECORD_INSERT_COUNT];
        for (unsigned int i = 0; i < RECORD_INSERT_COUNT; ++i)
        {
            prependRecords[i] = MakeKeyEvent(TRUE, 1, 'a' + i, 0, 'a' + i, 0);
        }
        DWORD prependCount = RECORD_INSERT_COUNT;
        VERIFY_SUCCESS_NTSTATUS(inputBuffer.PrependInputBuffer(prependRecords, &prependCount));
        VERIFY_ARE_EQUAL(prependCount, RECORD_INSERT_COUNT);

        // grab the first set of events and ensure they match prependRecords
        INPUT_RECORD outRecords[RECORD_INSERT_COUNT];
        DWORD length = RECORD_INSERT_COUNT;
        VERIFY_SUCCESS_NTSTATUS(inputBuffer.ReadInputBuffer(outRecords,
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
                                                            false));
        VERIFY_ARE_EQUAL(length, RECORD_INSERT_COUNT);
        VERIFY_ARE_EQUAL(inputBuffer.GetNumberOfReadyEvents(), RECORD_INSERT_COUNT);
        for (unsigned int i = 0; i < RECORD_INSERT_COUNT; ++i)
        {
            VERIFY_ARE_EQUAL(prependRecords[i], outRecords[i]);
        }

        // verify the rest of the records
        VERIFY_SUCCESS_NTSTATUS(inputBuffer.ReadInputBuffer(outRecords,
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
                                                            false));
        VERIFY_ARE_EQUAL(inputBuffer.GetNumberOfReadyEvents(), 0);
        VERIFY_ARE_EQUAL(length, RECORD_INSERT_COUNT);
        for (unsigned int i = 0; i < RECORD_INSERT_COUNT; ++i)
        {
            VERIFY_ARE_EQUAL(records[i], outRecords[i]);
        }
    }

    TEST_METHOD(CanReinitializeInputBuffer)
    {
        InputBuffer inputBuffer{ 0 };
        DWORD originalInputMode = inputBuffer.InputMode;

        // change the buffer's state a bit
        INPUT_RECORD record;
        record.EventType = MENU_EVENT;
        VERIFY_IS_GREATER_THAN(inputBuffer.WriteInputBuffer(&record, 1), 0u);
        VERIFY_ARE_EQUAL(inputBuffer.GetNumberOfReadyEvents(), 1);
        inputBuffer.InputMode = 0x0;
        inputBuffer.ReinitializeInputBuffer();

        // check that the changes were reverted
        VERIFY_ARE_EQUAL(originalInputMode, inputBuffer.InputMode);
        VERIFY_ARE_EQUAL(inputBuffer.GetNumberOfReadyEvents(), 0);
    }

    TEST_METHOD(PreprocessInputRemovesPauseKeys)
    {
        InputBuffer inputBuffer{ 0 };
        INPUT_RECORD pauseRecord = MakeKeyEvent(true, 1, VK_PAUSE, 0, 0, 0);

        // make sure we aren't currently paused and have an empty buffer
        VERIFY_IS_FALSE(IsFlagSet(g_ciConsoleInformation.Flags, CONSOLE_OUTPUT_SUSPENDED));
        VERIFY_ARE_EQUAL(inputBuffer.GetNumberOfReadyEvents(), 0);

        VERIFY_ARE_EQUAL(inputBuffer.WriteInputBuffer(&pauseRecord, 1), 0u);

        // we should now be paused and the input record should be discarded
        VERIFY_IS_TRUE(IsFlagSet(g_ciConsoleInformation.Flags, CONSOLE_OUTPUT_SUSPENDED));
        VERIFY_ARE_EQUAL(inputBuffer.GetNumberOfReadyEvents(), 0);

        // the next key press should unpause us but be discarded
        INPUT_RECORD unpauseRecord = MakeKeyEvent(true, 1, 'a', 0, 'a', 0);
        VERIFY_ARE_EQUAL(inputBuffer.WriteInputBuffer(&unpauseRecord, 1), 0u);

        VERIFY_IS_FALSE(IsFlagSet(g_ciConsoleInformation.Flags, CONSOLE_OUTPUT_SUSPENDED));
        VERIFY_ARE_EQUAL(inputBuffer.GetNumberOfReadyEvents(), 0);
    }

    TEST_METHOD(SystemKeysDontUnpauseConsole)
    {
        InputBuffer inputBuffer{ 0 };
        INPUT_RECORD pauseRecord = MakeKeyEvent(true, 1, VK_PAUSE, 0, 0, 0);

        // make sure we aren't currently paused and have an empty buffer
        VERIFY_IS_FALSE(IsFlagSet(g_ciConsoleInformation.Flags, CONSOLE_OUTPUT_SUSPENDED));
        VERIFY_ARE_EQUAL(inputBuffer.GetNumberOfReadyEvents(), 0);

        // pause the screen
        VERIFY_ARE_EQUAL(inputBuffer.WriteInputBuffer(&pauseRecord, 1), 0u);

        // we should now be paused and the input record should be discarded
        VERIFY_IS_TRUE(IsFlagSet(g_ciConsoleInformation.Flags, CONSOLE_OUTPUT_SUSPENDED));
        VERIFY_ARE_EQUAL(inputBuffer.GetNumberOfReadyEvents(), 0);

        // sending a system key event should not stop the pause and
        // the record should be stored in the input buffer
        INPUT_RECORD systemRecord = MakeKeyEvent(true, 1, VK_CONTROL, 0, 0, 0);
        VERIFY_IS_GREATER_THAN(inputBuffer.WriteInputBuffer(&systemRecord, 1), 0u);

        VERIFY_IS_TRUE(IsFlagSet(g_ciConsoleInformation.Flags, CONSOLE_OUTPUT_SUSPENDED));
        VERIFY_ARE_EQUAL(inputBuffer.GetNumberOfReadyEvents(), 1u);

        INPUT_RECORD outRecords[2];
        DWORD length = 2;
        VERIFY_SUCCESS_NTSTATUS(inputBuffer.ReadInputBuffer(outRecords,
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
                                                            false));
    }

    TEST_METHOD(WritingToEmptyBufferSignalsWaitEvent)
    {
        InputBuffer inputBuffer{ 0 };
        INPUT_RECORD record = MakeKeyEvent(true, 1, VK_PAUSE, 0, 0, 0);
        size_t eventsWritten;
        bool waitEvent = false;
        inputBuffer.FlushInputBuffer();
        // write one event to an empty buffer
        std::deque<INPUT_RECORD> storage = { record };
        VERIFY_SUCCEEDED(inputBuffer._WriteBuffer(storage, eventsWritten, waitEvent));
        VERIFY_IS_TRUE(waitEvent);
        // write another, it shouldn't signal this time
        INPUT_RECORD record2 = MakeKeyEvent(true, 1, 'b', 0, 'b', 0);
        // write another event to a non-empty buffer
        waitEvent = false;
        storage.push_back(record2);
        VERIFY_SUCCEEDED(inputBuffer._WriteBuffer(storage, eventsWritten, waitEvent));

        VERIFY_IS_FALSE(waitEvent);
    }

};