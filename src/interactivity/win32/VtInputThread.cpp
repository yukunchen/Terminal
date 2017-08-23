/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "VtInputThread.hpp"

#include "..\inc\ServiceLocator.hpp"
#include "..\..\host\input.h"
#include "..\..\terminal\parser\InputStateMachineEngine.hpp"
#include <string>
#include <sstream>

using namespace Microsoft::Console::Interactivity::Win32;

// hack
typedef struct _VT_TO_VK {
    std::string vtSeq;
    short vkey;
    WORD vsc;
} VT_TO_VK;

VT_TO_VK vtMap[] = {
    {"\x1b[A", VK_UP, VK_UP},
    {"\x1b[B", VK_DOWN, VK_DOWN},
    {"\x1b[C", VK_RIGHT, VK_RIGHT},
    {"\x1b[D", VK_LEFT, VK_LEFT},
    {"\x1b[H", VK_HOME, VK_HOME},
    {"\x1b[F", VK_END, VK_END},
    {"\x7f", VK_BACK, VK_BACK},
    {"\xd", VK_RETURN, VK_RETURN},
    {"\x1bOP", VK_F1, VK_F1},
    {"\x1bOQ", VK_F2, VK_F2},
    {"\x1bOR", VK_F3, VK_F3},
    {"\x1bOS", VK_F4, VK_F4},
    {"\x1b[3~", VK_DELETE, VK_DELETE},
    {"\x1b[1;5A", 'A', 'A'}, // C-Up
    {"\x1b\x31", '1', '1'}, // ESC+1, alt+1
    {"\x1b[", 'x', 'x'}, // ESC+1, alt+1
};
// /hack

// This is copied from ConsolInformation.cpp
// I've never thought that was a particularily good place for it...
void _HandleTerminalKeyEventCallback(_In_reads_(cInput) INPUT_RECORD* rgInput, _In_ DWORD cInput)
{
    // auto _cIn = cInput;
    // ServiceLocator::LocateGlobals()->getConsoleInformation()->
    //     pInputBuffer->PrependInputBuffer(rgInput, &_cIn);

    // FIXME
    // The prototype fix moves the VT translation to WriteInputBuffer. 
    //   This currently causes WriteInputBuffer to get called twice for every 
    //   key - not ideal. There needs to be a WriteInputBuffer that sidesteps this problem.
    ServiceLocator::LocateGlobals()->getConsoleInformation()->
        pInputBuffer->WriteInputBuffer(rgInput, cInput);
}

// std::stringstream vtStream;
// std::string vtBuffer;

char vtCharBuffer[16];
int nextBufferIndex=0;

StateMachine* inputStateMachine;

void sendKey(char ch, short vkey, bool keyup)
{
    auto gci = ServiceLocator::LocateGlobals()->getConsoleInformation();

    INPUT_RECORD InputEvent;
    bool bGenerateBreak;
    InputEvent.EventType = KEY_EVENT;
    InputEvent.Event.KeyEvent.bKeyDown = true;
    InputEvent.Event.KeyEvent.wRepeatCount = 1;
    InputEvent.Event.KeyEvent.wVirtualKeyCode = vkey;
    // InputEvent.Event.KeyEvent.wVirtualScanCode = (WORD)MapVirtualKey(vkey, MAPVK_VK_TO_VSC);
    InputEvent.Event.KeyEvent.wVirtualScanCode = 0;
    InputEvent.Event.KeyEvent.dwControlKeyState = 0;//ENHANCED_KEY;
    InputEvent.Event.KeyEvent.uChar.UnicodeChar = (wchar_t)ch;
    // InputEvent.Event.KeyEvent.uChar.AsciiChar = ch;
    bGenerateBreak = false;
    // N.B.: This call passes InputEvent by value.
    // HandleGenericKeyEvent(InputEvent, bGenerateBreak);
        
    // EventsWritten = ServiceLocator::LocateGlobals()->getConsoleInformation()->pInputBuffer->WriteInputBuffer(&InputEvent, 1);
    gci->pInputBuffer->WriteInputBuffer(&InputEvent, 1);

    if (keyup)
    {
        InputEvent.Event.KeyEvent.bKeyDown = false;
        // HandleGenericKeyEvent(InputEvent, bGenerateBreak);
        gci->pInputBuffer->WriteInputBuffer(&InputEvent, 1);
    }
}

void sendChar(char ch, bool keyup)
{
    short vkey = VkKeyScan(ch);
    // sendKey(ch, vkey, keyup);
    // sendKey((char)toupper(ch), vkey, keyup);
    sendKey(ch, (char)toupper(ch), keyup); vkey;
}

