/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/
#include "precomp.h"

#include "..\..\inc\consoletaeftemplates.hpp"

#include <memory>
#include <utility>
#include <iostream>

class KeyPressTests
{
    TEST_CLASS(KeyPressTests);

    // TODO: MSFT: 10187614 - Fix this test or the console code.
    //TEST_METHOD(TestAltGr)
    //{
    //    Log::Comment(L"Testing that alt-gr behavior hasn't changed");
    //    BOOL successBool;
    //    HWND hwnd = GetConsoleWindow();
    //    VERIFY_IS_NOT_NULL(hwnd);
    //    HANDLE inputHandle = GetStdHandle(STD_INPUT_HANDLE);
    //    DWORD events = 0;

    //    // flush input buffer
    //    FlushConsoleInputBuffer(inputHandle);
    //    successBool = GetNumberOfConsoleInputEvents(inputHandle, &events);
    //    VERIFY_IS_TRUE(!!successBool);
    //    VERIFY_ARE_EQUAL(events, 0);

    //    // send alt-gr + q keypress (@ on german keyboard)
    //    DWORD repeatCount = 1;
    //    SendMessage(hwnd,
    //                WM_CHAR,
    //                0x51, // q
    //                repeatCount | HIWORD(KF_EXTENDED | KF_ALTDOWN));
    //    // make sure the the keypresses got processed
    //    events = 0;
    //    successBool = GetNumberOfConsoleInputEvents(inputHandle, &events);
    //    VERIFY_IS_TRUE(!!successBool);
    //    VERIFY_IS_GREATER_THAN(events, 0u, NoThrowString().Format(L"%d", events));
    //    std::unique_ptr<INPUT_RECORD[]> inputBuffer = std::make_unique<INPUT_RECORD[]>(1);
    //    VERIFY_WIN32_BOOL_SUCCEEDED_RETURN(PeekConsoleInput(inputHandle,
    //                                                        inputBuffer.get(),
    //                                                        1,
    //                                                        &events));
    //    VERIFY_ARE_EQUAL(events, 1);

    //    INPUT_RECORD expectedEvent;
    //    expectedEvent.EventType = KEY_EVENT;
    //    expectedEvent.Event.KeyEvent.bKeyDown = !!true;
    //    expectedEvent.Event.KeyEvent.wRepeatCount = 1;
    //    expectedEvent.Event.KeyEvent.wVirtualKeyCode = 0;
    //    expectedEvent.Event.KeyEvent.wVirtualScanCode = 0;
    //    expectedEvent.Event.KeyEvent.dwControlKeyState = 32;
    //    expectedEvent.Event.KeyEvent.uChar.UnicodeChar = L'Q';
    //    // compare values against those that have historically been
    //    // returned with the same arguments to SendMessage
    //    VERIFY_ARE_EQUAL(expectedEvent, inputBuffer[0]);
    //}

    TEST_METHOD(TestCoalesceSameKeyPress)
    {
        Log::Comment(L"Testing that key events are properly coalesced when the same key is pressed repeatedly");
        BOOL successBool;
        HWND hwnd = GetConsoleWindow();
        VERIFY_IS_NOT_NULL(hwnd);
        HANDLE inputHandle = GetStdHandle(STD_INPUT_HANDLE);
        DWORD events = 0;

        // flush input buffer
        FlushConsoleInputBuffer(inputHandle);
        successBool = GetNumberOfConsoleInputEvents(inputHandle, &events);
        VERIFY_IS_TRUE(!!successBool);
        VERIFY_ARE_EQUAL(events, 0);

        // send a bunch of 'a' keypresses to the console
        DWORD repeatCount = 1;
        const unsigned int messageSendCount = 1000;
        for (unsigned int i = 0; i < messageSendCount; ++i)
        {
            SendMessage(hwnd,
                        WM_CHAR,
                        0x41,
                        repeatCount);
        }

        // make sure the the keypresses got processed and coalesced
        events = 0;
        successBool = GetNumberOfConsoleInputEvents(inputHandle, &events);
        VERIFY_IS_TRUE(!!successBool);
        VERIFY_IS_GREATER_THAN(events, 0u, NoThrowString().Format(L"%d", events));
        std::unique_ptr<INPUT_RECORD[]> inputBuffer = std::make_unique<INPUT_RECORD[]>(1);
        PeekConsoleInput(inputHandle,
                         inputBuffer.get(),
                         1,
                         &events);
        VERIFY_ARE_EQUAL(events, 1);
        VERIFY_ARE_EQUAL(inputBuffer[0].EventType, KEY_EVENT);
        VERIFY_ARE_EQUAL(inputBuffer[0].Event.KeyEvent.wRepeatCount, messageSendCount, NoThrowString().Format(L"%d", inputBuffer[0].Event.KeyEvent.wRepeatCount));
    }


