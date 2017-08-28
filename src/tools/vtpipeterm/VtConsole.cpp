#include "VtConsole.hpp"

#include <stdlib.h>     /* srand, rand */
#include <time.h>       /* time */

#include <deque>
#include <vector>
#include <string>
#include <sstream>
#include <assert.h>

VtConsole::VtConsole()
{
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

void VtConsole::spawn()
{
    _spawn1();
}

void VtConsole::_spawn1()
{
    _outPipeName = _inPipeName;
    _inPipe = (
        CreateNamedPipeW(_inPipeName.c_str(), sInPipeOpenMode, sInPipeMode, 1, 0, 0, 0, nullptr)
    );
    _outPipe = _inPipe;

    THROW_IF_HANDLE_INVALID(_inPipe);
    THROW_IF_HANDLE_INVALID(_outPipe);

    _openConsole1();

    bool fSuccess = !!ConnectNamedPipe(_inPipe, nullptr);
    if (!fSuccess)
    {
        DWORD lastError = GetLastError();
        if (lastError != ERROR_PIPE_CONNECTED) THROW_LAST_ERROR_IF_FALSE(fSuccess); 
    }

    _connected = true;
}

void VtConsole::_spawn2()
{

    _inPipe = (
        CreateNamedPipeW(_inPipeName.c_str(), sInPipeOpenMode, sInPipeMode, 1, 0, 0, 0, nullptr)
    );
    
    _outPipe = (
        CreateNamedPipeW(_outPipeName.c_str(), sOutPipeOpenMode, sOutPipeMode, 1, 0, 0, 0, nullptr)
    );

    THROW_IF_HANDLE_INVALID(_inPipe);
    THROW_IF_HANDLE_INVALID(_outPipe);

    _openConsole2();
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
}

void VtConsole::_openConsole1()
{
    std::wstring cmdline = L"OpenConsole.exe";
    if (_inPipeName.length() > 0)
    {
        cmdline += L" --pipe ";
        cmdline += _inPipeName;
    }
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
    fSuccess;
}

void VtConsole::_openConsole2()
{
    std::wstring cmdline = L"OpenConsole.exe";
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