// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

#pragma once

#include "Parser.g.h"
#include "../parser/stateMachine.hpp"
#include "../terminal/adapter/termDispatch.hpp"

namespace winrt::VtStateMachine::implementation
{
    struct Parser : ParserT<Parser>
    {
        Parser() = delete;
        Parser(const VtStateMachine::ITerminalDispatch& dispatch);
        bool ProcessString(const winrt::hstring& str);

    private:
        VtStateMachine::ITerminalDispatch _dispatch;
        Microsoft::Console::VirtualTerminal::StateMachine* _sm;
    };
}

namespace winrt::VtStateMachine::factory_implementation
{
    struct Parser : ParserT<Parser, implementation::Parser>
    {
    };
}
