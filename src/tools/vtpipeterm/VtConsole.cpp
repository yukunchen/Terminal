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
#include <iomanip>
#include <assert.h>
// Defined inside the console host PTY signal thread.
#define PTY_SIGNAL_RESIZE_WINDOW 8u

VtConsole::VtConsole(PipeReadCallback const pfnReadCallback,
                     bool const fHeadless,
                     COORD const initialSize)
{
    _pfnReadCallback = pfnReadCallback;
    _fHeadless = fHeadless;
    _lastDimensions = initialSize;

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

void VtConsole::signalWindow(unsigned short sx, unsigned short sy)
{
    unsigned short signalPacket[3];
    signalPacket[0] = PTY_SIGNAL_RESIZE_WINDOW;
    signalPacket[1] = sx;
    signalPacket[2] = sy;

    WriteFile(_signalPipe, signalPacket, sizeof(signalPacket), nullptr, nullptr);
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

    // Create some anon pipes so we can pass handles down and into the console.
    // IMPORTANT NOTE:
    // We're creating the pipe here with un-inheritable handles, then marking 
    //      the conhost sides of the pipes as inheritable. We do this because if
    //      the entire pipe is marked as inheritable, when we pass the handles 
    //      to CreateProcess, at some point the entire pipe object is copied to
    //      the conhost process, which includes the terminal side of the pipes 
    //      (_inPipe and _outPipe). This means that if we die, there's still 
    //      outstanding handles to our side of the pipes, and those handles are
    //      in conhost, despite conhost being unable to reference those handles 
    //      and close them.

    // CRITICAL: Close our side of the handles. Otherwise you'll get the same 
    //      problem if you close conhost, but not us (the terminal).
    // The conhost sides of the pipe will be unique_hfile's so that they'll get 
    //      closed automatically at the end of the method.
    wil::unique_hfile outPipeConhostSide;
    wil::unique_hfile inPipeConhostSide;
    wil::unique_hfile signalPipeConhostSide;

    SECURITY_ATTRIBUTES sa;
    sa = { 0 };
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = FALSE;
    sa.lpSecurityDescriptor = nullptr;

    THROW_IF_WIN32_BOOL_FALSE(CreatePipe(&inPipeConhostSide, &_inPipe, &sa, 0));
    THROW_IF_WIN32_BOOL_FALSE(CreatePipe(&_outPipe, &outPipeConhostSide, &sa, 0));

    // Mark inheritable for signal handle when creating. It'll have the same value on the other side.
    sa.bInheritHandle = TRUE;
    THROW_IF_WIN32_BOOL_FALSE(CreatePipe(&signalPipeConhostSide, &_signalPipe, &sa, 0));

    THROW_IF_WIN32_BOOL_FALSE(SetHandleInformation(inPipeConhostSide.get(), HANDLE_FLAG_INHERIT, 1));
    THROW_IF_WIN32_BOOL_FALSE(SetHandleInformation(outPipeConhostSide.get(), HANDLE_FLAG_INHERIT, 1));

    STARTUPINFO si = { 0 };
    si.cb = sizeof(STARTUPINFOW);
    si.hStdInput = inPipeConhostSide.get();
    si.hStdOutput = outPipeConhostSide.get();
    si.hStdError = outPipeConhostSide.get();
    si.dwFlags |= STARTF_USESTDHANDLES;

    if(!(_lastDimensions.X == 0 && _lastDimensions.Y == 0))
    {
        // STARTF_USECOUNTCHARS does not work. 
        // minkernel/console/client/dllinit will write that value to conhost 
        //  during init of a cmdline application, but because we're starting
        //  conhost directly, that doesn't work for us.
        std::wstringstream ss;
        ss << L" --width " << _lastDimensions.X;
        ss << L" --height " << _lastDimensions.Y;
        cmdline += ss.str();
    }

    // Attach signal handle ID onto command line using string stream for formatting
    std::wstringstream signalArg;
    signalArg << L" --signal 0x" << std::hex << HandleToUlong(signalPipeConhostSide.get());
    cmdline += signalArg.str();

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
        true,       // bInheritHandles
        0,          // dwCreationFlags
        nullptr,    // lpEnvironment
        nullptr,    // lpCurrentDirectory
        &si,        // lpStartupInfo
        &pi         // lpProcessInformation
    );

    if (!fSuccess)
    {
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        std::string msg = "Failed to launch Openconsole";
        WriteFile(hOut, msg.c_str(), (DWORD)msg.length(), nullptr, nullptr);
    }
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

bool VtConsole::Repaint()
{
    std::string seq = "\x1b[7t";
    return WriteInput(seq);
}

bool VtConsole::Resize(const unsigned int rows, const unsigned int cols)
{
    std::stringstream ss;
    ss << "\x1b[8;" << rows << ";" << cols << "t";
    std::string seq = ss.str();
    return WriteInput(seq);
}

