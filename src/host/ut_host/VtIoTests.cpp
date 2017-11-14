/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/
#include "precomp.h"
#include <wextestclass.h>
#include "..\..\inc\consoletaeftemplates.hpp"
#include "..\..\types\inc\Viewport.hpp"

#include "..\..\renderer\vt\Xterm256Engine.hpp"
#include "..\..\renderer\vt\XtermEngine.hpp"
#include "..\..\renderer\vt\WinTelnetEngine.hpp"
#include "..\Settings.hpp"
#include "..\VtIo.hpp"

using namespace WEX::Common;
using namespace WEX::Logging;
using namespace WEX::TestExecution;
using namespace std;


class Microsoft::Console::VirtualTerminal::VtIoTests
{
    TEST_CLASS(VtIoTests);

    // General Tests:
    TEST_METHOD(NoOpStartTest);
    TEST_METHOD(ModeParsingTest);

    // Pipe Opening tests:
    TEST_METHOD(BasicPipeOpeningTest);
    TEST_METHOD(NoInPipeOpeningTest);
    TEST_METHOD(NoOutPipeOpeningTest);
    TEST_METHOD(NonOverlappedInModeTest);
    TEST_METHOD(NonDuplexInModeTest);
    TEST_METHOD(WrongInModeTest);
    TEST_METHOD(WrongOutModeTest);
    TEST_METHOD(MultipleInstanceTest);
    TEST_METHOD(OnePipeForBothTest);
    TEST_METHOD(OnePipeMultipleInstancesTest);
    TEST_METHOD(PipeAlreadyOpenedTest);
    TEST_METHOD(MultipleInstancesPipeAlreadyOpenedTest);
    
    TEST_METHOD(DtorTestJustEngine);
    TEST_METHOD(DtorTestDeleteVtio);
    TEST_METHOD(DtorTestStackAlloc);
    TEST_METHOD(DtorTestStackAllocMany);
    TEST_METHOD(DtorTestNonDuplexInModeTest);
    TEST_METHOD(DtorTestNonDuplexInModeTestModified);
};

using namespace Microsoft::Console::VirtualTerminal;
using namespace Microsoft::Console::Render;
using namespace Microsoft::Console::Types;

void VtIoTests::NoOpStartTest()
{
    VtIo* vtio = new VtIo();
    VERIFY_IS_NOT_NULL(vtio);
    VERIFY_IS_FALSE(vtio->IsUsingVt());

    Log::Comment(L"Verify we succeed at StartIfNeeded even if we weren't initialized");
    VERIFY_SUCCEEDED(vtio->StartIfNeeded());
}

void VtIoTests::ModeParsingTest()
{
    VtIoMode mode;
    VERIFY_SUCCEEDED(VtIo::ParseIoMode(L"xterm", mode));
    VERIFY_ARE_EQUAL(mode, VtIoMode::XTERM);

    VERIFY_SUCCEEDED(VtIo::ParseIoMode(L"xterm-256color", mode));
    VERIFY_ARE_EQUAL(mode, VtIoMode::XTERM_256);

    VERIFY_SUCCEEDED(VtIo::ParseIoMode(L"win-telnet", mode));
    VERIFY_ARE_EQUAL(mode, VtIoMode::WIN_TELNET);

    VERIFY_SUCCEEDED(VtIo::ParseIoMode(L"xterm-ascii", mode));
    VERIFY_ARE_EQUAL(mode, VtIoMode::XTERM_ASCII);

    VERIFY_SUCCEEDED(VtIo::ParseIoMode(L"", mode));
    VERIFY_ARE_EQUAL(mode, VtIoMode::XTERM_256);
    
    VERIFY_FAILED(VtIo::ParseIoMode(L"garbage", mode));
    VERIFY_ARE_EQUAL(mode, VtIoMode::INVALID);

}

void VtIoTests::BasicPipeOpeningTest()
{
    Log::Comment(L"Just the basic case, two seperate pipes, expected modes");
    // DUPLEX is definitely acceptable for the inpipe
    // overlapped is acceptable, though it will not be used.
    DWORD inPipeOpenMode = PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED;
    DWORD outPipeOpenMode = PIPE_ACCESS_INBOUND;

    DWORD inPipeMode = PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT;
    DWORD outPipeMode = PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT;

    wstring inPipeName = L"\\\\.\\pipe\\convt-test-0-in";
    wstring outPipeName = L"\\\\.\\pipe\\convt-test-0-out";

    Log::Comment(L"\tcreating pipes");

    HANDLE inPipe = (
        CreateNamedPipeW(inPipeName.c_str(), inPipeOpenMode,inPipeMode, 1, 0, 0, 0, nullptr)
    );
    VERIFY_ARE_NOT_EQUAL(inPipe, INVALID_HANDLE_VALUE);
    auto inCleanup = wil::ScopeExit([&]{
        // 0 indicates failure for CloseHandle
        VERIFY_ARE_NOT_EQUAL(CloseHandle(inPipe), 0);
    });

    HANDLE outPipe = (
        CreateNamedPipeW(outPipeName.c_str(), outPipeOpenMode, outPipeMode, 1, 0, 0, 0, nullptr)
    );
    VERIFY_ARE_NOT_EQUAL(outPipe, INVALID_HANDLE_VALUE);
    auto outCleanup = wil::ScopeExit([&]{
        // 0 indicates failure for CloseHandle
        VERIFY_ARE_NOT_EQUAL(CloseHandle(outPipe), 0);
    });

    Log::Comment(L"\tinitializing vtio");

    VtIo vtio = VtIo();
    VERIFY_IS_FALSE(vtio.IsUsingVt());
    VERIFY_SUCCEEDED(vtio._Initialize(inPipeName, outPipeName, L""));
    VERIFY_IS_TRUE(vtio.IsUsingVt());

    Log::Comment(L"\tconnecting the pipes");

    bool fSuccess = !!ConnectNamedPipe(inPipe, nullptr);
    if (!fSuccess)
    {
        HRESULT lastError = (HRESULT)GetLastError();
        VERIFY_ARE_EQUAL(lastError, ERROR_PIPE_CONNECTED);
    }

    fSuccess = !!ConnectNamedPipe(outPipe, nullptr);
    if (!fSuccess)
    {
        HRESULT lastError = (HRESULT)GetLastError();
        VERIFY_ARE_EQUAL(lastError, ERROR_PIPE_CONNECTED);
    }    

}

