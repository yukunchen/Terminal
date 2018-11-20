#include "precomp.h"
#include "Terminal.hpp"
#include "../../terminal/parser/OutputStateMachineEngine.hpp"
#include "TerminalDispatch.hpp"

using namespace Microsoft::Terminal::Core;
using namespace Microsoft::Console::Types;
using namespace Microsoft::Console::VirtualTerminal;
using namespace Microsoft::Console::Render;

Terminal::Terminal() :
    _visibleViewport{Viewport::Empty()},
    _title{ L"" }
{
    _stateMachine = std::make_unique<StateMachine>(new OutputStateMachineEngine(new TerminalDispatch(*this)));
}

void Terminal::Create(COORD viewportSize, SHORT scrollbackLines, IRenderTarget& renderTarget)
{
    _visibleViewport = Viewport::FromDimensions({ 0,0 }, viewportSize);
    COORD bufferSize { viewportSize.X, viewportSize.Y + scrollbackLines };
    TextAttribute attr{};
    UINT cursorSize = 12;
    _buffer = std::make_unique<TextBuffer>(bufferSize, attr, cursorSize, renderTarget);
}

Viewport Terminal::_GetMutableViewport()
{
    return _visibleViewport;
}

void Terminal::Write(std::wstring_view stringView)
{
    _stateMachine->ProcessString(stringView.data(), stringView.size());
}

TextBuffer& Terminal::GetBuffer()
{
    return *_buffer;
}
