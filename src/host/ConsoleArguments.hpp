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
    ConsoleArguments(_In_ const std::wstring& commandline,
                     _In_ const HANDLE hStdIn,
                     _In_ const HANDLE hStdOut);

    ConsoleArguments() { }

    ConsoleArguments& operator=(const ConsoleArguments& other);

    HRESULT ParseCommandline();

    bool IsUsingVtPipe() const;
    bool HasVtHandles() const;
    bool IsHeadless() const;
    bool ShouldCreateServerHandle() const;

    HANDLE GetServerHandle() const;
    HANDLE GetVtInHandle() const;
    HANDLE GetVtOutHandle() const;
    
    bool HasSignalHandle() const;
    HANDLE GetSignalHandle() const;

    std::wstring GetClientCommandline() const;
    std::wstring GetVtInPipe() const;
    std::wstring GetVtOutPipe() const;
    std::wstring GetVtMode() const;
    bool GetForceV1() const;

    short GetWidth() const;
    short GetHeight() const;

    static const std::wstring VT_IN_PIPE_ARG;
    static const std::wstring VT_OUT_PIPE_ARG;
    static const std::wstring VT_MODE_ARG;
    static const std::wstring HEADLESS_ARG;
    static const std::wstring SERVER_HANDLE_ARG;
    static const std::wstring SIGNAL_HANDLE_ARG;
    static const std::wstring HANDLE_PREFIX;
    static const std::wstring CLIENT_COMMANDLINE_ARG;
    static const std::wstring FORCE_V1_ARG;
    static const std::wstring FILEPATH_LEADER_PREFIX;
    static const std::wstring WIDTH_ARG;
    static const std::wstring HEIGHT_ARG;

private:
#ifdef UNIT_TESTING
    // This accessor used to create a copy of this class for unit testing comparison ease.
    ConsoleArguments(_In_ const std::wstring commandline,
                     _In_ const std::wstring clientCommandline,
                     _In_ const HANDLE vtInHandle,
                     _In_ const HANDLE vtOutHandle,
                     _In_ const std::wstring vtInPipe,
                     _In_ const std::wstring vtOutPipe,
                     _In_ const std::wstring vtMode,
                     _In_ const short width,
                     _In_ const short height,
                     _In_ const bool forceV1,
                     _In_ const bool headless,
                     _In_ const bool createServerHandle,
                     _In_ const DWORD serverHandle,
                     _In_ const DWORD signalHandle) :
        _commandline(commandline),
        _clientCommandline(clientCommandline),
        _vtInHandle(vtInHandle),
        _vtOutHandle(vtOutHandle),
        _vtInPipe(vtInPipe),
        _vtOutPipe(vtOutPipe),
        _vtMode(vtMode),
        _width(width),
        _height(height),
        _forceV1(forceV1),
        _headless(headless),
        _createServerHandle(createServerHandle),
        _serverHandle(serverHandle),
        _signalHandle(signalHandle)
    {

    }