void VtIoTests::NoInPipeOpeningTest()
{
    Log::Comment(L"Don't create an in pipe, see what happens");
    DWORD outPipeOpenMode = PIPE_ACCESS_INBOUND;
    DWORD outPipeMode = PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT;

    wstring inPipeName = L"\\\\.\\pipe\\convt-test-1-in";
    wstring outPipeName = L"\\\\.\\pipe\\convt-test-1-out";

    Log::Comment(L"\tcreating pipes");

    HANDLE outPipe = (
        CreateNamedPipeW(outPipeName.c_str(), outPipeOpenMode, outPipeMode, 1, 0, 0, 0, nullptr)
    );
    VERIFY_ARE_NOT_EQUAL(outPipe, INVALID_HANDLE_VALUE);
    auto outCleanup = wil::ScopeExit([&]{
        // 0 indicates failure for CloseHandle
        VERIFY_ARE_NOT_EQUAL(CloseHandle(outPipe), 0);
    });

    Log::Comment(L"\tinitializing vtio");

    VtIo vtio = VtIo();
    VERIFY_IS_FALSE(vtio.IsUsingVt());
    VERIFY_FAILED(vtio._Initialize(inPipeName, outPipeName, L""));
    VERIFY_IS_FALSE(vtio.IsUsingVt());

    Log::Comment(L"\tconnecting the pipes");

}

void VtIoTests::NoOutPipeOpeningTest()
{
    Log::Comment(L"Don't create an out pipe, see what happens");
    DWORD inPipeOpenMode = PIPE_ACCESS_DUPLEX;

    DWORD inPipeMode = PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT;

    wstring inPipeName = L"\\\\.\\pipe\\convt-test-2-in";
    wstring outPipeName = L"\\\\.\\pipe\\convt-test-2-out";

    Log::Comment(L"\tcreating pipes");

    HANDLE inPipe = (
        CreateNamedPipeW(inPipeName.c_str(), inPipeOpenMode,inPipeMode, 1, 0, 0, 0, nullptr)
    );
    VERIFY_ARE_NOT_EQUAL(inPipe, INVALID_HANDLE_VALUE);
    auto inCleanup = wil::ScopeExit([&]{
        // 0 indicates failure for CloseHandle
        VERIFY_ARE_NOT_EQUAL(CloseHandle(inPipe), 0);
    });

    Log::Comment(L"\tinitializing vtio");

    VtIo vtio = VtIo();
    VERIFY_IS_FALSE(vtio.IsUsingVt());
    VERIFY_FAILED(vtio._Initialize(inPipeName, outPipeName, L""));
    VERIFY_IS_FALSE(vtio.IsUsingVt());

}

void VtIoTests::NonOverlappedInModeTest()
{
    Log::Comment(L"change the input mode to non-overlapped");
    DWORD inPipeOpenMode = PIPE_ACCESS_DUPLEX;
    DWORD outPipeOpenMode = PIPE_ACCESS_INBOUND;

    DWORD inPipeMode = PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT;
    DWORD outPipeMode = PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT;

    wstring inPipeName = L"\\\\.\\pipe\\convt-test-3-in";
    wstring outPipeName = L"\\\\.\\pipe\\convt-test-3-out";

    Log::Comment(L"\tcreating pipes");

    HANDLE inPipe = (
        CreateNamedPipeW(inPipeName.c_str(), inPipeOpenMode,inPipeMode, 1, 0, 0, 0, nullptr)
    );
    VERIFY_ARE_NOT_EQUAL(inPipe, INVALID_HANDLE_VALUE);
    auto inCleanup = wil::ScopeExit([&]{
        // 0 indicates failure for CloseHandle
        VERIFY_ARE_NOT_EQUAL(CloseHandle(inPipe), 0);
    });

    HANDLE outPipe = (
        CreateNamedPipeW(outPipeName.c_str(), outPipeOpenMode, outPipeMode, 1, 0, 0, 0, nullptr)
    );
    VERIFY_ARE_NOT_EQUAL(outPipe, INVALID_HANDLE_VALUE);
    auto outCleanup = wil::ScopeExit([&]{
        // 0 indicates failure for CloseHandle
        VERIFY_ARE_NOT_EQUAL(CloseHandle(outPipe), 0);
    });

    Log::Comment(L"\tinitializing vtio");

    VtIo vtio = VtIo();
    VERIFY_IS_FALSE(vtio.IsUsingVt());
    VERIFY_SUCCEEDED(vtio._Initialize(inPipeName, outPipeName, L""));
    VERIFY_IS_TRUE(vtio.IsUsingVt());

    Log::Comment(L"\tconnecting the pipes");

    bool fSuccess = !!ConnectNamedPipe(inPipe, nullptr);
    if (!fSuccess)
    {
        HRESULT lastError = (HRESULT)GetLastError();
        VERIFY_ARE_EQUAL(lastError, ERROR_PIPE_CONNECTED);
    }

    fSuccess = !!ConnectNamedPipe(outPipe, nullptr);
    if (!fSuccess)
    {
        HRESULT lastError = (HRESULT)GetLastError();
        VERIFY_ARE_EQUAL(lastError, ERROR_PIPE_CONNECTED);
    }    
}

