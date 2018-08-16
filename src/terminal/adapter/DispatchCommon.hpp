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

namespace Microsoft::Console::VirtualTerminal
{
    class DispatchCommon final
    {
    public:

        enum WindowManipulationType : unsigned int
        {
            Invalid = 0,
            RefreshWindow = 7,
            ResizeWindowInCharacters = 8,
        };

        enum class CursorStyle : unsigned int
        {
            BlinkingBlock = 0,
            BlinkingBlockDefault = 1,
            SteadyBlock = 2,
            BlinkingUnderline = 3,
            SteadyUnderline = 4,
            BlinkingBar = 5,
            SteadyBar = 6
        };

        static bool s_ResizeWindow(ConGetSet& conApi,
                                   const unsigned short usWidth,
                                   const unsigned short usHeight);

        static bool s_RefreshWindow(ConGetSet& conApi);

        static bool s_SuppressResizeRepaint(ConGetSet& conApi);

    };
}
