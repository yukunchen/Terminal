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

#ifdef UNIT_TESTING
#include "WexTestClass.h"
#endif

class ConsoleArguments
{
public:
    ConsoleArguments(_In_ const std::wstring& commandline);
    ConsoleArguments() { }

    ConsoleArguments& operator=(const ConsoleArguments& other);

    HRESULT ParseCommandline();

    bool IsUsingVtPipe() const;
    bool IsHeadless() const;
    bool ShouldCreateServerHandle() const;

    HANDLE GetServerHandle() const;

    std::wstring GetClientCommandline() const;
    std::wstring GetVtInPipe() const;
    std::wstring GetVtOutPipe() const;
    std::wstring GetVtMode() const;
    bool GetForceV1() const;

    static const std::wstring VT_IN_PIPE_ARG;
    static const std::wstring VT_OUT_PIPE_ARG;
    static const std::wstring VT_MODE_ARG;
    static const std::wstring HEADLESS_ARG;
    static const std::wstring SERVER_HANDLE_ARG;
    static const std::wstring SERVER_HANDLE_PREFIX;
    static const std::wstring CLIENT_COMMANDLINE_ARG;
    static const std::wstring FORCE_V1_ARG;
    static const std::wstring FILEPATH_LEADER_PREFIX;

private:
#ifdef UNIT_TESTING
    // This accessor used to create a copy of this class for unit testing comparison ease.
    ConsoleArguments(_In_ const std::wstring commandline,
                     _In_ const std::wstring clientCommandline,
                     _In_ const std::wstring vtInPipe,
                     _In_ const std::wstring vtOutPipe,
                     _In_ const std::wstring vtMode,
                     _In_ const bool forceV1,
                     _In_ const bool headless,
                     _In_ const bool createServerHandle,
                     _In_ const DWORD serverHandle) :
        _commandline(commandline),
        _clientCommandline(clientCommandline),
        _vtInPipe(vtInPipe),
        _vtOutPipe(vtOutPipe),
        _vtMode(vtMode),
        _forceV1(forceV1),
        _headless(headless),
        _createServerHandle(createServerHandle),
        _serverHandle(serverHandle)
    {

    }
#endif

    std::wstring _commandline;

    std::wstring _clientCommandline;

    std::wstring _vtInPipe;
    std::wstring _vtOutPipe;
    std::wstring _vtMode;

    bool _forceV1;
    bool _headless;

    bool _createServerHandle;
    DWORD _serverHandle;

    HRESULT _GetClientCommandline(_In_ std::vector<std::wstring>& args, _In_ const size_t index, _In_ const bool skipFirst);

    static void s_ConsumeArg(_Inout_ std::vector<std::wstring>& args, _In_ size_t& index);
    static HRESULT s_GetArgumentValue(_Inout_ std::vector<std::wstring>& args, _Inout_ size_t& index, _Out_opt_ std::wstring* const pSetting);

#ifdef UNIT_TESTING
    friend class ConsoleArgumentsTests;
#endif
};

#ifdef UNIT_TESTING
namespace WEX {
    namespace TestExecution {
        template<>
        class VerifyOutputTraits < ConsoleArguments >
        {
        public:
            static WEX::Common::NoThrowString ToString(const ConsoleArguments& ci)
            {
                return WEX::Common::NoThrowString().Format(L"\r\nClient Command Line: '%ws', \r\nUse VT Pipe: '%ws', \r\nVT In Pipe: '%ws', \r\nVT Out Pipe: '%ws', \r\nVt Mode: '%ws', \r\nForceV1: '%ws', \r\nHeadless: '%ws', \r\nCreate Server Handle: '%ws', \r\nServer Handle: '0x%x'\r\n",
                                                           ci.GetClientCommandline().c_str(),
                                                           ci.IsUsingVtPipe() ? L"true" : L"false",
                                                           ci.GetVtInPipe().c_str(),
                                                           ci.GetVtOutPipe().c_str(),
                                                           ci.GetVtMode().c_str(),
                                                           ci.GetForceV1() ? L"true" : L"false",
                                                           ci.IsHeadless() ? L"true" : L"false",
                                                           ci.ShouldCreateServerHandle() ? L"true" : L"false",
                                                           ci.GetServerHandle());
            }
        };

        template<>
        class VerifyCompareTraits < ConsoleArguments, ConsoleArguments>
        {
        public:
            static bool AreEqual(const ConsoleArguments& expected, const ConsoleArguments& actual)
            {
                return
                    expected.GetClientCommandline() == actual.GetClientCommandline() &&
                    expected.IsUsingVtPipe() == actual.IsUsingVtPipe() &&
                    expected.GetVtInPipe() == actual.GetVtInPipe() &&
                    expected.GetVtOutPipe() == actual.GetVtOutPipe() &&
                    expected.GetVtMode() == actual.GetVtMode() &&
                    expected.GetForceV1() == actual.GetForceV1() &&
                    expected.IsHeadless() == actual.IsHeadless() &&
                    expected.ShouldCreateServerHandle() == actual.ShouldCreateServerHandle() &&
                    expected.GetServerHandle() == actual.GetServerHandle();
            }

            static bool AreSame(const ConsoleArguments& expected, const ConsoleArguments& actual)
            {
                return &expected == &actual;
            }

            static bool IsLessThan(const ConsoleArguments&, const ConsoleArguments&) = delete;

            static bool IsGreaterThan(const ConsoleArguments&, const ConsoleArguments&) = delete;

            static bool IsNull(const ConsoleArguments& object)
            {
                return
                    object.GetClientCommandline().empty() &&
                    object.GetVtInPipe().empty() &&
                    object.GetVtOutPipe().empty() &&
                    object.GetVtMode().empty() &&
                    !object.GetForceV1() &&
                    !object.IsHeadless() &&
                    !object.ShouldCreateServerHandle() &&
                    object.GetServerHandle() == 0;
            }
        };
    }
}
#endif
