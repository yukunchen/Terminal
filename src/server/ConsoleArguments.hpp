/*++
Copyright (c) Microsoft Corporation

Module Name:
- ConsoleArguments.hpp

Abstract:
- Encapsulates the commandline arguments to the console host.

Author(s):
- Mike Griese (migrie) 07-Sept-2017
--*/

#pragma once

class ConsoleArguments
{
public:
    ConsoleArguments(_In_ const std::wstring commandline);

    HRESULT ParseCommandline();

    bool IsUsingVtPipe() const;
    bool IsHeadless() const;
    bool ShouldCreateServerHandle() const;
    
    DWORD GetServerHandle() const;

    std::wstring GetClientCommandline() const;
    std::wstring GetVtInPipe() const;
    std::wstring GetVtOutPipe() const;
    std::wstring GetVtMode() const;

    static const std::wstring VT_IN_PIPE_ARG;
    static const std::wstring VT_OUT_PIPE_ARG;
    static const std::wstring VT_MODE_ARG;
    static const std::wstring HEADLESS_ARG;
    static const std::wstring SERVER_HANDLE_ARG;
    static const std::wstring CLIENT_COMMANDLINE_ARG;

private:
    const std::wstring _commandline;

    std::wstring _clientCommandline;

    std::wstring _vtInPipe;
    std::wstring _vtOutPipe;
    std::wstring _vtMode;

    bool _headless;

    bool _createServerHandle;
    DWORD _serverHandle;

    HRESULT _GetClientCommandline(_In_ std::vector<std::wstring>& args, _In_ const size_t index, _In_ const bool skipFirst);
};