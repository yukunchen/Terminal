

#include <windows.h>
#include <wil\result.h>
#include <wil\resource.h>

#include <string>

class VtConsole
{
public:
    VtConsole();
    void spawn();

    HANDLE inPipe();
    HANDLE outPipe();

    static const DWORD sInPipeOpenMode = PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED;
    static const DWORD sOutPipeOpenMode = PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED;

    static const DWORD sInPipeMode = PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT;
    static const DWORD sOutPipeMode = PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT;

    DWORD getReadOffset();
    void incrementReadOffset(DWORD offset);

    void activate();
    void deactivate();


private:
    PROCESS_INFORMATION pi;
    HANDLE _outPipe;
    HANDLE _inPipe;
    std::wstring _inPipeName;
    std::wstring _outPipeName;
    bool _connected = false;

    DWORD _offset = 0;

    bool _active = false;

    void _openConsole1();
    void _openConsole2();

    void _spawn1();
    void _spawn2();



};