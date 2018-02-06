/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"
#include "ConsoleArguments.hpp"
#include <shellapi.h>

const std::wstring ConsoleArguments::VT_IN_PIPE_ARG = L"--inpipe";
const std::wstring ConsoleArguments::VT_OUT_PIPE_ARG = L"--outpipe";
const std::wstring ConsoleArguments::VT_MODE_ARG = L"--vtmode";
const std::wstring ConsoleArguments::HEADLESS_ARG = L"--headless";
const std::wstring ConsoleArguments::SERVER_HANDLE_ARG = L"--server";
const std::wstring ConsoleArguments::SIGNAL_HANDLE_ARG = L"--signal";
const std::wstring ConsoleArguments::HANDLE_PREFIX = L"0x";
const std::wstring ConsoleArguments::CLIENT_COMMANDLINE_ARG = L"--";
const std::wstring ConsoleArguments::FORCE_V1_ARG = L"-ForceV1";
const std::wstring ConsoleArguments::FILEPATH_LEADER_PREFIX = L"\\??\\";
const std::wstring ConsoleArguments::WIDTH_ARG = L"--width";
const std::wstring ConsoleArguments::HEIGHT_ARG = L"--height";
const std::wstring ConsoleArguments::INHERIT_CURSOR_ARG = L"--inheritcursor";

ConsoleArguments::ConsoleArguments(_In_ const std::wstring& commandline,
                                   _In_ const HANDLE hStdIn,
                                   _In_ const HANDLE hStdOut)
    : _commandline(commandline),
      _vtInHandle(hStdIn),
      _vtOutHandle(hStdOut)
{
    _clientCommandline = L"";
    _vtInPipe = L"";
    _vtOutPipe = L"";
    _vtMode = L"";
    _headless = false;
    _createServerHandle = true;
    _serverHandle = 0;
    _signalHandle = 0;
    _forceV1 = false;
    _width = 0;
    _height = 0;
    _inheritCursor = true;
}

ConsoleArguments& ConsoleArguments::operator=(const ConsoleArguments & other)
{
    if (this != &other)
    {
        _commandline = other._commandline;
        _clientCommandline = other._clientCommandline;
        _vtInHandle = other._vtInHandle;
        _vtOutHandle = other._vtOutHandle;
        _vtInPipe = other._vtInPipe;
        _vtOutPipe = other._vtOutPipe;
        _vtMode = other._vtMode;
        _headless = other._headless;
        _createServerHandle = other._createServerHandle;
        _serverHandle = other._serverHandle;
        _signalHandle = other._signalHandle;
        _forceV1 = other._forceV1;
        _width = other._width;
        _height = other._height;
        _inheritCursor = other._inheritCursor;
    }

    return *this;
}

// Routine Description:
// - Consumes the argument at the given index off of the vector.
// Arguments:
// - args - The vector full of args
// - index - The item to consume/remove from the vector.
// Return Value:
// - <none>
void ConsoleArguments::s_ConsumeArg(_Inout_ std::vector<std::wstring>& args, _In_ size_t& index)
{
    args.erase(args.begin() + index);
}

// Routine Description:
//  Given the commandline of tokens `args`, tries to find the argument at
//      index+1, and places it's value into pSetting.
//  If there aren't enough args, then returns E_INVALIDARG.
//  If we found a value, then we take the elements at both index and index+1 out
//      of args. We'll also decrement index, so that a caller who is using index
//      as a loop index will autoincrement it to have it point at the correct
//      next index.
//
// EX: for args=[--foo, bar, --baz]
//      index=0 would place "bar" in pSetting,
//          args is now [--baz], index is now -1, caller increments to 0
//      index=2 would return E_INVALIDARG,
//          args is still [--foo, bar, --baz], index is still 2, caller increments to 3.
// Arguments:
//  args: A collection of wstrings representing command-line arguments
//  index: the index of the argument of which to get the value for. The value
//      should be at (index+1). index will be decremented by one on success.
//  pSetting: recieves the string at index+1
// Return Value:
//  S_OK if we parsed the string successfully, otherwise E_INVALIDARG indicating
//      failure.
HRESULT ConsoleArguments::s_GetArgumentValue(_Inout_ std::vector<std::wstring>& args, _Inout_ size_t& index, _Out_opt_ std::wstring* const pSetting)
{
    bool hasNext = (index + 1) < args.size();
    if (hasNext)
    {
        s_ConsumeArg(args, index);
        if (pSetting != nullptr)
        {
            *pSetting = args[index];
        }
        s_ConsumeArg(args, index);
    }
    return (hasNext) ? S_OK : E_INVALIDARG;
}