void VtIoTests::NonDuplexInModeTest()
{
    Log::Comment(L"change the input mode to outbound only");

    DWORD inPipeOpenMode = PIPE_ACCESS_OUTBOUND;
    DWORD outPipeOpenMode = PIPE_ACCESS_INBOUND;

    DWORD inPipeMode = PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT;
    DWORD outPipeMode = PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT;

    wstring inPipeName = L"\\\\.\\pipe\\convt-test-4-in";
    wstring outPipeName = L"\\\\.\\pipe\\convt-test-4-out";

    Log::Comment(L"\tcreating pipes");

    HANDLE inPipe = (
        CreateNamedPipeW(inPipeName.c_str(), inPipeOpenMode,inPipeMode, 1, 0, 0, 0, nullptr)
    );
    VERIFY_ARE_NOT_EQUAL(inPipe, INVALID_HANDLE_VALUE);
    auto inCleanup = wil::ScopeExit([&]{
        // 0 indicates failure for CloseHandle
        VERIFY_ARE_NOT_EQUAL(CloseHandle(inPipe), 0);
    });

    HANDLE outPipe = (
        CreateNamedPipeW(outPipeName.c_str(), outPipeOpenMode, outPipeMode, 1, 0, 0, 0, nullptr)
    );
    VERIFY_ARE_NOT_EQUAL(outPipe, INVALID_HANDLE_VALUE);
    auto outCleanup = wil::ScopeExit([&]{
        // 0 indicates failure for CloseHandle
        VERIFY_ARE_NOT_EQUAL(CloseHandle(outPipe), 0);
    });

    Log::Comment(L"\tinitializing vtio");

    VtIo vtio = VtIo();
    VERIFY_IS_FALSE(vtio.IsUsingVt());
    VERIFY_SUCCEEDED(vtio._Initialize(inPipeName, outPipeName, L""));
    VERIFY_IS_TRUE(vtio.IsUsingVt());

    Log::Comment(L"\tconnecting the pipes");

    bool fSuccess = !!ConnectNamedPipe(inPipe, nullptr);
    if (!fSuccess)
    {
        HRESULT lastError = (HRESULT)GetLastError();
        VERIFY_ARE_EQUAL(lastError, ERROR_PIPE_CONNECTED);
    }

    fSuccess = !!ConnectNamedPipe(outPipe, nullptr);
    if (!fSuccess)
    {
        HRESULT lastError = (HRESULT)GetLastError();
        VERIFY_ARE_EQUAL(lastError, ERROR_PIPE_CONNECTED);
    }    

}

void VtIoTests::WrongInModeTest()
{
    Log::Comment(L"Make the input pipe PIPE_ACCESS_INBOUND only");

    DWORD inPipeOpenMode = PIPE_ACCESS_INBOUND;
    DWORD outPipeOpenMode = PIPE_ACCESS_INBOUND;

    DWORD inPipeMode = PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT;
    DWORD outPipeMode = PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT;

    wstring inPipeName = L"\\\\.\\pipe\\convt-test-5-in";
    wstring outPipeName = L"\\\\.\\pipe\\convt-test-5-out";

    Log::Comment(L"\tcreating pipes");

    HANDLE inPipe = (
        CreateNamedPipeW(inPipeName.c_str(), inPipeOpenMode,inPipeMode, 1, 0, 0, 0, nullptr)
    );
    VERIFY_ARE_NOT_EQUAL(inPipe, INVALID_HANDLE_VALUE);
    auto inCleanup = wil::ScopeExit([&]{
        // 0 indicates failure for CloseHandle
        VERIFY_ARE_NOT_EQUAL(CloseHandle(inPipe), 0);
    });

    HANDLE outPipe = (
        CreateNamedPipeW(outPipeName.c_str(), outPipeOpenMode, outPipeMode, 1, 0, 0, 0, nullptr)
    );
    VERIFY_ARE_NOT_EQUAL(outPipe, INVALID_HANDLE_VALUE);
    auto outCleanup = wil::ScopeExit([&]{
        // 0 indicates failure for CloseHandle
        VERIFY_ARE_NOT_EQUAL(CloseHandle(outPipe), 0);
    });

    Log::Comment(L"\tinitializing vtio");

    VtIo vtio = VtIo();
    VERIFY_IS_FALSE(vtio.IsUsingVt());
    VERIFY_FAILED(vtio._Initialize(inPipeName, outPipeName, L""));
    VERIFY_IS_FALSE(vtio.IsUsingVt());
}

