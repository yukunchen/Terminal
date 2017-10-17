/*++
Copyright (c) Microsoft Corporation

Module Name:
- GdiCursor.hpp

Abstract:
- Serves as an abstraction of the cursor's drawing responsibilities.
    Different engines will have differing implementations, for ex the 
    GDI engine will need a timer to control blinking the cursor, 

Author(s):
- Mike Griese (migrie) 16-Oct-2017
--*/

#pragma once

#include "..\inc\IRenderCursor.hpp"
#include "..\inc\IRenderEngine.hpp"

namespace Microsoft
{
    namespace Console
    {
        namespace Render
        {
            class GdiCursor;
        }
    }
}

class Microsoft::Console::Render::GdiCursor : public IRenderCursor
{
public:
    GdiCursor(IRenderEngine* const pEngine);

    // void ShowCursor() override;
    // void HideCursor() override;
    // void StartBlinking() override;
    // void StopBlinking() override;
    void Move(_In_ const COORD cPos) override;
    // void SetDoubleWide(_In_ const bool fDoubleWide) override;
    // void SetSize(_In_ const unsigned long ulSize) override;
    COORD GetPosition();

private:
    COORD _coordPosition;
    RECT _rcCursorInvert;
    IRenderEngine* const _pEngine;

    // bool _fIsVisible;  // controlled by show/hide
    // bool _fIsBlinking; // Controlled by by Start/Stop Blinking

    // bool _fDelay;    // don't blink cursor on next timer message
    
    // unsigned long _ulSize;
    
    // bool _fIsOn;   // the actual visibility of the cursor. This is toggled when it blinks.

    // // These use Timer Queues:
    // // https://msdn.microsoft.com/en-us/library/windows/desktop/ms687003(v=vs.85).aspx
    // HANDLE _hCaretBlinkTimer; // timer used to peridically blink the cursor
    // HANDLE _hCaretBlinkTimerQueue; // timer queue where the blink timer lives

    // void SetCaretTimer();
    // void KillCaretTimer();
    // UINT _uCaretBlinkTime;

};