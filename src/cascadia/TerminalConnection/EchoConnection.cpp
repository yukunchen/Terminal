/********************************************************
 *                                                       *
 *   Copyright (C) Microsoft. All rights reserved.       *
 *                                                       *
 ********************************************************/

#include "pch.h"
#include "EchoConnection.h"
#include <sstream>

namespace winrt::TerminalConnection::implementation
{
    EchoConnection::EchoConnection()
    {
    }

    winrt::event_token EchoConnection::TerminalOutput(TerminalConnection::TerminalOutputEventArgs const& handler)
    {
        return _outputHandlers.add(handler);
    }

    void EchoConnection::TerminalOutput(winrt::event_token const& token) noexcept
    {
        _outputHandlers.remove(token);
    }

    winrt::event_token EchoConnection::TerminalDisconnected(TerminalConnection::TerminalDisconnectedEventArgs const& handler)
    {
        handler;
        throw hresult_not_implemented();
    }

    void EchoConnection::TerminalDisconnected(winrt::event_token const& token) noexcept
    {
        token;
        //throw hresult_not_implemented();
    }

    void EchoConnection::Start()
    {
        // When we recieve some data:
        // hstring outputFromConpty = L"hello world";
        // //TerminalOutputEventArgs args(outputFromConpty);
        // //_outputHandlers(*this, args);
        // _outputHandlers(outputFromConpty);
    }

    void EchoConnection::WriteInput(hstring const& data)
    {
        std::wstringstream prettyPrint;
        for (wchar_t wch : data)
        {
            if (wch < 0x20)
            {
                prettyPrint << L"^" << (wchar_t)(wch+0x40);
            }
            else if (wch == 0x7f)
            {
                prettyPrint << L"0x7f";
            }
            else
            {
                prettyPrint << wch;
            }
        }
        _outputHandlers(prettyPrint.str());
    }

    void EchoConnection::Resize(uint32_t rows, uint32_t columns)
    {
        rows;
        columns;

        throw hresult_not_implemented();
    }

    void EchoConnection::Close()
    {
        throw hresult_not_implemented();
    }
}
