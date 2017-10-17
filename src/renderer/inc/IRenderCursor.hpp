/*++
Copyright (c) Microsoft Corporation

Module Name:
- IRenderCursor.hpp

Abstract:
- Serves as an abstraction of the cursor's drawing responsibilities.
    Different engines will have differing implementations, for ex the 
    GDI engine will need a timer to control blinking the cursor, 

Author(s):
- Mike Griese (migrie) 16-Oct-2017
--*/
#pragma once

namespace Microsoft
{
    namespace Console
    {
        namespace Render
        {
            class IRenderCursor;
        }
    }
}

class Microsoft::Console::Render::IRenderCursor
{
public:
    // virtual void ShowCursor() = 0;
    // virtual void HideCursor() = 0;
    // virtual void StartBlinking() = 0;
    // virtual void StopBlinking() = 0;
    virtual void Move(_In_ const COORD cPos) = 0;
    // virtual void SetDoubleWide(_In_ const bool fDoubleWide) = 0;
    // virtual void SetSize(_In_ const unsigned long ulSize) = 0;

private:

};