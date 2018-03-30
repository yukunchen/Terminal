/*++
Copyright (c) Microsoft Corporation

Module Name:
- ITerminalOutputConnection.hpp

Abstract:
- Provides an abstraction for writing to the output pipe connected to the TTY.
    In VtIo/pty mode, this is implemented by the VtRenderer, uch that other
    parts of the codebase (the state machine) can write VT sequences directly
    to the terminal controlling us.

Author(s):
- Mike Griese (migrie) 28 March 2018
--*/

#pragma once

namespace Microsoft::Console
{
    class ITerminalOutputConnection
    {
    public:
        virtual ~ITerminalOutputConnection() = 0;

        [[nodiscard]]
        virtual HRESULT WriteTerminalA(_In_ const std::string& str) = 0;
        [[nodiscard]]
        virtual HRESULT WriteTerminalW(_In_ const std::wstring& wstr) = 0;
    };

    inline Microsoft::Console::ITerminalOutputConnection::~ITerminalOutputConnection() { }
}
