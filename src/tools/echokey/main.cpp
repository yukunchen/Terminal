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

bool gVtInput = false;
bool gVtOutput = true;

void csi(string seq)
{
    if (!gVtOutput) return;
    string fullSeq = "\x1b[";
    fullSeq += seq;
    printf(fullSeq.c_str());
}

void toPrintableBuffer(char c, char* printBuffer, int* printCch)
{
    if (c == '\x1b')
    {
        printBuffer[0] = '^';
        printBuffer[1] = '[';
        printBuffer[2] = '\0';
        *printCch = 2;
    }
    else if (c == '\x03') {
        printBuffer[0] = '^';
        printBuffer[1] = 'C';
        printBuffer[2] = '\0';
        *printCch = 2;
    }
    else if (c == '\x0')
    {
        printBuffer[0] = '\\';
        printBuffer[1] = '0';
        printBuffer[2] = '\0';
        *printCch = 2;
    }
    else if (c == '\r')
    {
        printBuffer[0] = '\\';
        printBuffer[1] = 'r';
        printBuffer[2] = '\0';
        *printCch = 2;
    }
    else if (c == '\n')
    {
        printBuffer[0] = '\\';
        printBuffer[1] = 'n';
        printBuffer[2] = '\0';
        *printCch = 2;
    }
    else if (c == '\t')
    {
        printBuffer[0] = '\\';
        printBuffer[1] = 't';
        printBuffer[2] = '\0';
        *printCch = 2;
    }
    else if (c == '\b')
    {
        printBuffer[0] = '\\';
        printBuffer[1] = 'b';
        printBuffer[2] = '\0';
        *printCch = 2;
    }
    else
    {
        printBuffer[0] = (char)c;
        printBuffer[1] = ' ';
        printBuffer[2] = '\0';
        *printCch = 2;
    }

}

void handleKeyEvent(KEY_EVENT_RECORD keyEvent)
{
    char printBuffer[3];
    int printCch = 0;
    const char c = keyEvent.uChar.AsciiChar;
    toPrintableBuffer(c, printBuffer, &printCch);

    if (!keyEvent.bKeyDown)
    {
        // Print in grey
        csi("38;5;242m");
    }

    wprintf(L"Down: %d Repeat: %d KeyCode: 0x%x ScanCode: 0x%x Char: %hs (0x%x) KeyState: 0x%x\r\n",
            keyEvent.bKeyDown,
            keyEvent.wRepeatCount,
            keyEvent.wVirtualKeyCode,
            keyEvent.wVirtualScanCode,
            printBuffer,
            keyEvent.uChar.AsciiChar,
            keyEvent.dwControlKeyState);

    // restore colors
    csi("0m");

    // // If printable:
    // if (keyEvent.uChar.AsciiChar > ' ' && keyEvent.uChar.AsciiChar != '\x7f')
    // {
    //     wprintf(L"Down: %d Repeat: %d KeyCode: 0x%x ScanCode: 0x%x Char: %c (0x%x) KeyState: 0x%x\r\n",
    //             keyEvent.bKeyDown,
    //             keyEvent.wRepeatCount,
    //             keyEvent.wVirtualKeyCode,
    //             keyEvent.wVirtualScanCode,
    //             keyEvent.uChar.AsciiChar,
    //             keyEvent.uChar.AsciiChar,
    //             keyEvent.dwControlKeyState);
    // }
    // else
    // {
    //     wprintf(L"Down: %d Repeat: %d KeyCode: 0x%x ScanCode: 0x%x Char:(0x%x) KeyState: 0x%x\r\n",
    //             keyEvent.bKeyDown,
    //             keyEvent.wRepeatCount,
    //             keyEvent.wVirtualKeyCode,
    //             keyEvent.wVirtualScanCode,
    //             keyEvent.uChar.AsciiChar,
    //             keyEvent.dwControlKeyState);
    // }

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

    gVtInput = false;
    gVtOutput = true;

    for(int i = 0; i < argc; i++)
    {
        wstring arg = wstring(argv[i]);
        wprintf(L"arg=%s\n", arg.c_str());
        if (arg.compare(L"-i") == 0)
        {
            gVtInput = true;
            wprintf(L"Using VT Input\n");
        }
        else if (arg.compare(L"-o") == 0)
        {
            gVtOutput = false;
            wprintf(L"Disabling VT Output\n");
        }
    }

    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);

    DWORD dwOutMode = 0;
    DWORD dwInMode = 0;

    GetConsoleMode(hOut, &dwOutMode);
    GetConsoleMode(hIn, &dwInMode);
    wprintf(L"Start Mode (i/o):(0x%x, 0x%x)\n", dwInMode, dwOutMode);

    if (gVtOutput)
        dwOutMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING | DISABLE_NEWLINE_AUTO_RETURN;

    if (gVtInput)
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
