#pragma once
namespace Microsoft::Terminal::Core
{
    class ITerminalInput
    {
    public:
        virtual ~ITerminalInput() {}

        virtual bool SendKeyEvent(const WORD vkey, const bool ctrlPressed, const bool altPressed, const bool shiftPressed) = 0;

        // void SendMouseEvent(uint row, uint col, KeyModifiers modifiers);
        // void UserResize(int rows, int cols);
        // void ScrollViewport(int offset);
        // int GetScrollOffset();

    };
}
