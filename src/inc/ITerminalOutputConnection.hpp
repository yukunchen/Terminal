/*++
Copyright (c) Microsoft Corporation

Module Name:
- IDefaultColorProvider.hpp

Abstract:
- Provides an abstraction for aquiring the default colors of a console object.

Author(s):
- Mike Griese (migrie) 11 Oct 2017
--*/

#pragma once

namespace Microsoft::Console
{
    class ITerminalOutputConnection
    {
    public:
        virtual ~ITerminalOutputConnection() = 0;

        [[nodiscard]]
        virtual HRESULT WriteTerminal(_In_ const std::string& str) = 0;
    };

    inline Microsoft::Console::ITerminalOutputConnection::~ITerminalOutputConnection() { }
}
