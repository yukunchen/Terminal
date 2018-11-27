#pragma once

#include "EchoConnection.g.h"

namespace winrt::TerminalConnection::implementation
{
    struct EchoConnection : EchoConnectionT<EchoConnection>
    {
        EchoConnection();

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
    struct EchoConnection : EchoConnectionT<EchoConnection, implementation::EchoConnection>
    {
    };
}
