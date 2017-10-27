/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "InteractDispatch.hpp"
#include "conGetSet.hpp"
#include "../../types/inc/Viewport.hpp"

using namespace Microsoft::Console::Types;
using namespace Microsoft::Console::VirtualTerminal;

InteractDispatch::InteractDispatch(_In_ std::unique_ptr<ConGetSet> pConApi)
    : _pConApi(std::move(pConApi))
{

}

// Method Description:
// - Writes a collection of input to the host. The new input is appended to the 
//      end of the input buffer.
// Arguments:
// - inputEvents: a collection of IInputEvents
// Return Value:
// True if handled successfully. False otherwise.
bool InteractDispatch::WriteInput(_In_ std::deque<std::unique_ptr<IInputEvent>>& inputEvents) 
{
    size_t dwWritten = 0;
    return !!_pConApi->WriteConsoleInputW(inputEvents, dwWritten);
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
    //  MSFT:14179497 - RefreshWindow
    switch (uiFunction)
    {
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
