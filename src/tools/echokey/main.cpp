//
//    Copyright (C) Microsoft.  All rights reserved.
//
#define DEFINE_CONSOLEV2_PROPERTIES

// System headers
#include <windows.h>

// Standard library C-style
#include <wchar.h>
#include <stdlib.h>
#include <stdio.h>

#include <string>
using namespace std;


void handleKeyEvent(KEY_EVENT_RECORD keyEvent)
{
    // If printable:
    if (keyEvent.uChar.AsciiChar > ' ' && keyEvent.uChar.AsciiChar != '\x7f')
    {
        wprintf(L"Down: %d Repeat: %d KeyCode: 0x%x ScanCode: 0x%x Char: %c (0x%x) KeyState: 0x%x\r\n",
                keyEvent.bKeyDown,
                keyEvent.wRepeatCount,
                keyEvent.wVirtualKeyCode,
                keyEvent.wVirtualScanCode,
                keyEvent.uChar.AsciiChar,
                keyEvent.uChar.AsciiChar,
                keyEvent.dwControlKeyState);
    }
    else
    {
        wprintf(L"Down: %d Repeat: %d KeyCode: 0x%x ScanCode: 0x%x Char:(0x%x) KeyState: 0x%x\r\n",
                keyEvent.bKeyDown,
                keyEvent.wRepeatCount,
                keyEvent.wVirtualKeyCode,
                keyEvent.wVirtualScanCode,
                keyEvent.uChar.AsciiChar,
                keyEvent.dwControlKeyState);
    }

    // Die on Ctrl+C
    if (keyEvent.uChar.AsciiChar == 0x3)
    {
        exit (EXIT_FAILURE);
    }
}

int __cdecl wmain(int argc, wchar_t* argv[])
{
    // UNREFERENCED_PARAMETER(argc);
    // UNREFERENCED_PARAMETER(argv);

    bool vtInput = false;
    bool vtOutput = false;

    for(int i = 0; i < argc; i++)
    {
        wstring arg = wstring(argv[i]);
        wprintf(L"arg=%s\n", arg.c_str());
        if (arg.compare(L"-i") == 0)
        {
            vtInput = true;
            wprintf(L"Using VT Input\n");
        }
        else if (arg.compare(L"-o") == 0)
        {
            vtOutput = true;
            wprintf(L"Using VT Output\n");
        }
    }

    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);

    DWORD dwOutMode = 0;
    DWORD dwInMode = 0;

    GetConsoleMode(hOut, &dwOutMode);
    GetConsoleMode(hIn, &dwInMode);
    wprintf(L"Start Mode (i/o):(0x%x, 0x%x)\n", dwInMode, dwOutMode);

    if (vtOutput)
        dwOutMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING | DISABLE_NEWLINE_AUTO_RETURN;

    if (vtInput)
        dwInMode |= ENABLE_VIRTUAL_TERMINAL_INPUT;


    SetConsoleMode(hOut, dwOutMode);
    SetConsoleMode(hIn, dwInMode);
    // wprintf(L"New Mode:0x%x\n", dwInMode);
    wprintf(L"New Mode (i/o):(0x%x, 0x%x)\n", dwInMode, dwOutMode);
    
    for (;;)
    {
        INPUT_RECORD rc;
        DWORD dwRead = 0;
        ReadConsoleInputA(hIn, &rc, 1, &dwRead);

        switch (rc.EventType)
        {
        case KEY_EVENT:
        {
            handleKeyEvent(rc.Event.KeyEvent);
            break;
        }
        }
    }
}