#endif

    std::wstring _commandline;

    std::wstring _clientCommandline;

    HANDLE _vtInHandle;
    std::wstring _vtInPipe;

    HANDLE _vtOutHandle;
    std::wstring _vtOutPipe;

    std::wstring _vtMode;

    bool _forceV1;
    bool _headless;

    short _width;
    short _height;

    bool _createServerHandle;
    DWORD _serverHandle;
    DWORD _signalHandle;

    HRESULT _GetClientCommandline(_In_ std::vector<std::wstring>& args,
                                  _In_ const size_t index,
                                  _In_ const bool skipFirst);

    static void s_ConsumeArg(_Inout_ std::vector<std::wstring>& args,
                             _In_ size_t& index);
    static HRESULT s_GetArgumentValue(_Inout_ std::vector<std::wstring>& args,
                                      _Inout_ size_t& index,
                                      _Out_opt_ std::wstring* const pSetting);
    static HRESULT s_GetArgumentValue(_Inout_ std::vector<std::wstring>& args,
                                      _Inout_ size_t& index,
                                      _Out_opt_ short* const pSetting);
    
    static HRESULT s_ParseHandleArg(_In_ const std::wstring& handleAsText,
                                    _Inout_ DWORD& handleAsVal);

    static bool s_IsValidHandle(_In_ const HANDLE handle);

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
                return WEX::Common::NoThrowString().Format(L"\r\nClient Command Line: '%ws',\r\n"
                                                           L"Use VT Handles: '%ws',\r\n"
                                                           L"VT In Handle: '0x%x',\r\n"
                                                           L"VT Out Handle: '0x%x',\r\n"
                                                           L"Use VT Pipe: '%ws',\r\n"
                                                           L"VT In Pipe: '%ws',\r\n"
                                                           L"VT Out Pipe: '%ws',\r\n"
                                                           L"Vt Mode: '%ws',\r\n"
                                                           L"WxH: '%dx%d',\r\n"
                                                           L"ForceV1: '%ws',\r\n"
                                                           L"Headless: '%ws',\r\n"
                                                           L"Create Server Handle: '%ws',\r\n"
                                                           L"Server Handle: '0x%x'\r\n"
                                                           L"Use Signal Handle: '%ws'\r\n"
                                                           L"Signal Handle: '0x%x'\r\n",
                                                           ci.GetClientCommandline().c_str(),
                                                           s_ToBoolString(ci.HasVtHandles()),
                                                           ci.GetVtInHandle(),
                                                           ci.GetVtOutHandle(),
                                                           s_ToBoolString(ci.IsUsingVtPipe()),
                                                           ci.GetVtInPipe().c_str(),
                                                           ci.GetVtOutPipe().c_str(),
                                                           ci.GetVtMode().c_str(),
                                                           ci.GetWidth(), 
                                                           ci.GetHeight(),
                                                           s_ToBoolString(ci.GetForceV1()),
                                                           s_ToBoolString(ci.IsHeadless()),
                                                           s_ToBoolString(ci.ShouldCreateServerHandle()),
                                                           ci.GetServerHandle(),
                                                           s_ToBoolString(ci.HasSignalHandle()),
                                                           ci.GetSignalHandle());
            }

        private:
            static PCWSTR s_ToBoolString(_In_ const bool val)
            {
                return val ? L"true" : L"false";
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
                    expected.HasVtHandles() == actual.HasVtHandles() &&
                    expected.GetVtInHandle() == actual.GetVtInHandle() &&
                    expected.GetVtOutHandle() == actual.GetVtOutHandle() &&
                    expected.IsUsingVtPipe() == actual.IsUsingVtPipe() &&
                    expected.GetVtInPipe() == actual.GetVtInPipe() &&
                    expected.GetVtOutPipe() == actual.GetVtOutPipe() &&
                    expected.GetVtMode() == actual.GetVtMode() &&
                    expected.GetWidth() == actual.GetWidth() &&
                    expected.GetHeight() == actual.GetHeight() &&
                    expected.GetForceV1() == actual.GetForceV1() &&
                    expected.IsHeadless() == actual.IsHeadless() &&
                    expected.ShouldCreateServerHandle() == actual.ShouldCreateServerHandle() &&
                    expected.GetServerHandle() == actual.GetServerHandle() &&
                    expected.HasSignalHandle() == actual.HasSignalHandle() &&
                    expected.GetSignalHandle() == actual.GetSignalHandle();
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
                    (object.GetVtInHandle() == 0 || object.GetVtInHandle() == INVALID_HANDLE_VALUE) &&
                    (object.GetVtOutHandle() == 0 || object.GetVtOutHandle() == INVALID_HANDLE_VALUE) && 
                    object.GetVtInPipe().empty() &&
                    object.GetVtOutPipe().empty() &&
                    object.GetVtMode().empty() &&
                    !object.GetForceV1() &&
                    (object.GetWidth() == 0) &&
                    (object.GetHeight() == 0) &&
                    !object.IsHeadless() &&
                    !object.ShouldCreateServerHandle() &&
                    object.GetServerHandle() == 0 &&
                    (object.GetSignalHandle() == 0 || object.GetSignalHandle() == INVALID_HANDLE_VALUE);
            }
        };
    }
}
#endif
