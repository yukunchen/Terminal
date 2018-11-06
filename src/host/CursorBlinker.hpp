/*
Copyright (c) Microsoft Corporation

Module Name:
- CursorBlinker.hpp
*/

namespace Microsoft::Console
{
    class CursorBlinker
    {
    public:
        CursorBlinker();
        ~CursorBlinker();

        void FocusStart();
        void FocusEnd();

        void UpdateSystemMetrics();
        void SettingsChanged();
        void TimerRoutine(SCREEN_INFORMATION& ScreenInfo);

    private:
        // These use Timer Queues:
        // https://msdn.microsoft.com/en-us/library/windows/desktop/ms687003(v=vs.85).aspx
        HANDLE _hCaretBlinkTimer; // timer used to peridically blink the cursor
        HANDLE _hCaretBlinkTimerQueue; // timer queue where the blink timer lives
        UINT _uCaretBlinkTime;
        void SetCaretTimer();
        void KillCaretTimer();
    };
}
