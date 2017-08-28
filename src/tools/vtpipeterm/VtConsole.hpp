

#include <windows.h>
#include <wil\result.h>
#include <wil\resource.h>

#include <string>

class VtConsole
{
public:
    VtConsole();
    void spawn();
    void _openConsole();
    HANDLE _outPipe;
    HANDLE _inPipe;
    PROCESS_INFORMATION pi;
    std::wstring _inPipeName;
    std::wstring _outPipeName;
    bool _connected = false;

    static const DWORD sInPipeOpenMode = PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED;
    static const DWORD sOutPipeOpenMode = PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED;

    static const DWORD sInPipeMode = PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT;
    static const DWORD sOutPipeMode = PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT;

};