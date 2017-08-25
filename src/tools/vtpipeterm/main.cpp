//
//    Copyright (C) Microsoft.  All rights reserved.
//
#include <windows.h>
#include <wil\result.h>
#include <wil\resource.h>
#include <wil\wistd_functional.h>
#include <wil\wistd_memory.h>

#include <string>
#include <assert.h>

#define IN_PIPE_NAME L"\\\\.\\pipe\\convtinpipe"
#define OUT_PIPE_NAME L"\\\\.\\pipe\\convtoutpipe"
#define PIPE_NAME L"\\\\.\\pipe\\convtpipe"
using namespace std;
////////////////////////////////////////////////////////////////////////////////
// State
// wil::unique_handle outPipe;
// wil::unique_handle inPipe;
HANDLE outPipe;
HANDLE inPipe;
HANDLE Pipe;
HANDLE hOut;
HANDLE hIn;

////////////////////////////////////////////////////////////////////////////////


// bool openVtPipeIn()
// {
//     wchar_t commandline[] = L"/c start .\\VtPipeIn.exe";
//     PROCESS_INFORMATION pi = {0};
//     STARTUPINFO si = {0};
//     si.cb = sizeof(STARTUPINFOW);
//     bool fSuccess = !!CreateProcess(
//         L"cmd.exe",
//         commandline,
//         nullptr,    // lpProcessAttributes
//         nullptr,    // lpThreadAttributes
//         false,      // bInheritHandles
//         0,          // dwCreationFlags
//         nullptr,    // lpEnvironment
//         nullptr,    // lpCurrentDirectory
//         &si,        //lpStartupInfo
//         &pi         //lpProcessInformation
//     );


//     if (!fSuccess)
//     {
//         wprintf(L"Failed to launch VtPipeIn\n");
//     }
//     return fSuccess;
// }


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

            *nextBuffer = c;
            nextBuffer++;
            bufferCch++;

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
        // WriteFile(inPipe, vtseq.c_str(), (DWORD)vtseq.length(), nullptr, nullptr);
        WriteFile(Pipe, vtseq.c_str(), (DWORD)vtseq.length(), nullptr, nullptr);
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

    byte buffer[256];
    DWORD dwRead;
    while (true)
    {
        dwRead = 0;
        // THROW_LAST_ERROR_IF_FALSE(ReadFile(outPipe.get(), buffer, ARRAYSIZE(buffer), &dwRead, nullptr));
        // THROW_LAST_ERROR_IF_FALSE(ReadFile(outPipe, buffer, ARRAYSIZE(buffer), &dwRead, nullptr));
        THROW_LAST_ERROR_IF_FALSE(ReadFile(Pipe, buffer, ARRAYSIZE(buffer), &dwRead, nullptr));
        THROW_LAST_ERROR_IF_FALSE(WriteFile(hOut, buffer, dwRead, nullptr, nullptr));
    }
    // return 0;
}

DWORD InputThread(LPVOID lpParameter)
{
    // THROW_LAST_ERROR_IF_FALSE(ConnectNamedPipe(inPipe, nullptr));
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
    // return 0;
}

DWORD IOThread(LPVOID lpParameter)
{
    UNREFERENCED_PARAMETER(lpParameter);
    
    THROW_LAST_ERROR_IF_FALSE(ConnectNamedPipe(Pipe, nullptr));


    // OutputThread(nullptr);
    DWORD dwOutputThreadId = (DWORD) -1;
    HANDLE hOutputThread = CreateThread(nullptr,
                                        0,
                                        (LPTHREAD_START_ROUTINE)OutputThread,
                                        nullptr,
                                        0,
                                        &dwOutputThreadId);
    hOutputThread;


    // InputThread(nullptr);
    DWORD dwInputThreadId = (DWORD) -1;
    HANDLE hInputThread = CreateThread(nullptr,
                                        0,
                                        (LPTHREAD_START_ROUTINE)InputThread,
                                        nullptr,
                                        0,
                                        &dwInputThreadId);
    hInputThread;

    return 0;
}

