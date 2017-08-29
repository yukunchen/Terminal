//
//    Copyright (C) Microsoft.  All rights reserved.
//
#include <windows.h>
#include <wil\result.h>
#include <wil\resource.h>
#include <wil\wistd_functional.h>
#include <wil\wistd_memory.h>
#include <stdlib.h>     /* srand, rand */
#include <time.h>       /* time */

#include <deque>
#include <vector>
#include <string>
#include <sstream>
#include <assert.h>

#include "VtConsole.hpp"

#define IN_PIPE_NAME L"\\\\.\\pipe\\convtinpipe"
#define OUT_PIPE_NAME L"\\\\.\\pipe\\convtoutpipe"
using namespace std;
////////////////////////////////////////////////////////////////////////////////
// State
// wil::unique_handle outPipe;
// wil::unique_handle inPipe;
// HANDLE outPipe;
// HANDLE inPipe;
// HANDLE dbgPipe;
HANDLE hOut;
HANDLE hIn;

std::deque<VtConsole*> consoles;

bool prefixPressed = false;

// CRITICAL_SECTION whyNot;
////////////////////////////////////////////////////////////////////////////////

// void lock()
// {
//     EnterCriticalSection(&whyNot);
// }

// void unlock()
// {
//     LeaveCriticalSection(&whyNot);
// }

void ReadCallback(byte* buffer, DWORD dwRead)
{
    THROW_LAST_ERROR_IF_FALSE(WriteFile(hOut, buffer, dwRead, nullptr, nullptr));
}

VtConsole* getConsole()
{
    return consoles[0];
} 

void nextConsole()
{
    // lock();
    auto con = consoles[0];
    con->deactivate();
    consoles.pop_front();
    consoles.push_back(con);
    con = consoles[0];
    con->activate();
    // unlock();
}

HANDLE inPipe()
{
    return getConsole()->inPipe();
}

HANDLE outPipe()
{
    return getConsole()->outPipe();
}

void newConsole()
{
    auto con = new VtConsole(ReadCallback);
    con->spawn();
    consoles.push_back(con);
}

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

void handleManyEvents(const INPUT_RECORD* const inputBuffer, int cEvents)
{
    char* const buffer = new char[cEvents];
    // char* const printableBuffer = new char[cEvents * 3];
    char* nextBuffer = buffer;
    // char* nextPrintable = printableBuffer;
    int bufferCch = 0;
    // int printableCch = 0;


    for (int i = 0; i < cEvents; ++i)
    {
        INPUT_RECORD event = inputBuffer[i];
        if (event.EventType != KEY_EVENT)
        {
            continue;
        }

        KEY_EVENT_RECORD keyEvent = event.Event.KeyEvent;
        
        // printKeyEvent(keyEvent);
        
        if (keyEvent.bKeyDown)
        {
            const char c = keyEvent.uChar.AsciiChar;

            if (c == '\0' && keyEvent.wVirtualScanCode != 0)
            {
                // This is a special keyboard key that was pressed, not actually NUL
                continue;
            }
            if (!prefixPressed)
            {
                if (c == '\x2')
                // if (c == '\x2' && keyEvent.wVirtualKeyCode == 'B')
                {
                    prefixPressed = true;
                }
                else
                {
                    *nextBuffer = c;
                    nextBuffer++;
                    bufferCch++;
                }
            }
            else
            {
                switch(c)
                {
                    case 'n':
                        nextConsole();
                        break;
                    case 'c':
                        newConsole();
                        break;
                    default:
                        *nextBuffer = c;
                        nextBuffer++;
                        bufferCch++;
                }
                prefixPressed = false;
            }
            // int numPrintable = 0;
            // toPrintableBuffer(c, nextPrintable, &numPrintable);
            // nextPrintable += numPrintable;
            // printableCch += numPrintable;
            
        }
    }

    if (bufferCch > 0)
    {
        std::string vtseq = std::string(buffer, bufferCch);
        // std::string printSeq = std::string(printableBuffer, printableCch);

        // csi("38;5;242m");
        // wprintf(L"\tWriting \"%hs\" length=[%d]\n", printSeq.c_str(), (int)vtseq.length());
        // csi("0m");

        // WriteFile(inPipe.get(), vtseq.c_str(), (DWORD)vtseq.length(), nullptr, nullptr);
        WriteFile(inPipe(), vtseq.c_str(), (DWORD)vtseq.length(), nullptr, nullptr);
    }
}


