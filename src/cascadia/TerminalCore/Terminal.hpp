#pragma once

#include "../../buffer/out/textBuffer.hpp"
#include "../../renderer/inc/DummyRenderTarget.hpp"
#include "../../terminal/parser/StateMachine.hpp"

#include "../../types/inc/Viewport.hpp"

namespace Microsoft::Terminal::Core
{
    class Terminal;
}

class Microsoft::Terminal::Core::Terminal
{
public:
    Terminal(COORD viewportSize, SHORT scrollbackLines);

    TextBuffer& GetBuffer(); // TODO Remove this - use another interface to get what you actually need

    // Write goes through the parser
    void Write(std::wstring_view stringView);

    // Print puts the text in the buffer and moves the cursor
    // TODO move to ITerminalApi
    void PrintString(std::wstring_view stringView);

private:
    DummyRenderTarget _renderTarget;
    std::unique_ptr<TextBuffer> _buffer;
    std::unique_ptr<::Microsoft::Console::VirtualTerminal::StateMachine> _stateMachine;
    Microsoft::Console::Types::Viewport _visibleViewport;

    Microsoft::Console::Types::Viewport _GetMutableViewport();
};