void VtIoTests::WrongOutModeTest()
{
    Log::Comment(L"Make the output pipe PIPE_ACCESS_OUTBOUND only");

    DWORD inPipeOpenMode = PIPE_ACCESS_DUPLEX;
    DWORD outPipeOpenMode = PIPE_ACCESS_OUTBOUND;

    DWORD inPipeMode = PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT;
    DWORD outPipeMode = PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT;

    wstring inPipeName = L"\\\\.\\pipe\\convt-test-6-in";
    wstring outPipeName = L"\\\\.\\pipe\\convt-test-6-out";

    Log::Comment(L"\tcreating pipes");

    HANDLE inPipe = (
        CreateNamedPipeW(inPipeName.c_str(), inPipeOpenMode,inPipeMode, 1, 0, 0, 0, nullptr)
    );
    VERIFY_ARE_NOT_EQUAL(inPipe, INVALID_HANDLE_VALUE);
    auto inCleanup = wil::ScopeExit([&]{
        // 0 indicates failure for CloseHandle
        VERIFY_ARE_NOT_EQUAL(CloseHandle(inPipe), 0);
    });

    HANDLE outPipe = (
        CreateNamedPipeW(outPipeName.c_str(), outPipeOpenMode, outPipeMode, 1, 0, 0, 0, nullptr)
    );
    VERIFY_ARE_NOT_EQUAL(outPipe, INVALID_HANDLE_VALUE);
    auto outCleanup = wil::ScopeExit([&]{
        // 0 indicates failure for CloseHandle
        VERIFY_ARE_NOT_EQUAL(CloseHandle(outPipe), 0);
    });

    Log::Comment(L"\tinitializing vtio");

    VtIo vtio = VtIo();
    VERIFY_IS_FALSE(vtio.IsUsingVt());
    VERIFY_FAILED(vtio._Initialize(inPipeName, outPipeName, L""));
    VERIFY_IS_FALSE(vtio.IsUsingVt());
}

void VtIoTests::MultipleInstanceTest()
{
    
    Log::Comment(L"The basic case but multiple instances of the named pipe can be opened");
    // DUPLEX is definitely acceptable for the inpipe, but I think we ideally want non-overlapped...
    // I don't do anything overlapped, so I thin k that flag doesn't need to be there.
    DWORD inPipeOpenMode = PIPE_ACCESS_DUPLEX;
    DWORD outPipeOpenMode = PIPE_ACCESS_INBOUND;

    DWORD inPipeMode = PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT;
    DWORD outPipeMode = PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT;

    wstring inPipeName = L"\\\\.\\pipe\\convt-test-7-in";
    wstring outPipeName = L"\\\\.\\pipe\\convt-test-7-out";

    Log::Comment(L"\tcreating pipes");

    HANDLE inPipe = (
        CreateNamedPipeW(inPipeName.c_str(), inPipeOpenMode,inPipeMode, PIPE_UNLIMITED_INSTANCES, 0, 0, 0, nullptr)
    );
    VERIFY_ARE_NOT_EQUAL(inPipe, INVALID_HANDLE_VALUE);
    auto inCleanup = wil::ScopeExit([&]{
        // 0 indicates failure for CloseHandle
        VERIFY_ARE_NOT_EQUAL(CloseHandle(inPipe), 0);
    });

    HANDLE outPipe = (
        CreateNamedPipeW(outPipeName.c_str(), outPipeOpenMode, outPipeMode, PIPE_UNLIMITED_INSTANCES, 0, 0, 0, nullptr)
    );
    VERIFY_ARE_NOT_EQUAL(outPipe, INVALID_HANDLE_VALUE);
    auto outCleanup = wil::ScopeExit([&]{
        // 0 indicates failure for CloseHandle
        VERIFY_ARE_NOT_EQUAL(CloseHandle(outPipe), 0);
    });

    Log::Comment(L"\tinitializing vtio");

    VtIo vtio = VtIo();
    VERIFY_IS_FALSE(vtio.IsUsingVt());
    VERIFY_SUCCEEDED(vtio._Initialize(inPipeName, outPipeName, L""));
    VERIFY_IS_TRUE(vtio.IsUsingVt());

    Log::Comment(L"\tconnecting the pipes");

    bool fSuccess = !!ConnectNamedPipe(inPipe, nullptr);
    if (!fSuccess)
    {
        HRESULT lastError = (HRESULT)GetLastError();
        VERIFY_ARE_EQUAL(lastError, ERROR_PIPE_CONNECTED);
    }

    fSuccess = !!ConnectNamedPipe(outPipe, nullptr);
    if (!fSuccess)
    {
        HRESULT lastError = (HRESULT)GetLastError();
        VERIFY_ARE_EQUAL(lastError, ERROR_PIPE_CONNECTED);
    }    
}

void VtIoTests::OnePipeForBothTest()
{
    Log::Comment(L"Create only one pipe and pass it in as both pipes");

    DWORD onePipeOpenMode = PIPE_ACCESS_DUPLEX;

    DWORD onePipeMode = PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT;

    wstring onePipeName = L"\\\\.\\pipe\\convt-test-8-one";

    Log::Comment(L"\tcreating pipe");

    HANDLE onePipe = (
        CreateNamedPipeW(onePipeName.c_str(), onePipeOpenMode, onePipeMode, 1, 0, 0, 0, nullptr)
    );
    VERIFY_ARE_NOT_EQUAL(onePipe, INVALID_HANDLE_VALUE);
    auto oneCleanup = wil::ScopeExit([&]{
        // 0 indicates failure for CloseHandle
        VERIFY_ARE_NOT_EQUAL(CloseHandle(onePipe), 0);
    });

    Log::Comment(L"\tinitializing vtio");

    VtIo vtio = VtIo();
    VERIFY_IS_FALSE(vtio.IsUsingVt());
    VERIFY_FAILED(vtio._Initialize(onePipeName, onePipeName, L""));
    VERIFY_IS_FALSE(vtio.IsUsingVt());

}

