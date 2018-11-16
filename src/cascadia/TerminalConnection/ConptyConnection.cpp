#include "pch.h"
#include "ConptyConnection.h"

namespace winrt::TerminalConnection::implementation
{
    ConptyConnection::ConptyConnection(hstring const& commandline, uint32_t initialRows, uint32_t initialCols)
    {
        commandline;
        initialRows;
        initialCols;

    }

    winrt::event_token ConptyConnection::TerminalOutput(TerminalConnection::TerminalOutputEventArgs const& handler)
    {
        return _outputHandlers.add(handler);
    }

    void ConptyConnection::TerminalOutput(winrt::event_token const& token) noexcept
    {
        _outputHandlers.remove(token);
    }

    winrt::event_token ConptyConnection::TerminalDisconnected(TerminalConnection::TerminalDisconnectedEventArgs const& handler)
    {
        handler;

        throw hresult_not_implemented();
    }

    void ConptyConnection::TerminalDisconnected(winrt::event_token const& token) noexcept
    {
        token;

        // throw hresult_not_implemented();
    }

    void ConptyConnection::Start()
    {

        // When we recieve some data:
        hstring outputFromConpty = L"hello world";
        //TerminalOutputEventArgs args(outputFromConpty);
        //_outputHandlers(*this, args);
        _outputHandlers(outputFromConpty);
    }

    void ConptyConnection::WriteInput(hstring const& data)
    {
        data;

        throw hresult_not_implemented();
    }

    void ConptyConnection::Resize(uint32_t rows, uint32_t columns)
    {
        rows;
        columns;

        throw hresult_not_implemented();
    }

    void ConptyConnection::Close()
    {
        throw hresult_not_implemented();
    }
}
