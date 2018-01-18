/*++
Copyright (c) Microsoft Corporation

Module Name:
- DispatchCommon.hpp

Abstract:
- Defines a number of common functions and enums whose implementation is the 
    same in both the AdaptDispatch and the InteractDispatch.

Author(s): 
- Mike Griese (migrie) 11 Oct 2017
--*/

#pragma once

#include "conGetSet.hpp"

namespace Microsoft
{
    namespace Console
    {
        namespace VirtualTerminal
        {
            class DispatchCommon;
        }
    }
}

class Microsoft::Console::VirtualTerminal::DispatchCommon final
{
public:
    
    enum WindowManipulationType : unsigned int
    {
        Invalid = 0,
        RefreshWindow = 7,
        ResizeWindowInCharacters = 8,
    };

    static bool s_ResizeWindow(_Inout_ ConGetSet* const pConApi,
                               _In_ const unsigned short usWidth,
                               _In_ const unsigned short usHeight);

    static bool s_RefreshWindow(_Inout_ ConGetSet* const pConApi);
    
    static bool s_SuppressResizeRepaint(_Inout_ ConGetSet* const pConApi);

};
