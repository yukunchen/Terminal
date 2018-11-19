#include "precomp.h"
#include "Terminal.hpp"

using namespace Microsoft::Terminal::Core;
using namespace Microsoft::Console::Types;
using namespace Microsoft::Console::VirtualTerminal;

// Print puts the text in the buffer and moves the cursor
bool Terminal::PrintString(std::wstring_view stringView)
{
    _buffer->Write(stringView);
    for (int x = 0; x < stringView.size(); x++)
    {
        _buffer->IncrementCursor();
    }
    return true;
}

bool Terminal::ExecuteChar(wchar_t /*wch*/)
{
    _buffer->IncrementCursor();
    return true;
}
