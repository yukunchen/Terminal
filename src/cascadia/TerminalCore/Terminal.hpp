#pragma once

#include "../../buffer/out/textBuffer.hpp"
#include "../../renderer/inc/DummyRenderTarget.hpp"
#include "../../terminal/parser/StateMachine.hpp"

namespace Microsoft::Terminal::Core
{
    class Terminal;
}

class Microsoft::Terminal::Core::Terminal
{
public:
    Terminal(COORD viewportSize, SHORT scrollbackLines);

private:
    DummyRenderTarget _renderTarget;
    std::unique_ptr<TextBuffer> _buffer;
    std::unique_ptr<::Microsoft::Console::VirtualTerminal::StateMachine> _stateMachine;



};
