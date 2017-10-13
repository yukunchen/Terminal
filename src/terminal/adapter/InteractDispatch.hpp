/*++
Copyright (c) Microsoft Corporation

Module Name:
- adaptDispatch.hpp

Abstract:
- This serves as the Windows Console API-specific implementation of the 
    callbacks from our generic Virtual Terminal Input parser.

Author(s): 
- Mike Griese (migrie) 11 Oct 2017
--*/
#pragma once

#include "../parser/IInteractDispatch.hpp"
#include "conGetSet.hpp"

namespace Microsoft
{
    namespace Console
    {
        namespace VirtualTerminal
        {
            class InteractDispatch;
        }
    }
}

class Microsoft::Console::VirtualTerminal::InteractDispatch : public IInteractDispatch
{
public:

    static bool CreateInstance(_In_ std::unique_ptr<ConGetSet> pConApi,
                               _Outptr_ InteractDispatch ** const ppDispatch);

    virtual bool WriteInput(_In_ std::deque<std::unique_ptr<IInputEvent>>& inputEvents) override;
    virtual bool WindowManipulation(_In_ const WindowManipulationFunction uiFunction,
                                _In_reads_(cParams) const unsigned short* const rgusParams,
                                _In_ size_t const cParams) override; // DTTERM_WindowManipulation
private:

    InteractDispatch(_In_ std::unique_ptr<ConGetSet> const pConApi);
    
    std::unique_ptr<ConGetSet> _pConApi;
    
    bool _ResizeWindow(_In_ const unsigned short usWidth, _In_ const unsigned short usHeight);
};
