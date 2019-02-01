// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

#include "pch.h"
#include "Parser.h"
#include "../parser/OutputStateMachineEngine.hpp"
#include "WrapperDispatch.h"

using namespace winrt;
using namespace Microsoft::Console::VirtualTerminal;
namespace winrt::VtStateMachine::implementation
{

    Parser::Parser(const VtStateMachine::ITerminalDispatch& dispatch)
    {
		// The OutputStateMachineEngine takes ownership of the ITermDispatch
        auto engine = new OutputStateMachineEngine(new WrapperDispatch(dispatch));
        // The StateMachine takes ownership of the OutputStateMachineEngine
        _sm = new StateMachine(engine);
        _dispatch = dispatch;
        if (!_sm)
        {
            throw_hresult(E_OUTOFMEMORY);
        }
    }

    bool Parser::ProcessString(const winrt::hstring& str)
    {
        _sm->ProcessString(str.c_str(), str.size());
        return false;
    }
}
