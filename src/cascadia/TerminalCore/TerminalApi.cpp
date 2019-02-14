/********************************************************
 *                                                       *
 *   Copyright (C) Microsoft. All rights reserved.       *
 *                                                       *
 ********************************************************/

#include "precomp.h"
#include "Terminal.hpp"

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
    // TODO: Right now Terminal is using the "legacy attributes" mode of TextAttributes 
    //      to store 256colors as indicies into the table. However, on storing those 
    //      values in the TextAttribute, we bitwise AND with FG_ATTRS, which is 0x0f. 
    //      What we need to do is remove that AND, clamp the value instead to 255, 
    //      and make sure that both conhost and Cascadia can render TextColors that 
    //      have a 256 color table, instead of just a 16 color table.
    TextAttribute attrs = _buffer->GetCurrentAttributes();
    attrs.SetLegacyAttributes(colorIndex, true, false, false);
    _buffer->SetCurrentAttributes(attrs);
    return true;
}

bool Terminal::SetTextBackgroundIndex(BYTE colorIndex)
{
    // TODO: see above note in SetTextForegroundIndex

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

COORD Terminal::GetCursorPosition()
{
    const auto absoluteCursorPos = _buffer->GetCursor().GetPosition();
    const auto viewport = _GetMutableViewport();
    const auto viewOrigin = viewport.Origin();
    const short relativeX = absoluteCursorPos.X - viewOrigin.X;
    const short relativeY = absoluteCursorPos.Y - viewOrigin.Y;
    COORD newPos{ relativeX, relativeY };

    // TODO assert that the coord is > (0, 0) && <(view.W, view.H)
    return newPos;
}

bool Terminal::EraseCharacters(const unsigned int numChars)
{
    const auto absoluteCursorPos = _buffer->GetCursor().GetPosition();
    const auto viewport = _GetMutableViewport();
    const short distanceToRight = viewport.RightExclusive() - absoluteCursorPos.X;
    const short fillLimit = min(static_cast<short>(numChars), distanceToRight);
    auto eraseIter = OutputCellIterator(L' ', fillLimit);
    _buffer->Write(eraseIter, absoluteCursorPos);
    return true;
}

bool Terminal::SetWindowTitle(std::wstring_view title)
{
    _title = title;

    if (_pfnTitleChanged)
    {
        _pfnTitleChanged(title);
    }

    return true;
}
