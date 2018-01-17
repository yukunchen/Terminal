/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "InteractDispatch.hpp"
#include "conGetSet.hpp"
#include "../../types/inc/Viewport.hpp"
#include "../../types/inc/convert.hpp"
#include "../../inc/unicode.hpp"

using namespace Microsoft::Console::Types;
using namespace Microsoft::Console::VirtualTerminal;

InteractDispatch::InteractDispatch(_In_ std::unique_ptr<ConGetSet> pConApi)
    : _pConApi(std::move(pConApi))
{

}

// Method Description:
// - Writes a collection of input to the host. The new input is appended to the 
//      end of the input buffer. 
//  If Ctrl+C is written with this function, it will not trigger a Ctrl-C 
//      interrupt in the client, but instead write a Ctrl+C to the input buffer 
//      to be read by the client.
// Arguments:
// - inputEvents: a collection of IInputEvents
// Return Value:
// True if handled successfully. False otherwise.
bool InteractDispatch::WriteInput(_In_ std::deque<std::unique_ptr<IInputEvent>>& inputEvents) 
{
    size_t dwWritten = 0;
    return !!_pConApi->WriteConsoleInputW(inputEvents, dwWritten);
}

// Method Description:
// - Writes a Ctrl-C event to the host. The host will then decide what to do 
//      with it, including potentially sending an interrupt to a client 
//      application. 
// Arguments:
// <none>
// Return Value:
// True if handled successfully. False otherwise.
bool InteractDispatch::WriteCtrlC() 
{
    KeyEvent key = KeyEvent(true, 1, 'C', 0, UNICODE_ETX, LEFT_CTRL_PRESSED);
    return !!_pConApi->PrivateWriteConsoleControlInput(key);
}

bool InteractDispatch::WriteString(_In_reads_(cch) const wchar_t* const pws,
                                   const size_t cch)
{
    unsigned int codepage = 0;
    bool fSuccess = !!_pConApi->GetConsoleOutputCP(&codepage);
    if (fSuccess)
    {
        std::deque<std::unique_ptr<IInputEvent>> keyEvents;
        
        for (int i = 0; i < cch; ++i)
        {
            const wchar_t wch = pws[i];
            std::deque<std::unique_ptr<KeyEvent>> convertedEvents = CharToKeyEvents(wch, codepage);
            while (!convertedEvents.empty())
            {
                keyEvents.push_back(std::move(convertedEvents.front()));
                convertedEvents.pop_front();
            }
        }
        
        fSuccess = WriteInput(keyEvents);
    }
    return fSuccess;
}

//Method Description:
// Window Manipulation - Performs a variety of actions relating to the window,
//      such as moving the window position, resizing the window, querying 
//      window state, forcing the window to repaint, etc.
//  This is kept seperate from the output version, as there may be
//      codes that are supported in one direction but not the other.
//Arguments:
// - uiFunction - An identifier of the WindowManipulation function to perform
// - rgusParams - Additional parameters to pass to the function
// - cParams - size of rgusParams
// Return value:
// True if handled successfully. False otherwise.
bool InteractDispatch::WindowManipulation(_In_ const DispatchCommon::WindowManipulationType uiFunction,
                                          _In_reads_(cParams) const unsigned short* const rgusParams,
                                          _In_ size_t const cParams)
{
    bool fSuccess = false;
    // Other Window Manipulation functions:
    //  MSFT:13271098 - QueryViewport
    //  MSFT:13271146 - QueryScreenSize
    switch (uiFunction)
    {
        case DispatchCommon::WindowManipulationType::RefreshWindow:
            if (cParams == 0)
            {
                fSuccess = DispatchCommon::s_RefreshWindow(_pConApi.get());
            }
            break;
        case DispatchCommon::WindowManipulationType::ResizeWindowInCharacters:
            if (cParams == 2)
            {
                fSuccess = DispatchCommon::s_ResizeWindow(_pConApi.get(), rgusParams[1], rgusParams[0]);
            }
            break;
        default:
            fSuccess = false;
            break;
    }

    return fSuccess;
}
