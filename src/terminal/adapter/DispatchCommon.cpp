/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "DispatchCommon.hpp"
#include "../../types/inc/Viewport.hpp"

using namespace Microsoft::Console::Types;
using namespace Microsoft::Console::VirtualTerminal;

// Method Description:
// - Resizes the window to the specified dimensions, in characters.
// Arguments:
// - pConApi: The ConGetSet implementation to call back into.
// - usWidth: The new width of the window, in columns
// - usHeight: The new height of the window, in rows
// Return Value:
// True if handled successfully. False othewise.
bool DispatchCommon::s_ResizeWindow(_Inout_ ConGetSet* const pConApi,
                                    _In_ const unsigned short usWidth,
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
        fSuccess = !!pConApi->GetConsoleScreenBufferInfoEx(&csbiex);

        if (fSuccess)
        {
            const Viewport oldViewport = Viewport::FromInclusive(csbiex.srWindow);
            csbiex.dwSize.X = sColumns;
            // Can't just set the dwSize.Y - that's the buffer's height, not
            //      the viewport's
            fSuccess = !!pConApi->SetConsoleScreenBufferInfoEx(&csbiex);
            if (fSuccess)
            {
                // SetConsoleWindowInfo expect inclusive rects
                SMALL_RECT sr = Viewport::FromDimensions(oldViewport.Origin(),
                                                         sColumns,
                                                         sRows).ToInclusive();
                fSuccess = !!pConApi->SetConsoleWindowInfo(true, &sr);
            }
        }   
    }
    return fSuccess;
}