// Method Description:
// Routine Description:
//  Given the commandline of tokens `args`, tries to find the argument at
//      index+1, and places it's value into pSetting. See above for examples.
//  This implementation attempts to parse a short from the argument.
// Arguments:
//  args: A collection of wstrings representing command-line arguments
//  index: the index of the argument of which to get the value for. The value
//      should be at (index+1). index will be decremented by one on success.
//  pSetting: recieves the short at index+1
// Return Value:
//  S_OK if we parsed the short successfully, otherwise E_INVALIDARG indicating
//      failure. This could be the case for non-numeric arguments, or for >SHORT_MAX args.
HRESULT ConsoleArguments::s_GetArgumentValue(_Inout_ std::vector<std::wstring>& args,
                                             _Inout_ size_t& index,
                                             _Out_opt_ short* const pSetting)
{
    bool succeeded = (index + 1) < args.size();
    if (succeeded)
    {
        s_ConsumeArg(args, index);
        if (pSetting != nullptr)
        {
            try
            {
                size_t pos = 0;
                int value = std::stoi(args[index], &pos);
                // If the entire string was a number, pos will be equal to the
                //      length of the string. Otherwise, a string like 8foo will
                //       be parsed as "8"
                if (value > SHORT_MAX || pos != args[index].length())
                {
                    succeeded = false;
                }
                else
                {
                    *pSetting = static_cast<short>(value);
                    succeeded = true;
                }
            }
            catch (...)
            {
                succeeded = false;
            }

        }
        s_ConsumeArg(args, index);
    }
    return (succeeded) ? S_OK : E_INVALIDARG;
}

// Routine Description:
// - Parsing helper that will turn a string into a handle value if possible.
// Arguments:
// - handleAsText - The string representation of the handle that was passed in on the command line
// - handleAsVal - The location to store the value if we can appropriately convert it.
// Return Value:
// - S_OK if we could successfully parse the given text and store it in the handle value location.
// - E_INVALIDARG if we couldn't parse the text as a valid hex-encoded handle number OR
//                if the handle value was already filled.
HRESULT ConsoleArguments::s_ParseHandleArg(_In_ const std::wstring& handleAsText, _Inout_ DWORD& handleAsVal)
{
    HRESULT hr = S_OK;

    // The handle should have a valid prefix.
    if (handleAsText.substr(0, HANDLE_PREFIX.length()) != HANDLE_PREFIX)
    {
        hr = E_INVALIDARG;
    }
    else if (0 == handleAsVal)
    {
        handleAsVal = wcstoul(handleAsText.c_str(), nullptr /*endptr*/, 16 /*base*/);

        // If the handle didn't parse into a reasonable handle ID, invalid.
        if (handleAsVal == 0)
        {
            hr = E_INVALIDARG;
        }
    }
    else
    {
        // If we're trying to set the handle a second time, invalid.
        hr = E_INVALIDARG;
    }

    return hr;
}

// Routine Description:
//  Given the commandline of tokens `args`, creates a wstring containing all of
//      the remaining args after index joined with spaces.  If skipFirst==true,
//      then we omit the argument at index from this finished string. skipFirst
//      should only be true if the first arg is
//      ConsoleArguments::CLIENT_COMMANDLINE_ARG. Removes all the args starting
//      at index from the collection.
//  The finished commandline is placed in _clientCommandline
// Arguments:
//  args: A collection of wstrings representing command-line arguments
//  index: the index of the argument of which to start the commandline from.
//  skipFirst: if true, omit the arg at index (which should be "--")
// Return Value:
//  S_OK if we parsed the string successfully, otherwise E_INVALIDARG indicating
//       failure.
HRESULT ConsoleArguments::_GetClientCommandline(_In_ std::vector<std::wstring>& args, _In_ const size_t index, _In_ const bool skipFirst)
{
    auto start = args.begin()+index;

    // Erase the first token.
    //  Used to get rid of the explicit commandline token "--"
    if (skipFirst)
    {
        // Make sure that the arg we're deleting is "--"
        assert(CLIENT_COMMANDLINE_ARG == start->c_str());
        args.erase(start);
    }

    _clientCommandline = L"";
    size_t j = 0;
    for (j = index; j < args.size(); j++)
    {
        _clientCommandline += args[j];
        if (j+1 < args.size())
        {
            _clientCommandline += L" ";
        }
    }
    args.erase(args.begin()+index, args.begin()+j);

    return S_OK;
}

