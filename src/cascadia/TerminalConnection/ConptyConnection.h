#pragma once

#include "ConptyConnection.g.h"

namespace winrt::TerminalConnection::implementation
{
    struct ConptyConnection : ConptyConnectionT<ConptyConnection>
    {
        ConptyConnection() = delete;
        ConptyConnection(hstring const& commandline, uint32_t initialRows, uint32_t initialCols);

        winrt::event_token TerminalOutput(TerminalConnection::TerminalOutputEventArgs const& handler);
        void TerminalOutput(winrt::event_token const& token) noexcept;
        winrt::event_token TerminalDisconnected(TerminalConnection::TerminalDisconnectedEventArgs const& handler);
        void TerminalDisconnected(winrt::event_token const& token) noexcept;
        void Start();
        void WriteInput(hstring const& data);
        void Resize(uint32_t rows, uint32_t columns);
        void Close();

    private:
        winrt::event<TerminalConnection::TerminalOutputEventArgs> _outputHandlers;
    };
}

namespace winrt::TerminalConnection::factory_implementation
{
    struct ConptyConnection : ConptyConnectionT<ConptyConnection, implementation::ConptyConnection>
    {
    };
}
