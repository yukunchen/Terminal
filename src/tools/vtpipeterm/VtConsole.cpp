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



void VtConsole::spawn()
{

    _inPipe = (
        CreateNamedPipeW(_inPipeName.c_str(), sInPipeOpenMode, sInPipeMode, 1, 0, 0, 0, nullptr)
    );
    
    _outPipe = (
        CreateNamedPipeW(_outPipeName.c_str(), sOutPipeOpenMode, sOutPipeMode, 1, 0, 0, 0, nullptr)
    );

    THROW_IF_HANDLE_INVALID(_inPipe);
    THROW_IF_HANDLE_INVALID(_outPipe);

    _openConsole();
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

void VtConsole::_openConsole()
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
