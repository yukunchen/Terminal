//
//    Copyright (C) Microsoft.  All rights reserved.
//

#include "VtConsole.hpp"

#include <stdlib.h>     /* srand, rand */
#include <time.h>       /* time */

#include <deque>
#include <vector>
#include <string>
#include <sstream>
#include <assert.h>

VtConsole::VtConsole(PipeReadCallback const pfnReadCallback, bool const fHeadless)
{
    _pfnReadCallback = pfnReadCallback;
    _fHeadless = fHeadless;

    int r = rand();
    std::wstringstream ss;
    ss << r;
    std::wstring randString;
    ss >> randString;

    _inPipeName = L"\\\\.\\pipe\\convt-in-" + randString;
    _outPipeName = L"\\\\.\\pipe\\convt-out-" + randString;
}

HANDLE VtConsole::inPipe()
{
    return _inPipe;
}
HANDLE VtConsole::outPipe()
{
    return _outPipe;
}

bool VtConsole::WriteInput(std::string& seq)
{
    bool fSuccess = !!WriteFile(inPipe(), seq.c_str(), (DWORD)seq.length(), nullptr, nullptr);
    if (!fSuccess)
    {
        HRESULT hr = GetLastError();
        exit(hr);
    }
    return fSuccess;
}

void VtConsole::spawn()
{
    _spawn3(L"");
}

void VtConsole::spawn(const std::wstring& command)
{
    _spawn3(command);
}

void VtConsole::_spawn2(const std::wstring& command)
{

    _inPipe = (
        CreateNamedPipeW(_inPipeName.c_str(), sInPipeOpenMode, sInPipeMode, 1, 0, 0, 0, nullptr)
    );
    
    _outPipe = (
        CreateNamedPipeW(_outPipeName.c_str(), sOutPipeOpenMode, sOutPipeMode, 1, 0, 0, 0, nullptr)
    );

    THROW_IF_HANDLE_INVALID(_inPipe);
    THROW_IF_HANDLE_INVALID(_outPipe);

    _openConsole2(command);
    bool fSuccess = !!ConnectNamedPipe(_inPipe, nullptr);
    if (!fSuccess)
    {
        DWORD lastError = GetLastError();
        if (lastError != ERROR_PIPE_CONNECTED) THROW_LAST_ERROR_IF_FALSE(fSuccess); 
    }

    fSuccess = !!ConnectNamedPipe(_outPipe, nullptr);
    if (!fSuccess)
    {
        DWORD lastError = GetLastError();
        if (lastError != ERROR_PIPE_CONNECTED) THROW_LAST_ERROR_IF_FALSE(fSuccess); 
    }

    _connected = true;


    // Create our own output handling thread
    // Each console needs to make sure to drain the output from it's backing host.
    _dwOutputThreadId = (DWORD) -1;
    _hOutputThread = CreateThread(nullptr,
                                  0,
                                  (LPTHREAD_START_ROUTINE)StaticOutputThreadProc,
                                  this,
                                  0,
                                  &_dwOutputThreadId);
}

void VtConsole::_spawn3(const std::wstring& command)
{
    SECURITY_ATTRIBUTES sa;
    sa = { 0 };
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = nullptr;

    // Create some anon pipes so we can pass handles down and into the openconsole/console host.
    THROW_IF_WIN32_BOOL_FALSE(CreatePipe(&_inPipeConhostSide, &_inPipe, &sa, 0));
    THROW_IF_WIN32_BOOL_FALSE(CreatePipe(&_outPipe, &_outPipeConhostSide, &sa, 0));
    
    _openConsole3(command);
    
    _connected = true;
    
    // Create our own output handling thread
    // Each console needs to make sure to drain the output from it's backing host.
    _dwOutputThreadId = (DWORD)-1;
    _hOutputThread = CreateThread(nullptr,
                                  0,
                                  (LPTHREAD_START_ROUTINE)StaticOutputThreadProc,
                                  this,
                                  0,
                                  &_dwOutputThreadId);
}

PCWSTR GetCmdLine()
{
#ifdef __INSIDE_WINDOWS
    return L"conhost.exe";
#else
    return L"OpenConsole.exe";
#endif
}

void VtConsole::_openConsole2(const std::wstring& command)
{
    std::wstring cmdline(GetCmdLine());

    if (_inPipeName.length() > 0)
    {
        cmdline += L" --inpipe ";
        cmdline += _inPipeName;
    }
    if (_outPipeName.length() > 0)
    {
        cmdline += L" --outpipe ";
        cmdline += _outPipeName;
    }
    
    if (_fHeadless)
    {
        cmdline += L" --headless";
    }

    STARTUPINFO si = {0};
    si.cb = sizeof(STARTUPINFOW);
    
    if (command.length() > 0)
    {
        cmdline += L" -- ";
        cmdline += command;
    }
    else 
    {
        // si.dwFlags = STARTF_USESHOWWINDOW;
        // si.wShowWindow = SW_MINIMIZE;
    }

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
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        std::string msg = "Failed to launch Openconsole";
        WriteFile(hOut, msg.c_str(), (DWORD)msg.length(), nullptr, nullptr);
    }
    fSuccess;
}

void VtConsole::_openConsole3(const std::wstring& command)
{
    std::wstring cmdline(GetCmdLine());

    if (_fHeadless)
    {
        cmdline += L" --headless";
    }

    STARTUPINFO si = { 0 };
    si.cb = sizeof(STARTUPINFOW);
    si.hStdInput = _inPipeConhostSide;
    si.hStdOutput = _outPipeConhostSide;
    si.dwFlags |= STARTF_USESTDHANDLES;
    

    if (command.length() > 0)
    {
        cmdline += L" -- ";
        cmdline += command;
    }
    else
    {
        si.dwFlags |= STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_MINIMIZE;
    }

    bool fSuccess = !!CreateProcess(
        nullptr,
        &cmdline[0],
        nullptr,    // lpProcessAttributes
        nullptr,    // lpThreadAttributes
        true,      // bInheritHandles
        0,          // dwCreationFlags
        nullptr,    // lpEnvironment
        nullptr,    // lpCurrentDirectory
        &si,        //lpStartupInfo
        &pi         //lpProcessInformation
    );

    if (!fSuccess)
    {
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        std::string msg = "Failed to launch Openconsole";
        WriteFile(hOut, msg.c_str(), (DWORD)msg.length(), nullptr, nullptr);
    }
    fSuccess;
}

DWORD VtConsole::getReadOffset()
{
    return _offset;
}

void VtConsole::incrementReadOffset(DWORD offset)
{
    _offset += offset;
}

void VtConsole::activate()
{
    _active = true;
}

void VtConsole::deactivate()
{
    _active = false;
}

DWORD VtConsole::StaticOutputThreadProc(LPVOID lpParameter)
{
    VtConsole* const pInstance = (VtConsole*)lpParameter;
    return pInstance->_OutputThread();
}

DWORD VtConsole::_OutputThread()
{
    BYTE buffer[256];
    DWORD dwRead;
    while (true)
    {
        dwRead = 0;
        bool fSuccess = false;

        fSuccess = !!ReadFile(this->outPipe(), buffer, ARRAYSIZE(buffer), &dwRead, nullptr);

        THROW_LAST_ERROR_IF_FALSE(fSuccess);
        if (this->_active)
        {
            _pfnReadCallback(buffer, dwRead);
        }
    }
}
