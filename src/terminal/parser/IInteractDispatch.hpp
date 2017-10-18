/*++
Copyright (c) Microsoft Corporation

Module Name:
- InteractDispatch.hpp

Abstract:
- Base class for Input State Machine callbacks. When actions occur, they will 
    be dispatched to the methods on this interface which must be implemented by
    a child class and passed into the state machine on creation.

Author(s): 
- Mike Griese (migrie) 11 Oct 2017
--*/
#pragma once

#include "../../types/inc/IInputEvent.hpp"

namespace Microsoft
{
    namespace Console
    {
        namespace VirtualTerminal
        {
            class InteractDispatch;
            enum WindowManipulationType;
        }
    }
}
using namespace Microsoft::Console::VirtualTerminal;

class IInteractDispatch
{
public:
    virtual bool WriteInput(_In_ std::deque<std::unique_ptr<IInputEvent>>& /*inputEvents*/) {return false;}
    
    // This is kept seperate from the TermDispatch version, as there may be
    //  codes that are supported in one direction but not the other.
    enum WindowManipulationType : unsigned int
    {
        Invalid = 0,
        ResizeWindowInCharacters = 8,
    };

    virtual bool WindowManipulation(_In_ const WindowManipulationType /*uiFunction*/,
                                    _In_reads_(cParams) const unsigned short* const /*rgusParams*/,
                                    _In_ size_t const /*cParams*/) { return false; } 

};
