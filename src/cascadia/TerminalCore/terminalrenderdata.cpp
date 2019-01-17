#include "precomp.h"
#include "Terminal.hpp"
using namespace Microsoft::Terminal::Core;
using namespace Microsoft::Console::Types;
using namespace Microsoft::Console::Render;

Viewport Terminal::GetViewport()
{
    return _GetVisibleViewport();
}

const TextBuffer& Terminal::GetTextBuffer()
{
    return *_buffer;
}

const FontInfo* Terminal::GetFontInfo()
{
    return &_fontInfo;
}

const TextAttribute Terminal::GetDefaultBrushColors()
{
    return TextAttribute{};
}

const void Terminal::GetColorTable(COLORREF** const /*ppColorTable*/,
                                   size_t* const /*pcColors*/)
{
    THROW_HR(E_NOTIMPL);
}

const COLORREF Terminal::GetForegroundColor(const TextAttribute& attr) const
{
    return attr.CalculateRgbForeground({ &_colorTable[0], _colorTable.size() }, _defaultFg, _defaultBg);
}

const COLORREF Terminal::GetBackgroundColor(const TextAttribute& attr) const
{
    return attr.CalculateRgbBackground({ &_colorTable[0], _colorTable.size() }, _defaultFg, _defaultBg);
}

COORD Terminal::GetCursorPosition() const
{
    const auto& cursor = _buffer->GetCursor();
    return cursor.GetPosition();
}

bool Terminal::IsCursorVisible() const
{
    const auto& cursor = _buffer->GetCursor();
    return cursor.IsVisible() && !cursor.IsPopupShown();
}

bool Terminal::IsCursorOn() const
{
    const auto& cursor = _buffer->GetCursor();
    return cursor.IsOn();
}

ULONG Terminal::GetCursorPixelWidth() const
{
    return 1;
}

ULONG Terminal::GetCursorHeight() const
{
    return _buffer->GetCursor().GetSize();
}

CursorType Terminal::GetCursorStyle() const
{
    return _buffer->GetCursor().GetType();
}

COLORREF Terminal::GetCursorColor() const
{
    return INVALID_COLOR;
}

bool Terminal::IsCursorDoubleWidth() const
{
    return false;
}

const std::vector<RenderOverlay> Terminal::GetOverlays() const
{
    THROW_HR(E_NOTIMPL);
}

const bool Terminal::IsGridLineDrawingAllowed()
{
    return true;
}

std::vector<SMALL_RECT> Terminal::GetSelectionRects()
{
    return {};
}

const std::wstring Terminal::GetConsoleTitle() const
{
    return _title;
}

void Terminal::LockConsole() noexcept
{
    LockForReading();
}

void Terminal::UnlockConsole() noexcept
{
    UnlockForReading();
}
