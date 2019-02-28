/********************************************************
 *                                                       *
 *   Copyright (C) Microsoft. All rights reserved.       *
 *                                                       *
 ********************************************************/

#include "pch.h"
#include "Terminal.hpp"
#include "../../terminal/parser/OutputStateMachineEngine.hpp"
#include "TerminalDispatch.hpp"
#include <unicode.hpp>
#include <argb.h>
#include "../../types/inc/utils.hpp"

//#include "dll\Generated Files\winrt\Microsoft.Terminal.Core.h"
#include "winrt\Microsoft.Terminal.Core.h"

using namespace Microsoft::Terminal::Core;
using namespace Microsoft::Console::Types;
using namespace Microsoft::Console::VirtualTerminal;
using namespace Microsoft::Console::Render;

std::wstring _KeyEventsToText(std::deque<std::unique_ptr<IInputEvent>>& inEventsToWrite)
{
    std::wstring wstr = L"";
    for(auto& ev : inEventsToWrite)
    {
        if (ev->EventType() == InputEventType::KeyEvent)
        {
            auto& k = static_cast<KeyEvent&>(*ev);
            auto wch = k.GetCharData();
            wstr += wch;
        }
    }
    return wstr;
}

Terminal::Terminal() :
    _mutableViewport{Viewport::Empty()},
    _title{ L"" },
    _fontInfo{ L"Consolas", 0, 0, {8, 12}, 95001, false },
    _colorTable{},
    _defaultFg{ RGB(255, 255, 255) },
    _defaultBg{ ARGB(0, 0, 0, 0) },
    _pfnWriteInput{ nullptr },
    _scrollOffset{ 0 },
    _snapOnInput{ true }
{
    _stateMachine = std::make_unique<StateMachine>(new OutputStateMachineEngine(new TerminalDispatch(*this)));

    auto passAlongInput = [&](std::deque<std::unique_ptr<IInputEvent>>& inEventsToWrite)
    {
        if(!_pfnWriteInput) return;
        std::wstring wstr = _KeyEventsToText(inEventsToWrite);
        _pfnWriteInput(wstr);
    };

    _terminalInput = std::make_unique<TerminalInput>(passAlongInput);

    _InitializeColorTable();
}

void Terminal::Create(COORD viewportSize, SHORT scrollbackLines, IRenderTarget& renderTarget)
{
    _mutableViewport = Viewport::FromDimensions({ 0,0 }, viewportSize);
    _scrollbackLines = scrollbackLines;
    COORD bufferSize { viewportSize.X, viewportSize.Y + scrollbackLines };
    TextAttribute attr{};
    UINT cursorSize = 12;
    _buffer = std::make_unique<TextBuffer>(bufferSize, attr, cursorSize, renderTarget);
}

void Terminal::CreateFromSettings(winrt::Microsoft::Terminal::Core::ICoreSettings settings,
            Microsoft::Console::Render::IRenderTarget& renderTarget)
{
    _defaultFg = settings.DefaultForeground();
    _defaultBg = settings.DefaultBackground();

    //auto sourceTable = settings.GetColorTable();
    for (int i = 0; i < 16; i++)
    {
        _colorTable[i] = settings.GetColorTableEntry(i);
    }

    _snapOnInput = settings.SnapOnInput();
    COORD viewportSize{ (short)settings.InitialCols(), (short)settings.InitialRows() };
    Create(viewportSize, (short)settings.HistorySize(), renderTarget);
}

// Resize the terminal as the result of some user interaction.
// Returns S_OK if we successfully resized the terminal, S_FALSE if there was
//      nothing to do (the viewportSize is the same as our current size), or an
//      appropriate HRESULT for failing to resize.
HRESULT Terminal::UserResize(const COORD viewportSize)
{
    const auto oldDimensions = _mutableViewport.Dimensions();
    if (viewportSize == oldDimensions)
    {
        return S_FALSE;
    }
    _mutableViewport = Viewport::FromDimensions({ 0, 0 }, viewportSize);
    COORD bufferSize{ viewportSize.X, viewportSize.Y + _scrollbackLines };
    RETURN_IF_FAILED(_buffer->ResizeTraditional(bufferSize));
    _NotifyScrollEvent();
    return S_OK;
}

void Terminal::Write(std::wstring_view stringView)
{
    LockForWriting();
    auto a = wil::scope_exit([&]{ UnlockForWriting(); });

    _stateMachine->ProcessString(stringView.data(), stringView.size());
}

bool Terminal::SendKeyEvent(const WORD vkey,
                            const bool ctrlPressed,
                            const bool altPressed,
                            const bool shiftPressed)
{
    if (_snapOnInput)
    {
        LockForWriting();
        auto a = wil::scope_exit([&]{ UnlockForWriting(); });
        _scrollOffset = 0;
        _NotifyScrollEvent();
    }

    DWORD modifiers = 0
                      | (ctrlPressed? LEFT_CTRL_PRESSED : 0)
                      | (altPressed? LEFT_ALT_PRESSED : 0)
                      | (shiftPressed? SHIFT_PRESSED : 0)
                      ;
    KeyEvent keyEv{ true, 0, vkey, 0, L'\0', modifiers};
    return _terminalInput->HandleKey(&keyEv);
}


