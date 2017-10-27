//
//    Copyright (C) Microsoft.  All rights reserved.
//
#include <windows.h>
#include <wil\Common.h>
#include <wil\result.h>
#include <wil\resource.h>
#include <wil\wistd_functional.h>
#include <wil\wistd_memory.h>
#include <stdlib.h>     /* srand, rand */
#include <time.h>       /* time */

#include <deque>
#include <memory>
#include <vector>
#include <string>
#include <sstream>
#include <assert.h>

#include "VtConsole.hpp"

using namespace std;
////////////////////////////////////////////////////////////////////////////////
// State
HANDLE hOut;
HANDLE hIn;

std::deque<VtConsole*> consoles;
// A console for printing debug output to
VtConsole* debug;

bool prefixPressed = false;

////////////////////////////////////////////////////////////////////////////////
// Forward decls
std::string toPrintableString(std::string& inString);
void toPrintableBuffer(char c, char* printBuffer, int* printCch);
////////////////////////////////////////////////////////////////////////////////

void ReadCallback(byte* buffer, DWORD dwRead)
{
    // We already set the console to UTF-8 CP, so we can just write straight to it
    THROW_LAST_ERROR_IF_FALSE(WriteFile(hOut, buffer, dwRead, nullptr, nullptr));

    std::string renderData = std::string((char*)buffer, dwRead);
    std::string printable = toPrintableString(renderData);
    std::string seq = "\n";
    debug->WriteInput(printable);
    debug->WriteInput(seq);

}

void DebugReadCallback(byte* /*buffer*/, DWORD /*dwRead*/)
{
    // do nothing.
}

VtConsole* getConsole()
{
    return consoles[0];
} 

void nextConsole()
{
    auto con = consoles[0];
    con->deactivate();
    consoles.pop_front();
    consoles.push_back(con);
    con = consoles[0];
    con->activate();
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

std::string csi(string seq)
{
    // Note: This doesn't do anything for the debug console currently. 
    //      Somewhere, the TTY eats the control sequences. Still useful though.
    string fullSeq = "\x1b[";
    fullSeq += seq;
    return fullSeq;
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
        *printCch = 2;
    }
    else if (c == '\x03') {
        printBuffer[0] = '^';
        printBuffer[1] = 'C';
        *printCch = 2;
    }
    else if (c == '\x0')
    {
        printBuffer[0] = '\\';
        printBuffer[1] = '0';
        *printCch = 2;
    }
    else if (c == '\r')
    {
        printBuffer[0] = '\\';
        printBuffer[1] = 'r';
        *printCch = 2;
    }
    else if (c == '\n')
    {
        printBuffer[0] = '\\';
        printBuffer[1] = 'n';
        *printCch = 2;
    }
    else if (c < '\x20')
    {
        printBuffer[0] = '^';
        printBuffer[1] = c+0x40;
        *printCch = 2;
    }
    else if (c == '\x7f')
    {
        printBuffer[0] = '\\';
        printBuffer[1] = 'x';
        printBuffer[2] = '7';
        printBuffer[3] = 'f';
        *printCch = 4;
    }
    else
    {
        printBuffer[0] = (char)c;
        *printCch = 1;
    }
}

std::string toPrintableString(std::string& inString)
{
    std::string retval = "";
    for (size_t i = 0; i < inString.length(); i++)
    {
        char c = inString[i];
        if (c < '\x20')
        {
            retval += "^";
            char actual = (c + 0x40);
            retval += std::string(1, actual);
        }
        else if (c == '\x7f')
        {
            retval += "\\x7f";
        }
        else if (c == '\x20')
        {
            retval += "SPC";
        }
        else
        {
            retval += std::string(1, c);
        }
    }
    return retval;
}

