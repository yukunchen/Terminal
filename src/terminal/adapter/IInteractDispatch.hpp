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

#include "DispatchCommon.hpp"
#include "../../types/inc/IInputEvent.hpp"

namespace Microsoft
{
    namespace Console
    {
        namespace VirtualTerminal
        {
            class IInteractDispatch;
        }
    }
}

class Microsoft::Console::VirtualTerminal::IInteractDispatch
{
public:
    virtual bool WriteInput(_In_ std::deque<std::unique_ptr<IInputEvent>>& /*inputEvents*/) {return false;}

    virtual bool WindowManipulation(_In_ const DispatchCommon::WindowManipulationType /*uiFunction*/,
                                    _In_reads_(cParams) const unsigned short* const /*rgusParams*/,
                                    _In_ size_t const /*cParams*/) { return false; } 

};
