#include "precomp.h"
#include "Terminal.hpp"
#include <unicode.hpp>

using namespace Microsoft::Terminal::Core;
using namespace Microsoft::Console::Types;
using namespace Microsoft::Console::VirtualTerminal;

// Print puts the text in the buffer and moves the cursor
bool Terminal::PrintString(std::wstring_view stringView)
{
    _WriteBuffer(stringView);
    return true;
}

bool Terminal::ExecuteChar(wchar_t wch)
{
    std::wstring_view view{&wch, 1};
    _WriteBuffer(view);
    return true;
}

void Terminal::_WriteBuffer(const std::wstring_view& stringView)
{
    auto& cursor = _buffer->GetCursor();
    static bool skipNewline = false;

    for (int i = 0; i < stringView.size(); i++)
    {
        wchar_t wch = stringView[i];
        const COORD cursorPosBefore = cursor.GetPosition();

        if (wch == UNICODE_LINEFEED)
        {
            if (skipNewline)
            {
                skipNewline = false;
                continue;
            }
            else
            {
                _buffer->NewlineCursor();
                // COORD cursorPos = cursor.GetPosition();
                // cursorPos.Y++;
                // cursor.SetPosition(cursorPos);
            }
        }
        else if (wch == UNICODE_CARRIAGERETURN)
        {
            COORD cursorPos = cursor.GetPosition();
            cursorPos.X = 0;
            cursor.SetPosition(cursorPos);
        }
        else if (wch == UNICODE_BACKSPACE)
        {
            COORD cursorPos = cursor.GetPosition();
            if (cursorPos.X == 0)
            {
                cursorPos.X = _buffer->GetSize().Width() - 1;
                cursorPos.Y--;
            }
            else
            {
                cursorPos.X--;
            }
            cursor.SetPosition(cursorPos);
        }
        else
        {
            _buffer->Write({ {&wch, 1} , _buffer->GetCurrentAttributes() });
            _buffer->IncrementCursor();

            const COORD cursorPosAfter = cursor.GetPosition();
            skipNewline = cursorPosAfter.Y == cursorPosBefore.Y+1;
        }
    }

}

bool Terminal::SetTextToDefaults(bool foreground, bool background)
{
    TextAttribute attrs = _buffer->GetCurrentAttributes();
    if (foreground)
    {
        attrs.SetDefaultForeground();
    }
    if (background)
    {
        attrs.SetDefaultBackground();
    }
    _buffer->SetCurrentAttributes(attrs);
    return true;
}

bool Terminal::SetTextForegroundIndex(BYTE colorIndex)
{
    TextAttribute attrs = _buffer->GetCurrentAttributes();
    attrs.SetLegacyAttributes(colorIndex, true, false, false);
    _buffer->SetCurrentAttributes(attrs);
    return true;
}

bool Terminal::SetTextBackgroundIndex(BYTE colorIndex)
{
    TextAttribute attrs = _buffer->GetCurrentAttributes();
    // TODO: bitshifting magic is bad
    attrs.SetLegacyAttributes(colorIndex<<4, false, true, false);
    _buffer->SetCurrentAttributes(attrs);
    return true;
}

bool Terminal::SetTextRgbColor(COLORREF color, bool foreground)
{
    TextAttribute attrs = _buffer->GetCurrentAttributes();
    attrs.SetColor(color, foreground);
    _buffer->SetCurrentAttributes(attrs);
    return true;
}

bool Terminal::BoldText(bool boldOn)
{
    TextAttribute attrs = _buffer->GetCurrentAttributes();
    if (boldOn)
    {
        attrs.Embolden();
    }
    else
    {
        attrs.Debolden();
    }
    _buffer->SetCurrentAttributes(attrs);
    return true;
}

bool Terminal::UnderlineText(bool underlineOn)
{
    TextAttribute attrs = _buffer->GetCurrentAttributes();
    WORD metaAttrs = attrs.GetMetaAttributes();
    if (underlineOn)
    {
        metaAttrs |= COMMON_LVB_UNDERSCORE;
    }
    else
    {
        metaAttrs &= ~COMMON_LVB_UNDERSCORE;
    }
    attrs.SetMetaAttributes(metaAttrs);
    _buffer->SetCurrentAttributes(attrs);
    return true;
}

bool Terminal::ReverseText(bool reversed)
{
    TextAttribute attrs = _buffer->GetCurrentAttributes();
    WORD metaAttrs = attrs.GetMetaAttributes();
    if (reversed)
    {
        metaAttrs |= COMMON_LVB_REVERSE_VIDEO;
    }
    else
    {
        metaAttrs &= ~COMMON_LVB_REVERSE_VIDEO;
    }
    attrs.SetMetaAttributes(metaAttrs);
    _buffer->SetCurrentAttributes(attrs);
    return true;
}

bool Terminal::SetCursorPosition(short x, short y)
{
    const auto viewport = _GetMutableViewport();
    const auto viewOrigin = viewport.Origin();
    const short absoluteX = viewOrigin.X + x;
    const short absoluteY = viewOrigin.Y + y;
    COORD newPos{absoluteX, absoluteY};
    viewport.Clamp(newPos);
    _buffer->GetCursor().SetPosition(newPos);

    return true;
}
