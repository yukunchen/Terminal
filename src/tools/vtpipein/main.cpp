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

void printKeyEvent(KEY_EVENT_RECORD keyEvent)
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

}

void toPrintableBuffer(char c, char* printBuffer, int* printCch)
{
    if (c == '\x1b')
    {
        printBuffer[0] = '^';
        printBuffer[1] = '[';
        // printBuffer[2] = '\0';
        *printCch = 2;
    }
    else if (c == '\x03') {
        printBuffer[0] = '^';
        printBuffer[1] = 'C';
        // printBuffer[2] = '\0';
        *printCch = 2;
    }
    else if (c == '\x0')
    {
        printBuffer[0] = '\\';
        printBuffer[1] = '0';
        // printBuffer[2] = '\0';
        *printCch = 2;
    }
    else if (c == '\r')
    {
        printBuffer[0] = '\\';
        printBuffer[1] = 'r';
        // printBuffer[2] = '\0';
        *printCch = 2;
    }
    else if (c == '\n')
    {
        printBuffer[0] = '\\';
        printBuffer[1] = 'n';
        // printBuffer[2] = '\0';
        *printCch = 2;
    }
    else
    {
        printBuffer[0] = (char)c;
        // printBuffer[1] = '\0';
        *printCch = 1;
    }

}

void handleKeyEvent(KEY_EVENT_RECORD keyEvent)
{
    printKeyEvent(keyEvent);
    // if (keyEvent.uChar.AsciiChar == 0x3) return;

    if (keyEvent.bKeyDown)
    {
        char buffer[2];
        char printBuffer[3];
        int cch = 0;
        int printCch = 0;

        const char c = keyEvent.uChar.AsciiChar;
        
        buffer[0] = (char)c;
        buffer[1] = '\0';
        cch = 1;
        
        toPrintableBuffer(c, printBuffer, &printCch);

        std::string vtseq = std::string(buffer, cch);
        std::string printSeq = std::string(printBuffer, printCch);

        csi("38;5;242m");
        wprintf(L"\tWriting \"%hs\" length=[%d]\n", printSeq.c_str(), (int)vtseq.length());
        csi("0m");

        WriteFile(hVT, vtseq.c_str(), (DWORD)vtseq.length(), nullptr, nullptr);
        
    }
}

void handleManyEvents(const INPUT_RECORD* const inputBuffer, int cEvents)
{
    char* const buffer = new char[cEvents];
    char* const printableBuffer = new char[cEvents * 3];
    char* nextBuffer = buffer;
    char* nextPrintable = printableBuffer;
    int bufferCch = 0;
    int printableCch = 0;


    for (int i = 0; i < cEvents; ++i)
    {
        INPUT_RECORD event = inputBuffer[i];
        if (event.EventType != KEY_EVENT)
        {
            continue;
        }

        KEY_EVENT_RECORD keyEvent = event.Event.KeyEvent;
        
        printKeyEvent(keyEvent);
        
        if (keyEvent.bKeyDown)
        {
            const char c = keyEvent.uChar.AsciiChar;

            if (c == '\0' && keyEvent.wVirtualScanCode != 0)
            {
                // This is a special keyboard key that was pressed, not actually NUL
                continue;
            }

            *nextBuffer = c;
            nextBuffer++;
            bufferCch++;

            int numPrintable = 0;
            toPrintableBuffer(c, nextPrintable, &numPrintable);
            nextPrintable += numPrintable;
            printableCch += numPrintable;
            
        }
    }

    if (bufferCch > 0)
    {
        std::string vtseq = std::string(buffer, bufferCch);
        std::string printSeq = std::string(printableBuffer, printableCch);

        csi("38;5;242m");
        wprintf(L"\tWriting \"%hs\" length=[%d]\n", printSeq.c_str(), (int)vtseq.length());
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


    // hVT = CreateFileW(L"\\\\.\\pipe\\convtinpipe", GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    
    wil::unique_handle pipe;
    pipe.reset(CreateNamedPipeW(L"\\\\.\\pipe\\convtinpipe", PIPE_ACCESS_DUPLEX, PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT, 1, 0, 0, 0, nullptr));
    hVT = pipe.get();
    THROW_IF_HANDLE_INVALID(hVT);
    THROW_LAST_ERROR_IF_FALSE(ConnectNamedPipe(hVT, nullptr));

    

    wprintf(L"Connected to VT Pipe, Handle is Valid=%s\n", (hVT==INVALID_HANDLE_VALUE?L"false":L"true"));

    for (;;)
    {
        INPUT_RECORD rc[256];
        // INPUT_RECORD rc;
        DWORD dwRead = 0;
        ReadConsoleInputA(hIn, rc, 256, &dwRead);

        handleManyEvents(rc, dwRead);
        // switch (rc.EventType)
        // {
        // case KEY_EVENT:
        // {
        //     handleKeyEvent(rc.Event.KeyEvent);
        //     break;
        // }
        // }
    }
}
