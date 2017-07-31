//
//    Copyright (C) Microsoft.  All rights reserved.
//

// System headers
#include <windows.h>

// Standard library C-style
#include <wchar.h>
#include <stdlib.h>
#include <stdio.h>

#include <wil\result.h>
#include <wil\resource.h>
#include <wil\wistd_functional.h>
#include <wil\wistd_memory.h>

#include <string>
#include <sstream>

HANDLE hVT;

using namespace std;

void csi(string seq){
    string fullSeq = "\x1b[";
    fullSeq += seq;
    printf(fullSeq.c_str());
}

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

    // if (keyEvent.uChar.AsciiChar == 0x3) return;

    if (keyEvent.bKeyDown)
    {
        char buffer[2];
        // std::stringstream ss;
        // std::string vtseq;

        // ss << (char)keyEvent.uChar.AsciiChar;
        // ss >> vtseq;

        buffer[0] = (char)keyEvent.uChar.AsciiChar;
        buffer[1] = '\0';
        std::string vtseq = std::string(buffer, 1);

        csi("38;5;242m");
        wprintf(L"\tWriting \"%hs\" length=[%d]\n", vtseq.c_str(), (int)vtseq.length());
        csi("0m");

        WriteFile(hVT, vtseq.c_str(), (DWORD)vtseq.length(), nullptr, nullptr);
        
    }


}


int __cdecl wmain(int argc, WCHAR* argv[])
{
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);

    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);

    DWORD dwOutMode = 0;
    DWORD dwInMode = 0;

    GetConsoleMode(hOut, &dwOutMode);
    GetConsoleMode(hIn, &dwInMode);
    wprintf(L"Mode:0x%x\n", dwInMode);

    dwOutMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING | DISABLE_NEWLINE_AUTO_RETURN;
    dwInMode |= ENABLE_VIRTUAL_TERMINAL_INPUT;

    SetConsoleMode(hOut, dwOutMode);
    SetConsoleMode(hIn, dwInMode);


    hVT = CreateFileW(L"\\\\.\\pipe\\convtinpipe", GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    
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
