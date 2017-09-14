/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/
#include "precomp.h"
#include "WexTestClass.h"

#include "globals.h"
#include "..\VtIo.hpp"

using namespace WEX::Common;
using namespace WEX::Logging;
using namespace WEX::TestExecution;
using namespace std;

class Microsoft::Console::VirtualTerminal::VtIoTests
{
    TEST_CLASS(Microsoft::Console::VirtualTerminal::VtIoTests);

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
};

using namespace Microsoft::Console::VirtualTerminal;

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
    VERIFY_SUCCEEDED(VtIo::ParseIoMode(L"xterm", &mode));
    VERIFY_ARE_EQUAL(mode, VtIoMode::XTERM);

    VERIFY_SUCCEEDED(VtIo::ParseIoMode(L"xterm-256color", &mode));
    VERIFY_ARE_EQUAL(mode, VtIoMode::XTERM_256);

    VERIFY_SUCCEEDED(VtIo::ParseIoMode(L"win-telnet", &mode));
    VERIFY_ARE_EQUAL(mode, VtIoMode::WIN_TELNET);

    VERIFY_SUCCEEDED(VtIo::ParseIoMode(L"", &mode));
    VERIFY_ARE_EQUAL(mode, VtIoMode::XTERM_256);
    
    VERIFY_FAILED(VtIo::ParseIoMode(L"garbage", &mode));

}

void VtIoTests::BasicPipeOpeningTest()
{
    
    Log::Comment(L"Just the basic case, two seperate pipes, expected modes");
    // DUPLEX is definitely acceptable for the inpipe, but I think we ideally want non-overlapped...
    // I don't do anything overlapped, so I thin k that flag doesn't need to be there.
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
    VERIFY_SUCCEEDED(vtio.Initialize(inPipeName, outPipeName, L""));
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
    // DUPLEX is definitely acceptable for the inpipe, but I think we ideally want non-overlapped...
    // I don't do anything overlapped, so I thin k that flag doesn't need to be there.
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
    VERIFY_FAILED(vtio.Initialize(inPipeName, outPipeName, L""));
    VERIFY_IS_FALSE(vtio.IsUsingVt());

    Log::Comment(L"\tconnecting the pipes");

}

void VtIoTests::NoOutPipeOpeningTest()
{
    Log::Comment(L"Don't create an out pipe, see what happens");
    // DUPLEX is definitely acceptable for the inpipe, but I think we ideally want non-overlapped...
    // I don't do anything overlapped, so I thin k that flag doesn't need to be there.
    DWORD inPipeOpenMode = PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED;

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
    VERIFY_FAILED(vtio.Initialize(inPipeName, outPipeName, L""));
    VERIFY_IS_FALSE(vtio.IsUsingVt());

}

void VtIoTests::NonOverlappedInModeTest()
{
    Log::Comment(L"change the input mode to non-overlapped");
    // DUPLEX is definitely acceptable for the inpipe, but I think we ideally want non-overlapped...
    // I don't do anything overlapped, so I thin k that flag doesn't need to be there.
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
    VERIFY_SUCCEEDED(vtio.Initialize(inPipeName, outPipeName, L""));
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
    // I'm not sure about the final state of this yet. 
    //  We may want to force the pipe to be bidirectional... 


    // DUPLEX is definitely acceptable for the inpipe, but I think we ideally want non-overlapped...
    // I don't do anything overlapped, so I thin k that flag doesn't need to be there.
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
    VERIFY_SUCCEEDED(vtio.Initialize(inPipeName, outPipeName, L""));
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
    VERIFY_FAILED(vtio.Initialize(inPipeName, outPipeName, L""));
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
    VERIFY_FAILED(vtio.Initialize(inPipeName, outPipeName, L""));
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
    VERIFY_SUCCEEDED(vtio.Initialize(inPipeName, outPipeName, L""));
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
    VERIFY_FAILED(vtio.Initialize(onePipeName, onePipeName, L""));
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
    VERIFY_FAILED(vtio.Initialize(onePipeName, onePipeName, L""));
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
    VERIFY_FAILED(vtio.Initialize(inPipeName, outPipeName, L""));
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
    VERIFY_FAILED(vtio.Initialize(inPipeName, outPipeName, L""));
    VERIFY_IS_FALSE(vtio.IsUsingVt());

}