DWORD OutputThread(LPVOID lpParameter)
{
    // THROW_LAST_ERROR_IF_FALSE(ConnectNamedPipe(outPipe, nullptr));
    // DebugBreak();
    UNREFERENCED_PARAMETER(lpParameter);
    DWORD dwMode = 0;
    
    THROW_LAST_ERROR_IF_FALSE(GetConsoleMode(hOut, &dwMode));
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    dwMode |= DISABLE_NEWLINE_AUTO_RETURN;
    THROW_LAST_ERROR_IF_FALSE(SetConsoleMode(hOut, dwMode));

    return 0;
    // byte buffer[256];
    // DWORD dwRead;
    // while (true)
    // {
    //     dwRead = 0;
    //     bool fSuccess = false;
    //     OVERLAPPED o = {0};
    //     o.Offset = getConsole()->getReadOffset();

    //     fSuccess = !!ReadFile(outPipe(), buffer, ARRAYSIZE(buffer), &dwRead, nullptr);
    //     // fSuccess = !!ReadFile(outPipe(), buffer, ARRAYSIZE(buffer), &dwRead, &o);

    //     // if (!fSuccess && GetLastError()==ERROR_IO_PENDING) continue;
    //     // else if (!fSuccess) THROW_LAST_ERROR_IF_FALSE(fSuccess);
    //     THROW_LAST_ERROR_IF_FALSE(fSuccess);
    //     ReadCallback(buffer, dwRead);
    //     // THROW_LAST_ERROR_IF_FALSE(ReadFile(outPipe.get(), buffer, ARRAYSIZE(buffer), &dwRead, nullptr));
    //     // getConsole()->incrementReadOffset(dwRead);
    //     THROW_LAST_ERROR_IF_FALSE(WriteFile(hOut, buffer, dwRead, nullptr, nullptr));
    // }
}

DWORD InputThread(LPVOID lpParameter)
{
    // THROW_LAST_ERROR_IF_FALSE(ConnectNamedPipe(inPipe, nullptr));
    // writeDebug("Connected to both\n");
    // DebugBreak();
    UNREFERENCED_PARAMETER(lpParameter);
    
    DWORD dwInMode = 0;
    // DebugBreak();
    GetConsoleMode(hIn, &dwInMode);
    dwInMode |= ENABLE_VIRTUAL_TERMINAL_INPUT;
    SetConsoleMode(hIn, dwInMode);
    
    for (;;)
    {
        INPUT_RECORD rc[256];
        DWORD dwRead = 0;
        ReadConsoleInputA(hIn, rc, 256, &dwRead);
        handleManyEvents(rc, dwRead);
    }
}

bool openConsole(std::wstring inPipeName, std::wstring outPipeName)
{
    std::wstring cmdline = L"OpenConsole.exe";
    if (inPipeName.length() > 0)
    {
        cmdline += L" --inpipe ";
        cmdline += inPipeName;
    }
    if (outPipeName.length() > 0)
    {
        cmdline += L" --outpipe ";
        cmdline += outPipeName;
    }

    PROCESS_INFORMATION pi = {0};
    STARTUPINFO si = {0};
    si.cb = sizeof(STARTUPINFOW);
    bool fSuccess = !!CreateProcess(
        nullptr,
        &cmdline[0],
        nullptr,    // lpProcessAttributes
        nullptr,    // lpThreadAttributes
        false,      // bInheritHandles
        0,          // dwCreationFlags
        nullptr,    // lpEnvironment
        nullptr,    // lpCurrentDirectory
        &si,        //lpStartupInfo
        &pi         //lpProcessInformation
    );


    if (!fSuccess)
    {
        wprintf(L"Failed to launch console\n");
    }
    return fSuccess;
}

void CreateIOThreads()
{
    OutputThread(nullptr);
    // DWORD dwOutputThreadId = (DWORD) -1;
    // HANDLE hOutputThread = CreateThread(nullptr,
    //                                     0,
    //                                     (LPTHREAD_START_ROUTINE)OutputThread,
    //                                     nullptr,
    //                                     0,
    //                                     &dwOutputThreadId);
    // hOutputThread;



    DWORD dwInputThreadId = (DWORD) -1;
    HANDLE hInputThread = CreateThread(nullptr,
                                        0,
                                        (LPTHREAD_START_ROUTINE)InputThread,
                                        nullptr,
                                        0,
                                        &dwInputThreadId);
    hInputThread;
}

// this function has unreachable code due to its unusual lifetime. We
// disable the warning about it here.
#pragma warning(push)
#pragma warning(disable:4702)
int __cdecl wmain(int /*argc*/, WCHAR* /*argv[]*/)
{
    // initialize random seed: 
    srand((unsigned int)time(NULL));

    hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    hIn = GetStdHandle(STD_INPUT_HANDLE);
  
    // InitializeCriticalSection(&whyNot);

    newConsole();  
    getConsole()->activate();
    CreateIOThreads();

    // Sleep(2000);
    // newConsole(); 

    // while(true)
    // {
    //     nextConsole();
    //     Sleep(3000);
    // } 


    // Exit the thread so the CRT won't clean us up and kill. The IO thread owns the lifetime now.
    ExitThread(S_OK);
    // We won't hit this. The ExitThread above will kill the caller at this point.
    assert(false);
    return 0;
}
#pragma warning(pop)
