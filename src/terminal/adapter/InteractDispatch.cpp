/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "InteractDispatch.hpp"
#include "conGetSet.hpp"

#include <math.h>
#define ENABLE_INTSAFE_SIGNED_FUNCTIONS
#include <intsafe.h>

#include <assert.h>

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


bool InteractDispatch::WriteInput(_In_ std::deque<std::unique_ptr<IInputEvent>>& inputEvents) 
{
    size_t size = inputEvents.size();
    INPUT_RECORD rgInput[64]; // This is a placeholder till I get austin's stuff
    IInputEvent::ToInputRecords(inputEvents, rgInput, size);
    DWORD dwWritten = 0;
    return !!_pConApi->WriteConsoleInputW(rgInput, (DWORD)size, &dwWritten);
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
// True if handled successfully. False othewise.
bool InteractDispatch::WindowManipulation(_In_ const WindowManipulationFunction uiFunction,
                                       _In_reads_(cParams) const unsigned short* const rgusParams,
                                       _In_ size_t const cParams)
{
    bool fSuccess = false;
    switch (uiFunction)
    {
        case WindowManipulationFunction::ResizeWindowInCharacters:
            if (cParams == 2)
            {
                fSuccess = _ResizeWindow(rgusParams[1], rgusParams[0]);
            }
            break;
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
    SHORT sColumns = 0, sRows = 0;
    bool fSuccess = SUCCEEDED(UShortToShort(usWidth, &sColumns)) &&
                    SUCCEEDED(UIntToShort(usHeight, &sRows));
    if (fSuccess && usWidth > 0 && usHeight > 0)
    {
        CONSOLE_SCREEN_BUFFER_INFOEX csbiex = { 0 };
        csbiex.cbSize = sizeof(CONSOLE_SCREEN_BUFFER_INFOEX);
        fSuccess = !!_pConApi->GetConsoleScreenBufferInfoEx(&csbiex);

        if (fSuccess)
        {
            csbiex.dwSize.X = sColumns;
            // Can't just set the dwSize.Y - that's the buffer's height, not
            //      the viewport's
            fSuccess = !!_pConApi->SetConsoleScreenBufferInfoEx(&csbiex);
            if (fSuccess)
            {
                SMALL_RECT srNewViewport = csbiex.srWindow;
                // SetConsoleWindowInfo expect inclusive rects
                srNewViewport.Right = srNewViewport.Left + sColumns - 1;
                srNewViewport.Bottom = srNewViewport.Top + sRows - 1;
                fSuccess = !!_pConApi->SetConsoleWindowInfo(true, &srNewViewport);
            }
        }   
    }
    return fSuccess;
}