void handleManyEvents(const INPUT_RECORD* const inputBuffer, int cEvents)
{
    char* const buffer = new char[cEvents];
    char* const printableBuffer = new char[cEvents * 4];
    memset(buffer, 0, cEvents);
    memset(printableBuffer, 0, cEvents * 4);

    char* nextBuffer = buffer;
    char* nextPrintable = printableBuffer;
    int bufferCch = 0;
    int printableCch = 0;


    for (int i = 0; i < cEvents; ++i)
    {
        INPUT_RECORD event = inputBuffer[i];

        if (event.EventType == KEY_EVENT)
        {
            KEY_EVENT_RECORD keyEvent = event.Event.KeyEvent;
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
                        case '\t':
                            nextConsole();
                            break;
                        case 'c':
                            newConsole();
                            break;
                        case 't':
                            newConsole();
                            nextConsole();
                            break;
                        default:
                            *nextBuffer = c;
                            nextBuffer++;
                            bufferCch++;
                    }
                    prefixPressed = false;
                }
                int numPrintable = 0;
                toPrintableBuffer(c, nextPrintable, &numPrintable);
                nextPrintable += numPrintable;
                printableCch += numPrintable;
            }
        }
        else if (event.EventType == WINDOW_BUFFER_SIZE_EVENT)
        {
            WINDOW_BUFFER_SIZE_RECORD resize = event.Event.WindowBufferSizeEvent;
            // the resize event doesn't actually have the info we want.
            CONSOLE_SCREEN_BUFFER_INFOEX csbiex = { 0 };
            csbiex.cbSize = sizeof(CONSOLE_SCREEN_BUFFER_INFOEX);
            bool fSuccess = !!GetConsoleScreenBufferInfoEx(hOut, &csbiex);
            if (fSuccess)
            {
                SMALL_RECT srViewport = csbiex.srWindow;
                
                std::stringstream ss;
                std::stringstream debugSs;
                ss << "\x1b[8;";
                debugSs << "\\x1b[8;";
                short width = srViewport.Right - srViewport.Left + 1;
                short height = srViewport.Bottom - srViewport.Top + 1;
                
                ss << height;
                debugSs << height;

                ss << ";";
                debugSs << ";";
                
                ss << width;
                debugSs << width;
                
                ss << "t";
                debugSs << "t";

                std::string seq;
                std::string debugSeq;
                ss >> seq;
                debugSs >> debugSeq;
                getConsole()->WriteInput(seq);
                debug->WriteInput(debugSeq);
            }
        }

    }

    if (bufferCch > 0)
    {
        std::string vtseq = std::string(buffer, bufferCch);
        std::string printSeq = std::string(printableBuffer, printableCch);

        getConsole()->WriteInput(vtseq);

        std::stringstream ss;
        ss << "Input \"" << printSeq.c_str() << "\" [" << vtseq.length() << "]\n";
        std::string debugString = ss.str();
        debug->WriteInput(debugString);
    }
}

void SetupOutput()
{
    DWORD dwMode = 0;
    THROW_LAST_ERROR_IF_FALSE(GetConsoleMode(hOut, &dwMode));
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    dwMode |= DISABLE_NEWLINE_AUTO_RETURN;
    THROW_LAST_ERROR_IF_FALSE(SetConsoleMode(hOut, dwMode));
}
void SetupInput()
{
    DWORD dwInMode = 0;
    GetConsoleMode(hIn, &dwInMode);
    dwInMode |= ENABLE_VIRTUAL_TERMINAL_INPUT;
    dwInMode = ENABLE_VIRTUAL_TERMINAL_INPUT;
    SetConsoleMode(hIn, dwInMode);
}

DWORD InputThread(LPVOID lpParameter)
{
    UNREFERENCED_PARAMETER(lpParameter);
    
    // Because the input thread ends up owning the lifetime of the application, 
    // Set/restore the CP here.

    unsigned int launchCP = GetConsoleOutputCP();
    THROW_LAST_ERROR_IF_FALSE(SetConsoleOutputCP(CP_UTF8));
    auto restore = wil::ScopeExit([&] 
    {
        SetConsoleOutputCP(launchCP);
    });

    
    for (;;)
    {
        INPUT_RECORD rc[256];
        DWORD dwRead = 0;
        ReadConsoleInputA(hIn, rc, 256, &dwRead);
        handleManyEvents(rc, dwRead);
    }
}


void CreateIOThreads()
{
    // The VtConsoles themselves handle their output threads.

    DWORD dwInputThreadId = (DWORD) -1;
    HANDLE hInputThread = CreateThread(nullptr,
                                        0,
                                        (LPTHREAD_START_ROUTINE)InputThread,
                                        nullptr,
                                        0,
                                        &dwInputThreadId);
    hInputThread;
}


BOOL CtrlHandler( DWORD fdwCtrlType ) 
{
    switch( fdwCtrlType ) 
    { 
    // Handle the CTRL-C signal. 
    case CTRL_C_EVENT: 
    case CTRL_BREAK_EVENT: 
        return true;
    }

    return false;
}

// this function has unreachable code due to its unusual lifetime. We
// disable the warning about it here.
#pragma warning(push)
#pragma warning(disable:4702)
int __cdecl wmain(int /*argc*/, WCHAR* /*argv[]*/)
{
    // initialize random seed: 
    srand((unsigned int)time(NULL));
    SetConsoleCtrlHandler( (PHANDLER_ROUTINE) CtrlHandler, TRUE );

    hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    hIn = GetStdHandle(STD_INPUT_HANDLE);

    SetupOutput();
    SetupInput();

    newConsole();  
    getConsole()->activate();
    CreateIOThreads();

    // Create a debug console for writting debugging output to.
    debug = new VtConsole(DebugReadCallback);
    // Echo stdin to stdout, but ignore newlines (so cat doesn't echo the input)
    debug->spawn(L"ubuntu run tr -d '\n' | cat -sA");
    debug->activate();

    // Exit the thread so the CRT won't clean us up and kill. The IO thread owns the lifetime now.
    ExitThread(S_OK);
    // We won't hit this. The ExitThread above will kill the caller at this point.
    assert(false);
    return 0;
}
#pragma warning(pop)
