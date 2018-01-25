/*++
Copyright (c) Microsoft Corporation

Module Name:
- VtConsole.hpp

Abstract:
- This serves as an abstraction to allow for a test connection to a conhost.exe running
  in VT server mode. It's abstracted to allow multiple simultaneous connections to multiple
  conhost.exe servers.

Author(s):
- Mike Griese (MiGrie) 2017
--*/


#include <windows.h>
#include <wil\result.h>
#include <wil\resource.h>

#include <string>


typedef void(*PipeReadCallback)(BYTE* buffer, DWORD dwRead);

class VtConsole
{
public:
    VtConsole(PipeReadCallback const pfnReadCallback, bool const fHeadless, COORD const initialSize);
    void spawn();
    void spawn(const std::wstring& command);
    
    HANDLE inPipe();
    HANDLE outPipe();

    static const DWORD sInPipeOpenMode = PIPE_ACCESS_DUPLEX;
    static const DWORD sOutPipeOpenMode = PIPE_ACCESS_INBOUND;

    static const DWORD sInPipeMode = PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT;
    static const DWORD sOutPipeMode = PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT;

    void activate();
    void deactivate();

    void signalWindow(unsigned short sx, unsigned short sy);
    
    static DWORD StaticOutputThreadProc(LPVOID lpParameter);

    bool WriteInput(std::string& seq);

    bool Repaint();
    bool Resize(const unsigned int rows, const unsigned int cols);

private:
    COORD _lastDimensions;

    PROCESS_INFORMATION pi;

    HANDLE _outPipe;
    HANDLE _inPipe;
    HANDLE _signalPipe;
    std::wstring _inPipeName;
    std::wstring _outPipeName;
    
    bool _connected = false;
    bool _active = false;
    bool _fHeadless = false;

    PipeReadCallback _pfnReadCallback;

    DWORD _dwOutputThreadId;
    HANDLE _hOutputThread = INVALID_HANDLE_VALUE;

    void _openConsole2(const std::wstring& command);
    void _openConsole3(const std::wstring& command);

    void _spawn2(const std::wstring& command);
    void _spawn3(const std::wstring& command);

    DWORD _OutputThread();

};