void VtIoTests::OnePipeMultipleInstancesTest()
{
    Log::Comment(L"Create only one pipe, but allow for multiple instances to be created");

    DWORD onePipeOpenMode = PIPE_ACCESS_DUPLEX;

    DWORD onePipeMode = PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT;

    wstring onePipeName = L"\\\\.\\pipe\\convt-test-9-one";

    Log::Comment(L"\tcreating pipe");

    HANDLE onePipe = (
        CreateNamedPipeW(onePipeName.c_str(), onePipeOpenMode, onePipeMode, PIPE_UNLIMITED_INSTANCES, 0, 0, 0, nullptr)
    );
    VERIFY_ARE_NOT_EQUAL(onePipe, INVALID_HANDLE_VALUE);
    auto oneCleanup = wil::ScopeExit([&]{
        // 0 indicates failure for CloseHandle
        VERIFY_ARE_NOT_EQUAL(CloseHandle(onePipe), 0);
    });

    Log::Comment(L"\tinitializing vtio");

    VtIo vtio = VtIo();
    VERIFY_IS_FALSE(vtio.IsUsingVt());
    VERIFY_FAILED(vtio._Initialize(onePipeName, onePipeName, L""));
    VERIFY_IS_FALSE(vtio.IsUsingVt());

}

void VtIoTests::PipeAlreadyOpenedTest()
{
    Log::Comment(L"Two pipes, but one's already been opened before we initialize");

    // DUPLEX is definitely acceptable for the inpipe, but I think we ideally want non-overlapped...
    // I don't do anything overlapped, so I thin k that flag doesn't need to be there.
    DWORD inPipeOpenMode = PIPE_ACCESS_DUPLEX;
    DWORD outPipeOpenMode = PIPE_ACCESS_INBOUND;

    DWORD inPipeMode = PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT;
    DWORD outPipeMode = PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT;

    wstring inPipeName = L"\\\\.\\pipe\\convt-test-10-in";
    wstring outPipeName = L"\\\\.\\pipe\\convt-test-10-out";

    Log::Comment(L"\tcreating pipes");

    HANDLE inPipe = (
        CreateNamedPipeW(inPipeName.c_str(), inPipeOpenMode,inPipeMode, 1, 0, 0, 0, nullptr)
    );
    VERIFY_ARE_NOT_EQUAL(inPipe, INVALID_HANDLE_VALUE);
    auto inCleanup = wil::ScopeExit([&]{
        // 0 indicates failure for CloseHandle
        VERIFY_ARE_NOT_EQUAL(CloseHandle(inPipe), 0);
    });

    HANDLE outPipe = (
        CreateNamedPipeW(outPipeName.c_str(), outPipeOpenMode, outPipeMode, 1, 0, 0, 0, nullptr)
    );
    VERIFY_ARE_NOT_EQUAL(outPipe, INVALID_HANDLE_VALUE);
    auto outCleanup = wil::ScopeExit([&]{
        // 0 indicates failure for CloseHandle
        VERIFY_ARE_NOT_EQUAL(CloseHandle(outPipe), 0);
    });

    Log::Comment(L"\topen one of the pipes");

    HANDLE blocker = CreateFileW(inPipeName.c_str(),
                                 GENERIC_READ, 
                                 0, 
                                 nullptr, 
                                 OPEN_EXISTING, 
                                 FILE_ATTRIBUTE_NORMAL, 
                                 nullptr);
    VERIFY_ARE_NOT_EQUAL(blocker, INVALID_HANDLE_VALUE);
    auto blockerCleanup = wil::ScopeExit([&]{
        // 0 indicates failure for CloseHandle
        VERIFY_ARE_NOT_EQUAL(CloseHandle(blocker), 0);
    });

    Log::Comment(L"\tinitializing vtio");

    VtIo vtio = VtIo();
    VERIFY_IS_FALSE(vtio.IsUsingVt());
    VERIFY_FAILED(vtio._Initialize(inPipeName, outPipeName, L""));
    VERIFY_IS_FALSE(vtio.IsUsingVt());

}

