#pragma once

#include "../../buffer/out/textBuffer.hpp"
#include "../../renderer/inc/IRenderData.hpp"
#include "../../terminal/parser/StateMachine.hpp"
#include "../../terminal/input/terminalInput.hpp"

#include "../../types/inc/Viewport.hpp"

#include "../../cascadia/terminalcore/ITerminalApi.hpp"
#include "../../cascadia/terminalcore/ITerminalInput.hpp"

namespace Microsoft::Terminal::Core
{
    class Terminal;
}

class Microsoft::Terminal::Core::Terminal final :
    public Microsoft::Terminal::Core::ITerminalApi,
    public Microsoft::Terminal::Core::ITerminalInput,
    public Microsoft::Console::Render::IRenderData
{
public:
    Terminal();
    virtual ~Terminal() {};

    void Create(COORD viewportSize,
                SHORT scrollbackLines,
                Microsoft::Console::Render::IRenderTarget& renderTarget);

    TextBuffer& GetBuffer(); // TODO Remove this - use another interface to get what you actually need

    // Write goes through the parser
    void Write(std::wstring_view stringView);

    #pragma region ITerminalApi
    bool PrintString(std::wstring_view stringView) override;
    bool ExecuteChar(wchar_t wch) override;
    bool SetForegroundIndex(BYTE colorIndex) override;
    bool SetBackgroundIndex(BYTE colorIndex) override;
    #pragma endregion

    #pragma region ITerminalInput
    bool SendKeyEvent(const WORD vkey,
                      const bool ctrlPressed,
                      const bool altPressed,
                      const bool shiftPressed) override;
    #pragma endregion

    #pragma region IRenderData
    const Microsoft::Console::Types::Viewport& GetViewport() override;
    const TextBuffer& GetTextBuffer() override;
    const FontInfo* GetFontInfo() override;
    const TextAttribute GetDefaultBrushColors() override;
    const void GetColorTable(_Outptr_result_buffer_all_(*pcColors) COLORREF** const ppColorTable,
                             _Out_ size_t* const pcColors) override;
    const COLORREF GetForegroundColor(const TextAttribute& attr) const override;
    const COLORREF GetBackgroundColor(const TextAttribute& attr) const override;
    COORD GetCursorPosition() const override;
    bool IsCursorVisible() const override;
    ULONG GetCursorHeight() const override;
    CursorType GetCursorStyle() const override;
    COLORREF GetCursorColor() const override;
    bool IsCursorDoubleWidth() const override;
    const ConsoleImeInfo* GetImeData() override;
    const TextBuffer& GetImeCompositionStringBuffer(_In_ size_t iIndex) override;
    const bool IsGridLineDrawingAllowed() override;
    std::vector<SMALL_RECT> GetSelectionRects() override;
    const std::wstring GetConsoleTitle() const override;
    void LockConsole() override;
    void UnlockConsole() override;
    #pragma endregion

    std::function<void(std::wstring&)> _pfnWriteInput;

private:
    std::unique_ptr<TextBuffer> _buffer;
    std::unique_ptr<::Microsoft::Console::VirtualTerminal::StateMachine> _stateMachine;
    std::unique_ptr<::Microsoft::Console::VirtualTerminal::TerminalInput> _terminalInput;

    Microsoft::Console::Types::Viewport _visibleViewport;
    std::wstring _title;
    Microsoft::Console::Types::Viewport _GetMutableViewport();

    void _InitializeColorTable();

    std::array<COLORREF, 256> _colorTable;
    COLORREF _defaultFg;
    COLORREF _defaultBg;

    FontInfo _fontInfo;

};