// Routine Description:
//  Attempts to parse the commandline that this ConsoleArguments was initialized
//      with. Fills all of our members with values that were specified on the
//      commandline.
// Arguments:
//  <none>
// Return Value:
//  S_OK if we parsed our _commandline successfully, otherwise E_INVALIDARG
//      indicating failure.
HRESULT ConsoleArguments::ParseCommandline()
{
    // If the commandline was empty, quick return.
    if (_commandline.length() == 0)
    {
        return S_OK;
    }

    std::vector<std::wstring> args;
    HRESULT hr = S_OK;

    // Make a mutable copy of the commandline for tokenizing
    std::wstring copy = _commandline;

    // Tokenize the commandline
    int argc = 0;
    wil::unique_hlocal_ptr<PWSTR[]> argv;
    argv.reset(CommandLineToArgvW(copy.c_str(), &argc));
    RETURN_LAST_ERROR_IF(argv == nullptr);

    for (int i = 1; i < argc; ++i)
    {
        args.push_back(argv[i]);
    }

    // Parse args out of the commandline.
    //  As we handle a token, remove it from the args.
    //  At the end of parsing, there should be nothing left.
    for (size_t i = 0; i < args.size();)
    {
        hr = E_INVALIDARG;

        std::wstring arg = args[i];

        if (arg.substr(0, HANDLE_PREFIX.length()) == HANDLE_PREFIX ||
                 arg == SERVER_HANDLE_ARG)
        {
            // server handle token accepted two ways:
            // --server 0x4 (new method)
            // 0x4 (legacy method)
            // If we see >1 of these, it's invalid.
            std::wstring serverHandleVal = arg;

            if (arg == SERVER_HANDLE_ARG)
            {
                hr = s_GetArgumentValue(args, i, &serverHandleVal);
            }
            else
            {
                s_ConsumeArg(args, i);
                hr = S_OK;
            }

            if (SUCCEEDED(hr))
            {
                hr = s_ParseHandleArg(serverHandleVal, _serverHandle);
                if (SUCCEEDED(hr))
                {
                    _createServerHandle = false;
                }
            }
        }
        else if (arg == SIGNAL_HANDLE_ARG)
        {
            std::wstring signalHandleVal;
            hr = s_GetArgumentValue(args, i, &signalHandleVal);

            if (SUCCEEDED(hr))
            {
                hr = s_ParseHandleArg(signalHandleVal, _signalHandle);
            }
        }
        else if (arg == FORCE_V1_ARG)
        {
            // -ForceV1 command line switch for NTVDM support
            _forceV1 = true;
            s_ConsumeArg(args, i);
            hr = S_OK;
        }
        else if (arg.substr(0, FILEPATH_LEADER_PREFIX.length()) == FILEPATH_LEADER_PREFIX)
        {
            // beginning of command line -- includes file path
            // skipped for historical reasons.
            s_ConsumeArg(args, i);
            hr = S_OK;
        }
        else if (arg == VT_IN_PIPE_ARG)
        {
            // It's only valid to capture one of these if we weren't also passed a valid handle
            // on the process startup parameters through our standard handles.
            if (!s_IsValidHandle(_vtInHandle))
            {
                hr = s_GetArgumentValue(args, i, &_vtInPipe);
            }
            else
            {
                hr = E_INVALIDARG;
            }
        }
        else if (arg == VT_OUT_PIPE_ARG)
        {
            if (!s_IsValidHandle(_vtOutHandle))
            {
                hr = s_GetArgumentValue(args, i, &_vtOutPipe);
            }
            else
            {
                hr = E_INVALIDARG;
            }
        }
        else if (arg == VT_MODE_ARG)
        {
            hr = s_GetArgumentValue(args, i, &_vtMode);
        }
        else if (arg == WIDTH_ARG)
        {
            hr = s_GetArgumentValue(args, i, &_width);
        }
        else if (arg == HEIGHT_ARG)
        {
            hr = s_GetArgumentValue(args, i, &_height);
        }
        else if (arg == HEADLESS_ARG)
        {
            _headless = true;
            s_ConsumeArg(args, i);
            hr = S_OK;
        }
        else if (arg == INHERIT_CURSOR_ARG)
        {
            _inheritCursor = true;
            s_ConsumeArg(args, i);
            hr = S_OK;
        }
        else if (arg == CLIENT_COMMANDLINE_ARG)
        {
            // Everything after this is the explicit commandline
            hr = _GetClientCommandline(args, i, true);
            break;
        }
        // TODO: handle the rest of the possible params (MSFT:13271366, MSFT:13631640)
        // TODO: handle invalid args
        //  eg "conhost --foo bar" should not make the clientCommandline "--foo bar"
        else
        {
            // If we encounter something that doesn't match one of our other
            //      args, then it's the start of the commandline
            hr = _GetClientCommandline(args, i, false);
            break;
        }

        if (FAILED(hr))
        {
            break;
        }
    }

    // We should have consumed every token at this point.
    // if not, it is some sort of parsing error.
    // If we failed to parse an arg, then no need to assert.
    if (SUCCEEDED(hr))
    {
        assert(args.size() == 0);
    }

    return hr;
}