void VtIoTests::MultipleInstancesPipeAlreadyOpenedTest()
{
    Log::Comment(L"Two pipes, we can have multiple instances, but one's already been opened before we initialize");

    // DUPLEX is definitely acceptable for the inpipe, but I think we ideally want non-overlapped...
    // I don't do anything overlapped, so I thin k that flag doesn't need to be there.
    DWORD inPipeOpenMode = PIPE_ACCESS_DUPLEX;
    DWORD outPipeOpenMode = PIPE_ACCESS_INBOUND;

    DWORD inPipeMode = PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT;
    DWORD outPipeMode = PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT;

    wstring inPipeName = L"\\\\.\\pipe\\convt-test-11-in";
    wstring outPipeName = L"\\\\.\\pipe\\convt-test-11-out";

    Log::Comment(L"\tcreating pipes");

    HANDLE inPipe = (
        CreateNamedPipeW(inPipeName.c_str(), inPipeOpenMode,inPipeMode, PIPE_UNLIMITED_INSTANCES, 0, 0, 0, nullptr)
    );
    VERIFY_ARE_NOT_EQUAL(inPipe, INVALID_HANDLE_VALUE);
    auto inCleanup = wil::ScopeExit([&]{
        // 0 indicates failure for CloseHandle
        VERIFY_ARE_NOT_EQUAL(CloseHandle(inPipe), 0);
    });

    HANDLE outPipe = (
        CreateNamedPipeW(outPipeName.c_str(), outPipeOpenMode, outPipeMode, PIPE_UNLIMITED_INSTANCES, 0, 0, 0, nullptr)
    );
    VERIFY_ARE_NOT_EQUAL(outPipe, INVALID_HANDLE_VALUE);
    auto outCleanup = wil::ScopeExit([&]{
        // 0 indicates failure for CloseHandle
        VERIFY_ARE_NOT_EQUAL(CloseHandle(outPipe), 0);
    });

    Log::Comment(L"\topen one of the pipes");

    HANDLE blocker = CreateFileW(inPipeName.c_str(),
                                 GENERIC_READ, 
                                 0, 
                                 nullptr, 
                                 OPEN_EXISTING, 
                                 FILE_ATTRIBUTE_NORMAL, 
                                 nullptr);
    VERIFY_ARE_NOT_EQUAL(blocker, INVALID_HANDLE_VALUE);
    auto blockerCleanup = wil::ScopeExit([&]{
        // 0 indicates failure for CloseHandle
        VERIFY_ARE_NOT_EQUAL(CloseHandle(blocker), 0);
    });

    Log::Comment(L"\tinitializing vtio");

    VtIo vtio = VtIo();
    VERIFY_IS_FALSE(vtio.IsUsingVt());
    VERIFY_FAILED(vtio._Initialize(inPipeName, outPipeName, L""));
    VERIFY_IS_FALSE(vtio.IsUsingVt());

}

Viewport SetUpViewport()
{
    SMALL_RECT view = {};
    view.Top = view.Left = 0;
    view.Bottom = 31;
    view.Right = 79;

    return Viewport::FromInclusive(view);
}

void VtIoTests::DtorTestJustEngine()
{
    Log::Comment(NoThrowString().Format(
        L"This test is going to instantiate a bunch of VtIos in different \n"
        L"scenarios to see if something causes a weird cleanup.\n"
        L"It's here because of the strange nature of VtEngine having members\n"
        L"that are only defined in UNIT_TESTING"
    ));

    const WORD colorTableSize = 16;
    COLORREF colorTable[colorTableSize];

    Log::Comment(NoThrowString().Format(
        L"New some engines and delete them"
    ));
    for (int i = 0; i < 25; ++i)
    {
        Log::Comment(NoThrowString().Format(
            L"New/Delete loop #%d", i
        ));

        wil::unique_hfile hOutputFile;
        hOutputFile.reset(INVALID_HANDLE_VALUE);
		auto pRenderer256 = new Xterm256Engine(std::move(hOutputFile), SetUpViewport());
        Log::Comment(NoThrowString().Format(L"Made Xterm256Engine"));
        delete pRenderer256;
        Log::Comment(NoThrowString().Format(L"Deleted."));

        hOutputFile.reset(INVALID_HANDLE_VALUE);

        auto pRenderEngineXterm = new XtermEngine(std::move(hOutputFile), SetUpViewport(), colorTable, colorTableSize, false);
        Log::Comment(NoThrowString().Format(L"Made XtermEngine"));
        delete pRenderEngineXterm;
        Log::Comment(NoThrowString().Format(L"Deleted."));

        hOutputFile.reset(INVALID_HANDLE_VALUE);

        auto pRenderEngineXtermAscii = new XtermEngine(std::move(hOutputFile), SetUpViewport(), colorTable, colorTableSize, true);
        Log::Comment(NoThrowString().Format(L"Made XtermEngine"));
        delete pRenderEngineXtermAscii;
        Log::Comment(NoThrowString().Format(L"Deleted."));
        
        hOutputFile.reset(INVALID_HANDLE_VALUE);

        auto pRenderEngineWinTelnet = new WinTelnetEngine(std::move(hOutputFile), SetUpViewport(), colorTable, colorTableSize);
        Log::Comment(NoThrowString().Format(L"Made WinTelnetEngine"));
        delete pRenderEngineWinTelnet;
        Log::Comment(NoThrowString().Format(L"Deleted."));
    }

}

