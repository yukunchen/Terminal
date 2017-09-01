/*++
Copyright (c) Microsoft Corporation

Module Name:
- IStateMachineEngine.hpp

Abstract:
- This is the interface for a VT state machine language
    The terminal handles input sequences and output sequences differently, 
    almost as two seperate grammars. This enables different grammars to leverage 
    the existing VT parsing.

Author(s): 
- Mike Griese (migrie) 18 Aug 2017
--*/
#pragma once
namespace Microsoft
{
    namespace Console
    {
        namespace VirtualTerminal
        {
            class IStateMachineEngine
            {
            public:

                virtual bool ActionExecute(_In_ wchar_t const wch) = 0;
                virtual bool ActionPrint(_In_ wchar_t const wch) = 0;
                virtual bool ActionPrintString(_In_reads_(cch) wchar_t* const rgwch, _In_ size_t const cch) = 0;
                virtual bool ActionEscDispatch(_In_ wchar_t const wch, _In_ const unsigned short cIntermediate, _In_ const wchar_t wchIntermediate) = 0;
                virtual bool ActionCsiDispatch(_In_ wchar_t const wch, 
                                               _In_ const unsigned short cIntermediate,
                                               _In_ const wchar_t wchIntermediate,
                                               _In_ const unsigned short* const rgusParams,
                                               _In_ const unsigned short cParams) = 0;
                virtual bool ActionClear() = 0;
                virtual bool ActionIgnore() = 0;
                virtual bool ActionOscDispatch(_In_ wchar_t const wch, _In_ const unsigned short sOscParam, _In_ wchar_t* const pwchOscStringBuffer, _In_ const unsigned short cchOscString) = 0;
                // virtual bool ActionSs3Dispatch(_In_ wchar_t const wch) = 0;
                virtual bool FlushAtEndOfString() = 0;

            };
        }
    }
}
