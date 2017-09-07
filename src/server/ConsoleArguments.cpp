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

ConsoleArguments::ConsoleArguments(const std::wstring commandline)
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

ConsoleArguments::~ConsoleArguments()
{
}

HRESULT ConsoleArguments::ParseCommandline()
{
    std::vector<std::wstring> args;
    HRESULT hr = S_OK;

    // Tokenize the commandline
    // Make a mutable copy of the commandline
    std::wstring copy = _commandline;
    wchar_t* pszNextToken;
    wchar_t* pszToken = wcstok_s(&copy[0], L" ", &pszNextToken);
    while (pszToken != nullptr && *pszToken != L'\0')
    {
        args.push_back(pszToken);
        pszToken = wcstok_s(nullptr, L" ", &pszNextToken);
    }

    // Parse args out of the commandline.
    //  As we handle a token, remove it from the args.
    //  At the end of parsing, there should be nothing left.
    for (size_t i = 0; i < args.size(); i++)
    {
        std::wstring arg = args[i];
        bool hasNext = (i+1) < args.size();

        if (arg == VT_IN_PIPE_ARG)
        {
            if (hasNext)
            {
                args.erase(args.begin()+i);
                _vtInPipe = args[i];
                args.erase(args.begin()+i);
                i--;
            }
            else
            {
                hr = E_INVALIDARG;
                break;
            }
        }
        else if (arg == VT_OUT_PIPE_ARG)
        {
            if (hasNext)
            {
                args.erase(args.begin()+i);
                _vtOutPipe = args[i];
                args.erase(args.begin()+i);
                i--;
            }
            else
            {
                hr = E_INVALIDARG;
                break;
            }
        }
        else if (arg == VT_MODE_ARG)
        {
            if (hasNext)
            {
                args.erase(args.begin()+i);
                _vtMode = args[i];
                args.erase(args.begin()+i);
                i--;
            }
            else
            {
                hr = E_INVALIDARG;
                break;
            }
        }
        else if (arg == CLIENT_COMMANDLINE_ARG)
        {
            // Everything after this is the explicit commandline
            auto start = args.begin()+i;
            args.erase(start);
            _clientCommandline = L"";
            size_t j = 0;
            for (j = i; j < args.size(); j++)
            {
                _clientCommandline += args[j];
                if (j+1 < args.size())
                    _clientCommandline += L" ";
            }
            args.erase(args.begin()+i, args.begin()+j);
            break;
        }
        // TODO: handle the rest of the possible params (MSFT:13271366, MSFT:13631640)
        // TODO: handle invalid args 
        //  eg "conhost --foo bar" should not make the clientCommandline "--foo bar"
        else
        {
            // If we encounter something that doesn't match one of our other 
            //      args, then it's the start of the commandline
            auto start = args.begin()+i;
            _clientCommandline = L"";
            size_t j = 0;
            for (j = i; j < args.size(); j++)
            {
                _clientCommandline += args[j];
                if (j+1 < args.size())
                    _clientCommandline += L" ";
            }
            args.erase(args.begin()+i, args.begin()+j);
            break;
        }
    }

    if (args.size() > 0)
    {
        // This is some sort of parsing error. 
        // We should have consumed every token at this point.
    }

    return hr;
}

bool ConsoleArguments::UseVtPipe() const
{    
    const bool useVtIn = _vtInPipe.length() > 0;
    const bool useVtOut = _vtOutPipe.length() > 0;
    // (useVtIn && useVtOut) makes sense here, but would || be useful?
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
