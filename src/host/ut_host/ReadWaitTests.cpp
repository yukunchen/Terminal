/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"
#include "WexTestClass.h"
#include "..\..\inc\consoletaeftemplates.hpp"

#include "misc.h"
#include "dbcs.h"

using namespace WEX::Logging;

class InputRecordConversionTests
{
    TEST_CLASS(InputRecordConversionTests);

    static const size_t INPUT_RECORD_COUNT = 10;
    UINT savedCodepage = 0;

    TEST_CLASS_SETUP(ClassSetup)
    {
        savedCodepage = g_ciConsoleInformation.CP;
        g_ciConsoleInformation.CP = CP_JAPANESE;
        VERIFY_IS_TRUE(!!GetCPInfo(g_ciConsoleInformation.CP, &g_ciConsoleInformation.CPInfo));
        return true;
    }

    TEST_CLASS_CLEANUP(ClassCleanup)
    {
        g_ciConsoleInformation.CP = savedCodepage;
        VERIFY_IS_TRUE(!!GetCPInfo(g_ciConsoleInformation.CP, &g_ciConsoleInformation.CPInfo));
        return true;
    }

    TEST_METHOD(TranslateInputToOemLeavesNonKeyEventsAlone)
    {
        Log::Comment(L"nothing should happen to input records that aren't key events");
        INPUT_RECORD inRecords[INPUT_RECORD_COUNT];
        for (size_t i = 0; i < INPUT_RECORD_COUNT; ++i)
        {
            inRecords[i].EventType = MOUSE_EVENT;
            inRecords[i].Event.MouseEvent.dwMousePosition.X = static_cast<SHORT>(i);
            inRecords[i].Event.MouseEvent.dwMousePosition.Y = static_cast<SHORT>(i * 2);
        }
        // need to make a copy of the input data since it can be
        // modified in place
        INPUT_RECORD referenceInRecords[INPUT_RECORD_COUNT];
        std::copy(inRecords, inRecords + INPUT_RECORD_COUNT, referenceInRecords);
        ULONG outNum = TranslateInputToOem(inRecords, INPUT_RECORD_COUNT, INPUT_RECORD_COUNT, nullptr);
        VERIFY_ARE_EQUAL(outNum, INPUT_RECORD_COUNT);
        for (size_t i = 0; i < INPUT_RECORD_COUNT; ++i)
        {
            VERIFY_ARE_EQUAL(inRecords[i], referenceInRecords[i]);
        }
    }

    TEST_METHOD(TranslateInputToOemLeavesNonDbcsCharsAlone)
    {
        Log::Comment(L"non-dbcs chars shouldn't be split");
        INPUT_RECORD inRecords[INPUT_RECORD_COUNT];
        for (size_t i = 0; i < INPUT_RECORD_COUNT; ++i)
        {
            inRecords[i].EventType = KEY_EVENT;
            inRecords[i].Event.KeyEvent.uChar.UnicodeChar = static_cast<wchar_t>(L'a' + i);
        }
        // need to make a copy of the input data since it can be
        // modified in place
        INPUT_RECORD referenceInRecords[INPUT_RECORD_COUNT];
        std::copy(inRecords, inRecords + INPUT_RECORD_COUNT, referenceInRecords);
        ULONG outNum = TranslateInputToOem(inRecords, INPUT_RECORD_COUNT * 2, INPUT_RECORD_COUNT, nullptr);
        VERIFY_ARE_EQUAL(outNum, INPUT_RECORD_COUNT);
        for (size_t i = 0; i < INPUT_RECORD_COUNT; ++i)
        {
            VERIFY_ARE_EQUAL(inRecords[i], referenceInRecords[i]);
        }
    }

    TEST_METHOD(TranslateInputToOemSplitsDbcsChars)
    {
        Log::Comment(L"dbcs chars should be split");
        INPUT_RECORD inRecords[INPUT_RECORD_COUNT * 2];
        // U+3042 hiragana letter A
        wchar_t hiraganaA = 0x3042;
        wchar_t inChars[INPUT_RECORD_COUNT];
        for (size_t i = 0; i < INPUT_RECORD_COUNT; ++i)
        {
            wchar_t currentChar = static_cast<wchar_t>(hiraganaA + (i * 2));
            inRecords[i].EventType = KEY_EVENT;
            inRecords[i].Event.KeyEvent.uChar.UnicodeChar = currentChar;
            inChars[i] = currentChar;
        }
        ULONG outNum = TranslateInputToOem(inRecords, INPUT_RECORD_COUNT * 2, INPUT_RECORD_COUNT, nullptr);
        VERIFY_ARE_EQUAL(outNum, INPUT_RECORD_COUNT * 2);
        // create the data to compare the output to
        char dbcsChars[INPUT_RECORD_COUNT * 2] = { 0 };
        int writtenBytes = WideCharToMultiByte(g_ciConsoleInformation.CP,
                                               0,
                                               inChars,
                                               INPUT_RECORD_COUNT,
                                               dbcsChars,
                                               INPUT_RECORD_COUNT * 2,
                                               nullptr,
                                               false);
        VERIFY_ARE_EQUAL(writtenBytes, (int)(INPUT_RECORD_COUNT * 2));
        for (size_t i = 0; i < INPUT_RECORD_COUNT * 2; ++i)
        {
            VERIFY_ARE_EQUAL(inRecords[i].Event.KeyEvent.uChar.AsciiChar, dbcsChars[i]);
        }
    }

    TEST_METHOD(TranslateInputToOemSavesPartiaDbcsByteLeftover)
    {
        Log::Comment(L"if the optional 4th param is not null and there isn't enough space to store all converted dbcs chars in the buffer, it should store the leftover byte in the optional param");
        INPUT_RECORD inRecords[INPUT_RECORD_COUNT * 2];
        // U+3042 hiragana letter A
        wchar_t hiraganaA = 0x3042;
        wchar_t inChars[INPUT_RECORD_COUNT];
        for (size_t i = 0; i < INPUT_RECORD_COUNT; ++i)
        {
            wchar_t currentChar = static_cast<wchar_t>(hiraganaA + (i * 2));
            inRecords[i].EventType = KEY_EVENT;
            inRecords[i].Event.KeyEvent.uChar.UnicodeChar = currentChar;
            inChars[i] = currentChar;
        }
        INPUT_RECORD partialRecord;
        ULONG outNum = TranslateInputToOem(inRecords, (INPUT_RECORD_COUNT * 2) - 1, INPUT_RECORD_COUNT, &partialRecord);
        VERIFY_ARE_EQUAL(outNum, (INPUT_RECORD_COUNT * 2) - 1);
        // create the data to compare the output to
        char dbcsChars[INPUT_RECORD_COUNT * 2] = { 0 };
        int writtenBytes = WideCharToMultiByte(g_ciConsoleInformation.CP,
                                               0,
                                               inChars,
                                               INPUT_RECORD_COUNT,
                                               dbcsChars,
                                               INPUT_RECORD_COUNT * 2,
                                               nullptr,
                                               false);
        VERIFY_ARE_EQUAL(writtenBytes, (int)(INPUT_RECORD_COUNT * 2));
        for (size_t i = 0; i < (INPUT_RECORD_COUNT * 2) - 1; ++i)
        {
            VERIFY_ARE_EQUAL(inRecords[i].Event.KeyEvent.uChar.AsciiChar, dbcsChars[i]);
        }
        VERIFY_ARE_EQUAL(partialRecord.Event.KeyEvent.uChar.AsciiChar, dbcsChars[(INPUT_RECORD_COUNT * 2) - 1]);
    }


};