void handleInputFromPipe(char ch)
{
    if (nextBufferIndex > 0 || ch == '\x7f')
    {
        vtCharBuffer[nextBufferIndex] = ch;
        nextBufferIndex++;

        int maxLen = 0;
        bool found = false;
        for (int i = 0; i < ARRAYSIZE(vtMap); i++)
        {
            VT_TO_VK mapping = vtMap[i];

            if (0==mapping.vtSeq.compare(std::string(vtCharBuffer, nextBufferIndex)))
            {
                sendKey(0, mapping.vkey, false);
                nextBufferIndex = 0;
                found = true;
                break;
            }
            else
            {
                maxLen = (int)(maxLen > mapping.vtSeq.length()? maxLen : mapping.vtSeq.length());
            }
        }

        if (!found && nextBufferIndex > maxLen)
        {
            for (int i = 0; i < nextBufferIndex; i++)
            {
                sendChar(vtCharBuffer[i], true);
            }
            nextBufferIndex = 0;
        }
    }
    else if (ch == '\x1b')
    {
        vtCharBuffer[nextBufferIndex] = ch;
        nextBufferIndex++;
    }
    else
    {
        sendChar(ch, true);
    }
        // sendChar(ch, true);
}

void handleRunInput(char* charBuffer, int cch)
{
    std::string inputSequernce = std::string(charBuffer, cch);
    bool found = false;
    
    for (int i = 0; i < ARRAYSIZE(vtMap); i++)
    {
        VT_TO_VK mapping = vtMap[i];
        if (0==mapping.vtSeq.compare(inputSequernce))
        {
            sendKey(0, mapping.vkey, false);
            found = true;
            break;
        }
    }

    if (!found)
    {
        for (int i = 0; i < inputSequernce.length(); i++)
        {
            sendChar(inputSequernce[i], true);
        }
    }
}

DWORD VtInputThreadProc(LPVOID /*lpParameter*/)
{
    // DebugBreak();
    // wil::unique_handle pipe;
    // pipe.reset(CreateNamedPipeW(L"\\\\.\\pipe\\convtinpipe", PIPE_ACCESS_DUPLEX, PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT, 1, 0, 0, 0, nullptr));
    // pipe.reset(CreateNamedPipeW(L"\\\\.\\pipe\\convtinpipe", PIPE_ACCESS_INBOUND, PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT, 1, 0, 0, 0, nullptr));
    // THROW_IF_HANDLE_INVALID(pipe.get());
    HANDLE hVT = CreateFileW(L"\\\\.\\pipe\\convtinpipe", GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

    inputStateMachine = new StateMachine(new InputStateMachineEngine(_HandleTerminalKeyEventCallback));

    // THROW_LAST_ERROR_IF_FALSE(ConnectNamedPipe(pipe.get(), nullptr));

    byte buffer[256];
    DWORD dwRead;
    while (true)
    {
        dwRead = 0;
        // THROW_LAST_ERROR_IF_FALSE(ReadFile(pipe.get(), buffer, ARRAYSIZE(buffer), &dwRead, nullptr));
        THROW_LAST_ERROR_IF_FALSE(ReadFile(hVT, buffer, ARRAYSIZE(buffer), &dwRead, nullptr));

        // handleRunInput((char*)buffer, dwRead);
        // For debugging, zero mem
        // ZeroMemory(buffer, ARRAYSIZE(buffer)*sizeof(*buffer));


        std::string inputSequernce = std::string((char*)buffer, dwRead);
        auto cch = inputSequernce.length() + 1;
        wchar_t* wc = new wchar_t[cch];
        // mbstowcs(wc, inputSequernce.c_str(), cch);
        size_t numConverted = 0;
        mbstowcs_s(&numConverted, wc, cch, inputSequernce.c_str(), cch);
        inputStateMachine->ProcessString(wc, cch);
        
        // for (DWORD i = 0; i < dwRead; i++)
        // {
        //     handleInputFromPipe(buffer[i]);
        // }
    }

}

// Routine Description:
// - Starts the Win32-specific console input thread.
HANDLE VtInputThread::Start()
{
    HANDLE hThread = nullptr;
    DWORD dwThreadId = (DWORD) -1;

    hThread = CreateThread(nullptr,
                           0,
                           (LPTHREAD_START_ROUTINE)VtInputThreadProc,
                           nullptr,
                           0,
                           &dwThreadId);

    if (hThread)
    {
        _hThread = hThread;
        _dwThreadId = dwThreadId;
    }

    return hThread;
}