void VtIoTests::DtorTestDeleteVtio()
{
    Log::Comment(NoThrowString().Format(
        L"This test is going to instantiate a bunch of VtIos in different \n"
        L"scenarios to see if something causes a weird cleanup.\n"
        L"It's here because of the strange nature of VtEngine having members\n"
        L"that are only defined in UNIT_TESTING"
    ));

    const WORD colorTableSize = 16;
    COLORREF colorTable[colorTableSize];

    Log::Comment(NoThrowString().Format(
        L"New some engines and delete them"
    ));
    for (int i = 0; i < 25; ++i)
    {
        Log::Comment(NoThrowString().Format(
            L"New/Delete loop #%d", i
        ));

        wil::unique_hfile hOutputFile = wil::unique_hfile(INVALID_HANDLE_VALUE);

        hOutputFile.reset(INVALID_HANDLE_VALUE);

        VtIo* vtio = new VtIo();
        Log::Comment(NoThrowString().Format(L"Made VtIo"));
        vtio->_pVtRenderEngine = std::make_unique<Xterm256Engine>(std::move(hOutputFile),
                                                                  SetUpViewport());
        Log::Comment(NoThrowString().Format(L"Made Xterm256Engine"));
        delete vtio;
        Log::Comment(NoThrowString().Format(L"Deleted."));

        hOutputFile = wil::unique_hfile(INVALID_HANDLE_VALUE);      
        vtio = new VtIo();
        Log::Comment(NoThrowString().Format(L"Made VtIo"));
        vtio->_pVtRenderEngine = std::make_unique<XtermEngine>(std::move(hOutputFile),
                                                               SetUpViewport(),
                                                               colorTable,
                                                               colorTableSize,
                                                               false);
        Log::Comment(NoThrowString().Format(L"Made XtermEngine"));
        delete vtio;
        Log::Comment(NoThrowString().Format(L"Deleted."));

        hOutputFile = wil::unique_hfile(INVALID_HANDLE_VALUE);
        vtio = new VtIo();
        Log::Comment(NoThrowString().Format(L"Made VtIo"));
        vtio->_pVtRenderEngine = std::make_unique<XtermEngine>(std::move(hOutputFile),
                                                               SetUpViewport(),
                                                               colorTable,
                                                               colorTableSize,
                                                               true);
        Log::Comment(NoThrowString().Format(L"Made XtermEngine"));
        delete vtio;
        Log::Comment(NoThrowString().Format(L"Deleted."));
        
        hOutputFile = wil::unique_hfile(INVALID_HANDLE_VALUE);
        vtio = new VtIo();
        Log::Comment(NoThrowString().Format(L"Made VtIo"));
        vtio->_pVtRenderEngine = std::make_unique<WinTelnetEngine>(std::move(hOutputFile),
                                                                   SetUpViewport(),
                                                                   colorTable,
                                                                   colorTableSize);
        Log::Comment(NoThrowString().Format(L"Made WinTelnetEngine"));
        delete vtio;
        Log::Comment(NoThrowString().Format(L"Deleted."));
    }

}

void VtIoTests::DtorTestStackAlloc()
{
    Log::Comment(NoThrowString().Format(
        L"This test is going to instantiate a bunch of VtIos in different \n"
        L"scenarios to see if something causes a weird cleanup.\n"
        L"It's here because of the strange nature of VtEngine having members\n"
        L"that are only defined in UNIT_TESTING"
    ));

    const WORD colorTableSize = 16;
    COLORREF colorTable[colorTableSize];

    Log::Comment(NoThrowString().Format(
        L"make some engines and let them fall out of scope"
    ));
    for (int i = 0; i < 25; ++i)
    {
        Log::Comment(NoThrowString().Format(
            L"Scope Exit Auto cleanup #%d", i
        ));
    
        wil::unique_hfile hOutputFile;

        hOutputFile.reset(INVALID_HANDLE_VALUE);
        {
            VtIo vtio = VtIo();
            vtio._pVtRenderEngine = std::make_unique<Xterm256Engine>(std::move(hOutputFile),
                                                                     SetUpViewport());
        }

        hOutputFile.reset(INVALID_HANDLE_VALUE);
        {
            VtIo vtio = VtIo();
            vtio._pVtRenderEngine = std::make_unique<XtermEngine>(std::move(hOutputFile),
                                                                  SetUpViewport(),
                                                                  colorTable,
                                                                  colorTableSize,
                                                                  false);
        }

        hOutputFile.reset(INVALID_HANDLE_VALUE);
        {
            VtIo vtio = VtIo();
            vtio._pVtRenderEngine = std::make_unique<XtermEngine>(std::move(hOutputFile),
                                                                  SetUpViewport(),
                                                                  colorTable,
                                                                  colorTableSize,
                                                                  true);
        }
        
        hOutputFile.reset(INVALID_HANDLE_VALUE);
        {
            VtIo vtio = VtIo();
            vtio._pVtRenderEngine = std::make_unique<WinTelnetEngine>(std::move(hOutputFile),
                                                                      SetUpViewport(),
                                                                      colorTable,
                                                                      colorTableSize);
        }
    }

}

void VtIoTests::DtorTestStackAllocMany()
{
    Log::Comment(NoThrowString().Format(
        L"This test is going to instantiate a bunch of VtIos in different \n"
        L"scenarios to see if something causes a weird cleanup.\n"
        L"It's here because of the strange nature of VtEngine having members\n"
        L"that are only defined in UNIT_TESTING"
    ));

    const WORD colorTableSize = 16;
    COLORREF colorTable[colorTableSize];

    Log::Comment(NoThrowString().Format(
        L"Try an make a whole bunch all at once, and have them all fall out of scope at once."
    ));
    for (int i = 0; i < 25; ++i)
    {
        Log::Comment(NoThrowString().Format(
            L"Multiple engines, one scope loop #%d", i
        ));
    
        wil::unique_hfile hOutputFile;
        {
            hOutputFile.reset(INVALID_HANDLE_VALUE);
            VtIo vtio1 = VtIo();
            vtio1._pVtRenderEngine = std::make_unique<Xterm256Engine>(std::move(hOutputFile),
                                                                      SetUpViewport());

            hOutputFile.reset(INVALID_HANDLE_VALUE);
            VtIo vtio2 = VtIo();
            vtio2._pVtRenderEngine = std::make_unique<XtermEngine>(std::move(hOutputFile),
                                                                   SetUpViewport(),
                                                                   colorTable,
                                                                   colorTableSize,
                                                                   false);

            hOutputFile.reset(INVALID_HANDLE_VALUE);
            VtIo vtio3 = VtIo();
            vtio3._pVtRenderEngine = std::make_unique<XtermEngine>(std::move(hOutputFile),
                                                                   SetUpViewport(),
                                                                   colorTable,
                                                                   colorTableSize,
                                                                   true);

            hOutputFile.reset(INVALID_HANDLE_VALUE);
            VtIo vtio4 = VtIo();
            vtio4._pVtRenderEngine = std::make_unique<WinTelnetEngine>(std::move(hOutputFile),
                                                                       SetUpViewport(),
                                                                       colorTable,
                                                                       colorTableSize);
        }
    }

}

