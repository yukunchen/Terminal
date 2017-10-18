/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "InteractDispatch.hpp"
#include "conGetSet.hpp"
#include "../../types/inc/Viewport.hpp"

#include <math.h>
#define ENABLE_INTSAFE_SIGNED_FUNCTIONS
#include <intsafe.h>

#include <assert.h>

using namespace Microsoft::Console::Types;
using namespace Microsoft::Console::VirtualTerminal;

InteractDispatch::InteractDispatch(_In_ std::unique_ptr<ConGetSet> pConApi)
    : _pConApi(std::move(pConApi))
{

}

bool InteractDispatch::CreateInstance(_In_ std::unique_ptr<ConGetSet> pConApi,
                                      _Outptr_ InteractDispatch ** const ppDispatch)
{
    bool fSuccess = false;
    InteractDispatch* const pDispatch = new InteractDispatch(std::move(pConApi));
    if (pDispatch != nullptr)
    {
        *ppDispatch = pDispatch;
        fSuccess = true;
    }
    
    return fSuccess;
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
//Arguments:
// - uiFunction - An identifier of the WindowManipulation function to perform
// - rgusParams - Additional parameters to pass to the function
// - cParams - size of rgusParams
// Return value:
// True if handled successfully. False otherwise.
bool InteractDispatch::WindowManipulation(_In_ const WindowManipulationType uiFunction,
                                       _In_reads_(cParams) const unsigned short* const rgusParams,
                                       _In_ size_t const cParams)
{
    bool fSuccess = false;
    switch (uiFunction)
    {
        case WindowManipulationType::ResizeWindowInCharacters:
            if (cParams == 2)
            {
                fSuccess = _ResizeWindow(rgusParams[1], rgusParams[0]);
            }
            break;
        default:
            fSuccess = false;
    }

    return fSuccess;
}

// Method Description:
// - Resizes the window to the specified dimensions, in characters.
// Arguments:
// - <none>
// Return Value:
// - <none>
bool InteractDispatch::_ResizeWindow(_In_ const unsigned short usWidth,
                                  _In_ const unsigned short usHeight)
{
    SHORT sColumns = 0;
    SHORT sRows = 0;

    // We should do nothing if 0 is passed in for a size.
    bool fSuccess = SUCCEEDED(UShortToShort(usWidth, &sColumns)) &&
                    SUCCEEDED(UIntToShort(usHeight, &sRows)) && 
                    (usWidth > 0 && usHeight > 0);
    
    if (fSuccess)
    {
        CONSOLE_SCREEN_BUFFER_INFOEX csbiex = { 0 };
        csbiex.cbSize = sizeof(CONSOLE_SCREEN_BUFFER_INFOEX);
        fSuccess = !!_pConApi->GetConsoleScreenBufferInfoEx(&csbiex);

        if (fSuccess)
        {
            const Viewport oldViewport = Viewport::FromInclusive(csbiex.srWindow);
            csbiex.dwSize.X = sColumns;
            // Can't just set the dwSize.Y - that's the buffer's height, not
            //      the viewport's
            fSuccess = !!_pConApi->SetConsoleScreenBufferInfoEx(&csbiex);
            if (fSuccess)
            {
                // SetConsoleWindowInfo expect inclusive rects
                SMALL_RECT sr = Viewport::FromDimensions(oldViewport.Origin(),
                                                         sColumns,
                                                         sRows).ToInclusive();
                fSuccess = !!_pConApi->SetConsoleWindowInfo(true, &sr);
            }
        }   
    }
    return fSuccess;
}
