#include "precomp.h"
#include "Terminal.hpp"
#include "../../terminal/parser/OutputStateMachineEngine.hpp"

using namespace Microsoft::Terminal::Core;
using namespace Microsoft::Console::Types;

Terminal::Terminal(COORD viewportSize, SHORT scrollbackLines) :
    _visibleViewport{ Viewport::FromDimensions({ 0,0 }, viewportSize) }

{
    COORD bufferSize { viewportSize.X, viewportSize.Y + scrollbackLines };
    TextAttribute attr{};
    UINT cursorSize = 12;
    _buffer = std::make_unique<TextBuffer>(bufferSize, attr, cursorSize, _renderTarget);


    // TODO: StateMachine

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

void Terminal::PrintString(std::wstring_view stringView)
{
    _buffer->Write(stringView);
    for (int x = 0; x < stringView.size(); x++)
    {
        _buffer->IncrementCursor();
    }
}
