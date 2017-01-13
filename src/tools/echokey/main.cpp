//
//    Copyright (C) Microsoft.  All rights reserved.
//
#include <precomp.h>

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

    // dwOutMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING | DISABLE_NEWLINE_AUTO_RETURN;
    // dwInMode |= ENABLE_VIRTUAL_TERMINAL_INPUT;

    SetConsoleMode(hOut, dwOutMode);
    SetConsoleMode(hIn, dwInMode);

    
    for (;;)
    {
        INPUT_RECORD rc;
        DWORD dwRead = 0;
        ReadConsoleInputA(hIn, &rc, 1, &dwRead);

        switch (rc.EventType)
        {
        case KEY_EVENT:
        {
            wprintf(L"Down: %d Repeat: %d KeyCode: 0x%x ScanCode: 0x%x Char: %c (0x%x) KeyState: 0x%x\r\n",
                    rc.Event.KeyEvent.bKeyDown,
                    rc.Event.KeyEvent.wRepeatCount,
                    rc.Event.KeyEvent.wVirtualKeyCode,
                    rc.Event.KeyEvent.wVirtualScanCode,
                    rc.Event.KeyEvent.uChar.AsciiChar,
                    rc.Event.KeyEvent.uChar.AsciiChar,
                    rc.Event.KeyEvent.dwControlKeyState);

            if (rc.Event.KeyEvent.uChar.AsciiChar == 0x3) return 0;

            break;
        }
        }
    }
}