// Routine Description:
// - Returns true if we already have opened handles to use for the VT server
//   streams.
// - If false, try next to see if we have pipe names to open instead.
// Arguments:
// - <none> - uses internal state
// Return Value:
// - True or false (see description)
bool ConsoleArguments::HasVtHandles() const
{
    return s_IsValidHandle(_vtInHandle) && s_IsValidHandle(_vtOutHandle);
}

// Routine Description:
// - Returns true if we were passed a seemingly valid signal handle on startup.
// Arguments:
// - <none> - uses internal state
// Return Value:
// - True or false (see description)
bool ConsoleArguments::HasSignalHandle() const
{
    return s_IsValidHandle(GetSignalHandle());
}

// Routine Description:
//  Returns true if according to the arguments parsed from _commandline we
//      should start with the VT pipe enabled. This is when we have both a VT
//      input and output pipe name given. Guarentees nothing about the pipe
//      itself or even the strings, just whether or not they were specified.
// Arguments:
//  <none>
// Return Value:
//  true iff we have parsed both a VT input and output pipe name.
bool ConsoleArguments::IsUsingVtPipe() const
{
    return (_vtInPipe.length() > 0) && (_vtOutPipe.length() > 0);
}

bool ConsoleArguments::IsHeadless() const
{
    return _headless;
}

bool ConsoleArguments::ShouldCreateServerHandle() const
{
    return _createServerHandle;
}

HANDLE ConsoleArguments::GetServerHandle() const
{
    return ULongToHandle(_serverHandle);
}

HANDLE ConsoleArguments::GetSignalHandle() const
{
    return ULongToHandle(_signalHandle);
}

HANDLE ConsoleArguments::GetVtInHandle() const
{
    return _vtInHandle;
}

HANDLE ConsoleArguments::GetVtOutHandle() const
{
    return _vtOutHandle;
}

std::wstring ConsoleArguments::GetClientCommandline() const
{
    return _clientCommandline;
}

std::wstring ConsoleArguments::GetVtInPipe() const
{
    return _vtInPipe;
}

std::wstring ConsoleArguments::GetVtOutPipe() const
{
    return _vtOutPipe;
}

std::wstring ConsoleArguments::GetVtMode() const
{
    return _vtMode;
}

bool ConsoleArguments::GetForceV1() const
{
    return _forceV1;
}

short ConsoleArguments::GetWidth() const
{
    return _width;
}

short ConsoleArguments::GetHeight() const
{
    return _height;
}

// Routine Description:
// - Shorthand check if a handle value is null or invalid.
// Arguments:
// - Handle
// Return Value:
// - True if non zero and not set to invalid magic value. False otherwise.
bool ConsoleArguments::s_IsValidHandle(_In_ const HANDLE handle)
{
    return handle != 0 && handle != INVALID_HANDLE_VALUE;
}

bool ConsoleArguments::GetInheritCursor() const
{
    return _inheritCursor;
}