void VtIoTests::DtorTestNonDuplexInModeTest()
{
    Log::Comment(NoThrowString().Format(
        L"This test is going to instantiate a bunch of VtIos in different \n"
        L"scenarios to see if something causes a weird cleanup.\n"
        L"It's here because of the strange nature of VtEngine having members\n"
        L"that are only defined in UNIT_TESTING"
    ));

    Log::Comment(NoThrowString().Format(
        L"The next test runs the body of the NonDuplexInModeTest a bunch of times\n"
        L"That was a test that would intermittently fail. This one will ALWAYS fail."
    ));

    for (int i = 0; i < 25; ++i)
    {
        Log::Comment(NoThrowString().Format(
            L"NonDuplexInModeTest, unmodified, loop #%d", i
        ));

        {
            Log::Comment(L"change the input mode to outbound only");

            DWORD inPipeOpenMode = PIPE_ACCESS_OUTBOUND;
            DWORD outPipeOpenMode = PIPE_ACCESS_INBOUND;

            DWORD inPipeMode = PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT;
            DWORD outPipeMode = PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT;

            wstring inPipeName = L"\\\\.\\pipe\\convt-test-12-in";
            wstring outPipeName = L"\\\\.\\pipe\\convt-test-12-out";

            Log::Comment(L"\tcreating pipes");

            HANDLE inPipe = (
                CreateNamedPipeW(inPipeName.c_str(), inPipeOpenMode,inPipeMode, 1, 0, 0, 0, nullptr)
            );
            VERIFY_ARE_NOT_EQUAL(inPipe, INVALID_HANDLE_VALUE);
            auto inCleanup = wil::ScopeExit([inPipe]{
                Log::Comment(NoThrowString().Format(L"inCleanup"));
                VERIFY_ARE_NOT_EQUAL(CloseHandle(inPipe), 0);
            });

            HANDLE outPipe = (
                CreateNamedPipeW(outPipeName.c_str(), outPipeOpenMode, outPipeMode, 1, 0, 0, 0, nullptr)
            );
            VERIFY_ARE_NOT_EQUAL(outPipe, INVALID_HANDLE_VALUE);
            auto outCleanup = wil::ScopeExit([outPipe]{
                Log::Comment(NoThrowString().Format(L"outCleanup"));
                VERIFY_ARE_NOT_EQUAL(CloseHandle(outPipe), 0);
            });

            Log::Comment(L"\tinitializing vtio");


            VtIo vtio = VtIo();
            VERIFY_IS_FALSE(vtio.IsUsingVt());
            VERIFY_SUCCEEDED(vtio._Initialize(inPipeName, outPipeName, L""));
            VERIFY_IS_TRUE(vtio.IsUsingVt());

            Log::Comment(L"\tconnecting the pipes isn't relevant here, repros without that part.");   

        }
        Log::Comment(L"Bottom of scope.");  
    }

}

void VtIoTests::DtorTestNonDuplexInModeTestModified()
{
    Log::Comment(NoThrowString().Format(
        L"This test is going to instantiate a bunch of VtIos in different \n"
        L"scenarios to see if something causes a weird cleanup.\n"
        L"It's here because of the strange nature of VtEngine having members\n"
        L"that are only defined in UNIT_TESTING"
    ));

    Log::Comment(NoThrowString().Format(
        L"Now modifed to use unique_hfiles instead of ScopeExit([]{CloseHandle()})"
    ));

    for (int i = 0; i < 25; ++i)
    {
        Log::Comment(NoThrowString().Format(
            L"NonDuplexInModeTest, unique_hfiles, loop #%d", i
        ));

        {
            DWORD inPipeOpenMode = PIPE_ACCESS_OUTBOUND;
            DWORD outPipeOpenMode = PIPE_ACCESS_INBOUND;

            DWORD inPipeMode = PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT;
            DWORD outPipeMode = PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT;

            wstring inPipeName = L"\\\\.\\pipe\\convt-test-13-in";
            wstring outPipeName = L"\\\\.\\pipe\\convt-test-13-out";

            wil::unique_hfile inPipe;
            inPipe.reset(
                CreateNamedPipeW(inPipeName.c_str(), inPipeOpenMode,inPipeMode, 1, 0, 0, 0, nullptr)
            );
            VERIFY_ARE_NOT_EQUAL(inPipe.get(), INVALID_HANDLE_VALUE);

            wil::unique_hfile outPipe;
            outPipe.reset(
                CreateNamedPipeW(outPipeName.c_str(), outPipeOpenMode, outPipeMode, 1, 0, 0, 0, nullptr)
            );
            VERIFY_ARE_NOT_EQUAL(outPipe.get(), INVALID_HANDLE_VALUE);

            VtIo vtio;
            VERIFY_IS_FALSE(vtio.IsUsingVt());
            VERIFY_SUCCEEDED(vtio._Initialize(inPipeName, outPipeName, L""));
            VERIFY_IS_NOT_NULL(vtio._pVtRenderEngine);
            Log::Comment(L"\tconnecting the pipes isn't relevant here, repros without that part.");   

        }
        Log::Comment(L"Bottom of scope.");  
    }
}