void Terminal::LockForReading()
{
    _readWriteLock.lock_shared();
}
void Terminal::LockForWriting()
{
    _readWriteLock.lock();
}
void Terminal::UnlockForReading()
{
    _readWriteLock.unlock_shared();
}
void Terminal::UnlockForWriting()
{
    _readWriteLock.unlock();
}

Viewport Terminal::_GetMutableViewport() const noexcept
{
    return _mutableViewport;
}

short Terminal::GetBufferHeight() const noexcept
{
    return _mutableViewport.BottomExclusive();
}

// _ViewStartIndex is also the length of the scrollback
int Terminal::_ViewStartIndex() const noexcept
{
    return _mutableViewport.Top();
}

// _VisibleStartIndex is the first visible line of the buffer
int Terminal::_VisibleStartIndex() const noexcept
{
    return std::max(0, _ViewStartIndex() - _scrollOffset);
}

Viewport Terminal::_GetVisibleViewport() const noexcept
{
    const COORD origin{ 0, gsl::narrow<short>(_VisibleStartIndex()) };
    return Viewport::FromDimensions(origin,
                                    _mutableViewport.Dimensions());
}

// Writes a string of text to the buffer, then moves the cursor (and viewport)
//      in accordance with the written text.
// This method is our proverbial `WriteCharsLegacy`, and great care should be made to
//      keep it minimal and orderly, lest it become WriteCharsLegacy2ElectricBoogaloo
void Terminal::_WriteBuffer(const std::wstring_view& stringView)
{
    auto& cursor = _buffer->GetCursor();
    static bool skipNewline = false;

    for (size_t i = 0; i < stringView.size(); i++)
    {
        wchar_t wch = stringView[i];
        const COORD cursorPosBefore = cursor.GetPosition();
        COORD proposedCursorPosition = cursorPosBefore;

        if (wch == UNICODE_LINEFEED)
        {
            if (skipNewline)
            {
                skipNewline = false;
                continue;
            }
            else
            {
                proposedCursorPosition.Y++;
            }
        }
        else if (wch == UNICODE_CARRIAGERETURN)
        {
            proposedCursorPosition.X = 0;
        }
        else if (wch == UNICODE_BACKSPACE)
        {
            if (cursorPosBefore.X == 0)
            {
                proposedCursorPosition.X = _buffer->GetSize().Width() - 1;
                proposedCursorPosition.Y--;
            }
            else
            {
                proposedCursorPosition.X--;
            }
        }
        else
        {
            _buffer->Write({ {&wch, 1} , _buffer->GetCurrentAttributes() });
            proposedCursorPosition.X++;

        }

        // This section is essentially equivalent to `AdjustCursorPosition`
        // Update Cursor Position
        cursor.SetPosition(proposedCursorPosition);

        const COORD cursorPosAfter = cursor.GetPosition();
        skipNewline = cursorPosAfter.Y == cursorPosBefore.Y+1;

        // Move the viewport down if the cursor moved below the viewport.
        if (cursorPosAfter.Y > _mutableViewport.BottomInclusive())
        {
            const auto newViewTop = std::max(0, cursorPosAfter.Y - (_mutableViewport.Height() - 1));
            if (newViewTop != _mutableViewport.Top())
            {
                _mutableViewport = Viewport::FromDimensions({0, gsl::narrow<short>(newViewTop)}, _mutableViewport.Dimensions());
                _buffer->GetRenderTarget().TriggerRedrawAll();
                _NotifyScrollEvent();
            }
        }
    }
}

void Terminal::UserScrollViewport(const int viewTop)
{
    const auto clampedNewTop = std::max(0, viewTop);
    const auto realTop = _ViewStartIndex();
    const auto newDelta = realTop - clampedNewTop;
    // if viewTop > realTop, we want the offset to be 0.

    _scrollOffset = std::max(0, newDelta);
    _buffer->GetRenderTarget().TriggerRedrawAll();
}

int Terminal::GetScrollOffset()
{
    return _VisibleStartIndex();
}

void Terminal::_NotifyScrollEvent()
{
    if (_pfnScrollPositionChanged)
    {
        const auto visible = _GetVisibleViewport();
        const auto top = visible.Top();
        const auto height = visible.Height();
        const auto bottom = this->GetBufferHeight();
        _pfnScrollPositionChanged(top, height, bottom);
    }
}

void Terminal::SetWriteInputCallback(std::function<void(std::wstring&)> pfn) noexcept
{
    _pfnWriteInput = pfn;
}

void Terminal::SetTitleChangedCallback(std::function<void(const std::wstring_view&)> pfn) noexcept
{
    _pfnTitleChanged = pfn;
}

void Terminal::SetScrollPositionChangedCallback(std::function<void(const int, const int, const int)> pfn) noexcept
{
    _pfnScrollPositionChanged = pfn;
}


void Terminal::_InitializeColorTable()
{
    gsl::span<COLORREF> tableView = { &_colorTable[0], gsl::narrow<ptrdiff_t>(_colorTable.size()) };
    // First set up the basic 256 colors
    ::Microsoft::Console::Utils::Initialize256ColorTable(tableView);
    // Then use fill the first 16 values with the Campbell scheme
    ::Microsoft::Console::Utils::InitializeCampbellColorTable(tableView);
    // Then make sure all the values have an alpha of 255
    ::Microsoft::Console::Utils::SetColorTableAlpha(tableView, 0xff);
}