    TEST_METHOD(TestCtrlKeyDownUp)
    {
        BEGIN_TEST_METHOD_PROPERTIES()
            // VKeys for A-Z
            // See https://msdn.microsoft.com/en-us/library/windows/desktop/dd375731(v=vs.85).aspx
            TEST_METHOD_PROPERTY(L"Data:vKey", L"{"
                "0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4A,0x4B,0x4C,0x4D,0x4E,0x4F," 
                "0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5A" 
            "}")
        END_TEST_METHOD_PROPERTIES();
        UINT vk;
        VERIFY_SUCCEEDED(TestData::TryGetValue(L"vKey", vk));

        Log::Comment(L"Testing the right number of input events is generated by Ctrl+Key press");
        BOOL successBool;
        HWND hwnd = GetConsoleWindow();
        VERIFY_IS_NOT_NULL(hwnd);
        HANDLE inputHandle = GetStdHandle(STD_INPUT_HANDLE);
        DWORD events = 0;

        // Set the console to raw mode, so that it doesn't hijack any keypresses as shortcut keys
        SetConsoleMode(inputHandle, 0);
        
        // flush input buffer
        FlushConsoleInputBuffer(inputHandle);
        VERIFY_WIN32_BOOL_SUCCEEDED(GetNumberOfConsoleInputEvents(inputHandle, &events));
        VERIFY_ARE_EQUAL(events, 0);

        DWORD dwInMode = 0;
        GetConsoleMode(inputHandle, &dwInMode);
        Log::Comment(NoThrowString().Format(L"Mode:0x%x", dwInMode));

        UINT vkCtrl = VK_LCONTROL; // Need this instead of VK_CONTROL
        UINT uiCtrlScancode = MapVirtualKey(vkCtrl , MAPVK_VK_TO_VSC);
        // According to 
        // KEY_KEYDOWN https://msdn.microsoft.com/en-us/library/windows/desktop/ms646280(v=vs.85).aspx
        // KEY_UP https://msdn.microsoft.com/en-us/library/windows/desktop/ms646281(v=vs.85).aspx
        LPARAM CtrlFlags = ( 0 | ((uiCtrlScancode<<16) & 0x00ff0000) | 0x00000001);
        LPARAM CtrlUpFlags = CtrlFlags | 0xc0000000;

        UINT uiScancode = MapVirtualKey(vk , MAPVK_VK_TO_VSC);
        LPARAM DownFlags = ( 0 | ((uiScancode<<16) & 0x00ff0000) | 0x00000001);
        LPARAM UpFlags = DownFlags | 0xc0000000;

        Log::Comment(NoThrowString().Format(L"Testing Ctrl+%c", vk));
        Log::Comment(NoThrowString().Format(L"DownFlags=0x%x, CtrlFlags=0x%x", DownFlags, CtrlFlags));
        Log::Comment(NoThrowString().Format(L"UpFlags=0x%x, CtrlUpFlags=0x%x", UpFlags, CtrlUpFlags));

        // Don't Use PostMessage, those events come in the wrong order.
        // Also can't use SendInput because of the whole test window backgrounding thing.
        //      It'd work locally, until you minimize the window.
        SendMessage(hwnd, WM_KEYDOWN,   vkCtrl,     CtrlFlags);
        SendMessage(hwnd, WM_KEYDOWN,   vk,         DownFlags);
        SendMessage(hwnd, WM_KEYUP,     vk,         UpFlags);
        SendMessage(hwnd, WM_KEYUP,     vkCtrl,     CtrlUpFlags); 

        Sleep(50);

        events = 0;
        successBool = GetNumberOfConsoleInputEvents(inputHandle, &events);
        VERIFY_IS_TRUE(!!successBool);
        VERIFY_IS_GREATER_THAN(events, 0u, NoThrowString().Format(L"%d events found", events));

        std::unique_ptr<INPUT_RECORD[]> inputBuffer = std::make_unique<INPUT_RECORD[]>(16);
        PeekConsoleInput(inputHandle,
                         inputBuffer.get(),
                         16,
                         &events);

        for (size_t i = 0; i < events; i++)
        {
            INPUT_RECORD rc = inputBuffer[i];
            switch (rc.EventType)
            {
                case KEY_EVENT:
                {
                    Log::Comment(NoThrowString().Format(
                        L"Down: %d Repeat: %d KeyCode: 0x%x ScanCode: 0x%x Char: %c (0x%x) KeyState: 0x%x",
                        rc.Event.KeyEvent.bKeyDown,
                        rc.Event.KeyEvent.wRepeatCount,
                        rc.Event.KeyEvent.wVirtualKeyCode,
                        rc.Event.KeyEvent.wVirtualScanCode,
                        rc.Event.KeyEvent.uChar.UnicodeChar != 0 ? rc.Event.KeyEvent.uChar.UnicodeChar : ' ',
                        rc.Event.KeyEvent.uChar.UnicodeChar,
                        rc.Event.KeyEvent.dwControlKeyState
                    ));

                    break;
                }
                default:
                    Log::Comment(NoThrowString().Format(L"Another event type was found."));
            }
        }
        VERIFY_ARE_EQUAL(events, 4);
        VERIFY_ARE_EQUAL(inputBuffer[0].EventType, KEY_EVENT);
        VERIFY_ARE_EQUAL(inputBuffer[1].EventType, KEY_EVENT);
        VERIFY_ARE_EQUAL(inputBuffer[2].EventType, KEY_EVENT);
        VERIFY_ARE_EQUAL(inputBuffer[3].EventType, KEY_EVENT);

        FlushConsoleInputBuffer(inputHandle);

    }

};