bool _openCon(std::wstring cmdline)
{
    // wchar_t commandline[] = L"OpenConsole.exe --pipe " PIPE_NAME;
    PROCESS_INFORMATION pi = {0};
    STARTUPINFO si = {0};
    si.cb = sizeof(STARTUPINFOW);
    bool fSuccess = !!CreateProcess(
        nullptr,
        // commandline,
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

bool openConsole(std::wstring PipeName)
{
    std::wstring cmdline = L"OpenConsole.exe --pipe ";
    cmdline += PipeName;
    return _openCon(cmdline);
}

bool openConsole(std::wstring inPipeName, std::wstring outPipeName)
{
    std::wstring cmdline = L"OpenConsole.exe --inpipe ";
    cmdline += inPipeName;
    cmdline += L" --outpipe ";
    cmdline += outPipeName;
    return _openCon(cmdline);
}


// this function has unreachable code due to its unusual lifetime. We
// disable the warning about it here.
#pragma warning(push)
#pragma warning(disable:4702)
int __cdecl wmain(int /*argc*/, WCHAR* /*argv[]*/)
{
    hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    hIn = GetStdHandle(STD_INPUT_HANDLE);

// PIPE_ACCESS_INBOUND
// PIPE_ACCESS_OUTBOUND
// PIPE_ACCESS_DUPLEX

    // // inPipe.reset(
    // inPipe = (
    //     // CreateNamedPipeW(IN_PIPE_NAME, PIPE_ACCESS_OUTBOUND, PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT, 1, 0, 0, 0, nullptr)
    //     CreateNamedPipeW(IN_PIPE_NAME, PIPE_ACCESS_DUPLEX, PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT, 1, 0, 0, 0, nullptr)
    // );
    
    // // outPipe.reset(
    // outPipe = (
    //     // CreateNamedPipeW(OUT_PIPE_NAME, PIPE_ACCESS_INBOUND, PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT, 1, 0, 0, 0, nullptr)
    //     CreateNamedPipeW(OUT_PIPE_NAME, PIPE_ACCESS_DUPLEX, PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT, 1, 0, 0, 0, nullptr)
    // );

    // outPipe.reset(
    Pipe = (
        // CreateNamedPipeW(OUT_PIPE_NAME, PIPE_ACCESS_INBOUND, PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT, 1, 0, 0, 0, nullptr)
        CreateNamedPipeW(PIPE_NAME, PIPE_ACCESS_DUPLEX, PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT, 1, 0, 0, 0, nullptr)
    );

    // THROW_IF_HANDLE_INVALID(inPipe.get());
    // THROW_IF_HANDLE_INVALID(outPipe.get());
    // THROW_IF_HANDLE_INVALID(inPipe);
    // THROW_IF_HANDLE_INVALID(outPipe);
    THROW_IF_HANDLE_INVALID(Pipe);

    // Open our backing console
    // openVtPipeIn();
    // Sleep(100);

    // THROW_LAST_ERROR_IF_FALSE(ConnectNamedPipe(inPipe.get(), nullptr));
    // THROW_LAST_ERROR_IF_FALSE(ConnectNamedPipe(outPipe.get(), nullptr));

    // OutputThread(nullptr);

    // DWORD dwOutputThreadId = (DWORD) -1;
    // HANDLE hOutputThread = CreateThread(nullptr,
    //                                     0,
    //                                     (LPTHREAD_START_ROUTINE)OutputThread,
    //                                     nullptr,
    //                                     0,
    //                                     &dwOutputThreadId);
    // hOutputThread;



    // DWORD dwInputThreadId = (DWORD) -1;
    // HANDLE hInputThread = CreateThread(nullptr,
    //                                     0,
    //                                     (LPTHREAD_START_ROUTINE)InputThread,
    //                                     nullptr,
    //                                     0,
    //                                     &dwInputThreadId);
    // hInputThread;

    DWORD dwInputThreadId = (DWORD) -1;
    HANDLE hInputThread = CreateThread(nullptr,
                                        0,
                                        (LPTHREAD_START_ROUTINE)IOThread,
                                        nullptr,
                                        0,
                                        &dwInputThreadId);
    hInputThread;

    // openConsole(IN_PIPE_NAME, OUT_PIPE_NAME);
    openConsole(PIPE_NAME);

    // Exit the thread so the CRT won't clean us up and kill. The IO thread owns the lifetime now.
    ExitThread(S_OK);
    // We won't hit this. The ExitThread above will kill the caller at this point.
    assert(false);
    return 0;
}
#pragma warning(pop)
