/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "VtInputThread.hpp"

#include "../interactivity/inc/ServiceLocator.hpp"
#include "input.h"
#include "../terminal/parser/InputStateMachineEngine.hpp"
#include "outputStream.hpp" // For ConhostInternalGetSet
#include "../terminal/adapter/InteractDispatch.hpp"
#include "misc.h"
#include "server.h"

using namespace Microsoft::Console;

void _HandleTerminalKeyEventCallback(_In_ std::deque<std::unique_ptr<IInputEvent>>& inEvents)
{
    const CONSOLE_INFORMATION* const gci = ServiceLocator::LocateGlobals()->getConsoleInformation();
    gci->pInputBuffer->Write(inEvents);
}

// Constructor Description:
// - Creates the VT Input Thread.
// Arguments:
// - hPipe - a handle to the file representing the read end of the VT pipe. 
VtInputThread::VtInputThread(_In_ wil::unique_hfile hPipe)
    : _hFile(std::move(hPipe))
{
    THROW_IF_HANDLE_INVALID(_hFile.get());



}

// Method Description:
// - Processes a buffer of input characters. The characters should be utf-8 
//      encoded, and will get converted to wchar_t's to be processed by the 
//      input state machine. 
// Arguments:
// - charBuffer - the UTF-8 characters recieved.
// - cch - number of UTF-8 characters in charBuffer
// Return Value:
// - S_OK on success, otherwise an appropriate failure.
HRESULT VtInputThread::_HandleRunInput(_In_reads_(cch) const char* const charBuffer, _In_ const int cch)
{

    CONSOLE_INFORMATION* const gci = ServiceLocator::LocateGlobals()->getConsoleInformation();
    gci->LockConsole();
    auto Unlock = wil::ScopeExit([&] { gci->UnlockConsole(); });
    
    unsigned int const uiCodePage = gci->CP;
    try
    {
        wistd::unique_ptr<wchar_t[]> pwsSequence;
        size_t cchSequence;
        RETURN_IF_FAILED(ConvertToW(uiCodePage, charBuffer, cch, pwsSequence, cchSequence));

        _pInputStateMachine->ProcessString(pwsSequence.get(), cchSequence);
    }
    CATCH_RETURN();

    return S_OK;
}

// Function Description:
// - Static function used for initializing an instance's ThreadProc.
// Arguments:
// - lpParameter - A pointer to the VtInputThread instance that should be called.
// Return Value:
// - The return value of the underlying instance's _InputThread
DWORD VtInputThread::StaticVtInputThreadProc(_In_ LPVOID lpParameter)
{
    VtInputThread* const pInstance = reinterpret_cast<VtInputThread*>(lpParameter);
    return pInstance->_InputThread();
}

// Method Description:
// - The ThreadProc for the VT Input Thread. Reads input from the pipe, and 
//      passes it to _HandleRunInput to be processed by the 
//      InputStateMachineEngine.
// Return Value:
// - <none>
DWORD VtInputThread::_InputThread()
{
    char buffer[256];
    DWORD dwRead;
    while (true)
    {
        dwRead = 0;
        THROW_LAST_ERROR_IF_FALSE(ReadFile(_hFile.get(), buffer, ARRAYSIZE(buffer), &dwRead, nullptr));

        THROW_IF_FAILED(_HandleRunInput(buffer, dwRead));
    }
}

// Method Description:
// - Starts the VT input thread.
HRESULT VtInputThread::Start()
{
    RETURN_IF_HANDLE_INVALID(_hFile.get());
    
    //Initialize the state machine here, because the gci->pInputBuffer 
    // hasn't been initialized when we initialize the VtInputThread object.
    const CONSOLE_INFORMATION* const gci = ServiceLocator::LocateGlobals()->getConsoleInformation();
    
    std::unique_ptr<ConhostInternalGetSet> pGetSet = 
        std::make_unique<ConhostInternalGetSet>(gci->CurrentScreenBuffer, gci->pInputBuffer);
    THROW_IF_NULL_ALLOC(pGetSet);

    InteractDispatch* _pDispatch;
    THROW_HR_IF_FALSE(E_OUTOFMEMORY, InteractDispatch::CreateInstance(std::move(pGetSet), &_pDispatch));
    std::unique_ptr<InteractDispatch> pDispatch = std::unique_ptr<InteractDispatch>(_pDispatch);
    
    std::unique_ptr<InputStateMachineEngine> pEngine = 
        std::make_unique<InputStateMachineEngine>(std::move(pDispatch));
    THROW_IF_NULL_ALLOC(pEngine);

    _pInputStateMachine = std::make_unique<StateMachine>(std::move(pEngine));
    THROW_IF_NULL_ALLOC(_pInputStateMachine);
    
    HANDLE hThread = nullptr;
    // 0 is the right value, https://blogs.msdn.microsoft.com/oldnewthing/20040223-00/?p=40503
    DWORD dwThreadId = 0;

    hThread = CreateThread(nullptr,
                           0,
                           (LPTHREAD_START_ROUTINE)VtInputThread::StaticVtInputThreadProc,
                           this,
                           0,
                           &dwThreadId);

    RETURN_IF_HANDLE_INVALID(hThread);
    _hThread.reset(hThread);
    _dwThreadId = dwThreadId;

    return S_OK;
}
