/*++
Copyright (c) Microsoft Corporation

Module Name:
- InteractDispatch.hpp

Abstract:
- Base class for Input State Machine callbacks. When actions occur, they will 
    be dispatched to the methods on this interface which must be implemented by
    a child class and passed into the state machine on creation.

Author(s): 
- Mike Griese (migrie) 10 Oct 2017
--*/
#include <functional>
#include "../../types/inc/IInputEvent.hpp"

namespace Microsoft
{
    namespace Console
    {
        namespace VirtualTerminal
        {
            class InteractDispatch;
        }
    }
};


class Microsoft::Console::VirtualTerminal::IInteractDispatch
{
public:
    virtual bool WriteInput(_In_ std::deque<std::unique_ptr<IInputEvent>>& /*inputEvents*/) {return false;}
    virtual bool ResizeWindow(_In_ const unsigned short /*usWidth*/,
                              _In_ const unsigned short /*usHeight*/) {return false;}
}
