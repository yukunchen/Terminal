/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"
#include "ConsoleArguments.hpp"

const std::wstring ConsoleArguments::VT_IN_PIPE_ARG = L"--inpipe";
const std::wstring ConsoleArguments::VT_OUT_PIPE_ARG = L"--outpipe";
const std::wstring ConsoleArguments::VT_MODE_ARG = L"--vtmode";
const std::wstring ConsoleArguments::HEADLESS_ARG = L"--headless";
const std::wstring ConsoleArguments::SERVER_HANDLE_ARG = L"--handle";
const std::wstring ConsoleArguments::CLIENT_COMMANDLINE_ARG = L"--";

ConsoleArguments::ConsoleArguments(_In_ const std::wstring& commandline)
    : _commandline(commandline)
{
    _clientCommandline = L"";
    _vtInPipe = L"";
    _vtOutPipe = L"";
    _vtMode = L"";
    _headless = false;
    _createServerHandle = true;
    _serverHandle = 0;
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
// EX: for args=[--foo bar --baz]
//      index 0 would place "bar" in pSetting, 
//          args is now [--baz], index is now -1, caller increments to 0
//      index 2 would return E_INVALIDARG,
//          args is still [--foo bar --baz], index is still 2, caller increments to 3.
// Arguments:
//  args: A collection of wstrings representing command-line arguments
//  index: the index of the argument of which to get the value for. The value 
//      should be at (index+1). index will be decremented by one on success.
//  pSetting: recieves the string at index+1
// Return Value:
//  S_OK if we parsed the string successfully, otherwise E_INVALIDARG indicating
//      failure.
HRESULT _GetArgumentValue(_In_ std::vector<std::wstring>& args, _Inout_ size_t& index, _Out_opt_ std::wstring* const pSetting)
{
    HRESULT hr = E_INVALIDARG;
    bool hasNext = (index+1) < args.size();
    if (hasNext)
    {
        args.erase(args.begin()+index);

        std::wstring next = args[index];
        if (next.compare(0, 2, L"--") == 0)
        {
            // the next token starts with --, so it's an argument.
            // eg: [--foo, --bar, baz]
            // We should reject this. Parameters to args shouldn't start with --
        }
        else
        {
            if (pSetting != nullptr)
            {
                *pSetting = args[index];
            }
            args.erase(args.begin()+index);
            hr = S_OK;
        }
        index--;
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
    std::vector<std::wstring> args;
    HRESULT hr = S_OK;

    // Make a mutable copy of the commandline for tokenizing
    std::wstring copy = _commandline;
    
    // Tokenize the commandline
    wchar_t* pszNextToken;
    wchar_t* pszToken = wcstok_s(&copy[0], L" \t", &pszNextToken);
    while (pszToken != nullptr && *pszToken != L'\0')
    {
        args.push_back(pszToken);
        pszToken = wcstok_s(nullptr, L" \t", &pszNextToken);
    }

    // Parse args out of the commandline.
    //  As we handle a token, remove it from the args.
    //  At the end of parsing, there should be nothing left.
    for (size_t i = 0; i < args.size(); i++)
    {
        std::wstring arg = args[i];

        if (arg == VT_IN_PIPE_ARG)
        {
            hr = _GetArgumentValue(args, i, &_vtInPipe);
        }
        else if (arg == VT_OUT_PIPE_ARG)
        {
            hr = _GetArgumentValue(args, i, &_vtOutPipe);
        }
        else if (arg == VT_MODE_ARG)
        {
            hr = _GetArgumentValue(args, i, &_vtMode);
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
        if (!SUCCEEDED(hr))
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
    const bool useVtIn = _vtInPipe.length() > 0;
    const bool useVtOut = _vtOutPipe.length() > 0;
    return useVtIn && useVtOut;
}

bool ConsoleArguments::IsHeadless() const
{
    return _headless;
}

bool ConsoleArguments::ShouldCreateServerHandle() const
{
    return _createServerHandle;
}

DWORD ConsoleArguments::GetServerHandle() const
{
    return _serverHandle;
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
