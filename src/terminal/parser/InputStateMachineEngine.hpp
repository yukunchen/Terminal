/*++
Copyright (c) Microsoft Corporation

Module Name:
- InputStateMachineEngine.hpp

Abstract:
- This is the implementation of the client VT input state machine engine.
    This generates InpueEvents from a stream of VT sequences emmited by a 
    client "terminal" application.

Author(s): 
- Mike Griese (migrie) 18 Aug 2017
--*/
#pragma once

#include "telemetry.hpp"
#include "IStateMachineEngine.hpp"

namespace Microsoft
{
    namespace Console
    {
        namespace VirtualTerminal
        {
            typedef void(*WriteInputEvents)(_In_reads_(cInput) INPUT_RECORD* rgInput, _In_ DWORD cInput);

            class InputStateMachineEngine : public IStateMachineEngine
            {
            public:
                InputStateMachineEngine(_In_ WriteInputEvents const pfnWriteEvents);
                ~InputStateMachineEngine();

                bool ActionExecute(_In_ wchar_t const wch);
                bool ActionPrint(_In_ wchar_t const wch);
                bool ActionPrintString(_In_reads_(cch) wchar_t* const rgwch, _In_ size_t const cch);
                bool ActionEscDispatch(_In_ wchar_t const wch, _In_ const unsigned short cIntermediate, _In_ const wchar_t wchIntermediate);
                bool ActionCsiDispatch(_In_ wchar_t const wch, 
                                       _In_ const unsigned short cIntermediate,
                                       _In_ const wchar_t wchIntermediate,
                                       _In_ const unsigned short* const rgusParams,
                                       _In_ const unsigned short cParams);
                bool ActionClear();
                bool ActionIgnore();
                bool ActionOscDispatch(_In_ wchar_t const wch, _In_ const unsigned short sOscParam, _In_ wchar_t* const pwchOscStringBuffer, _In_ const unsigned short cchOscString);
                
                // TODO: MSFT:13420038
                // bool ActionSs3Dispatch(_In_ wchar_t const wch);
                
                bool FlushAtEndOfString() const;

            private:

                // Microsoft::Console::VirtualTerminal::ParserTracing _trace;
                WriteInputEvents _pfnWriteEvents;

                enum CsiActionCodes : wchar_t
                {
                    ArrowUp = L'A',
                    ArrowDown = L'B',
                    ArrowRight = L'C',
                    ArrowLeft = L'D',
                    Home = L'H',
                    End = L'F',
                    Generic = L'~', // Used for a whole bunch of possible keys
                    F1 = L'P',
                    F2 = L'Q',
                    F3 = L'R',
                    F4 = L'S',
                };

                // enum Ss3ActionCodes : wchar_t
                // {
                //     // The "Cursor Keys" are sometimes sent as a Ss3 in "application mode"
                //     //  But for now we'll only accept them as Normal Mode sequences, as CSI's.
                //     // ArrowUp = L'A',
                //     // ArrowDown = L'B',
                //     // ArrowRight = L'C',
                //     // ArrowLeft = L'D',
                //     // Home = L'H',
                //     // End = L'F',
                //     F1 = L'P',
                //     F2 = L'Q',
                //     F3 = L'R',
                //     F4 = L'S',
                // };

                // Sequences ending in '~' use these numbers as identifiers.
                enum GenericKeyIdentifiers : unsigned short
                {
                    Insert = 2,
                    Delete = 3,
                    Prior = 5, //PgUp
                    Next = 6, //PgDn
                    F5 = 15,
                    F6 = 17,
                    F7 = 18,
                    F8 = 19,
                    F9 = 20,
                    F10 = 21,
                    F11 = 23,
                    F12 = 24,
                };

                typedef struct _CSI_TO_VKEY {
                    CsiActionCodes Action;
                    short vkey;
                } CSI_TO_VKEY;

                typedef struct _GENERIC_TO_VKEY {
                    GenericKeyIdentifiers Identifier;
                    short vkey;
                } GENERIC_TO_VKEY;

                static const CSI_TO_VKEY s_rgCsiMap[];
                static const GENERIC_TO_VKEY s_rgGenericMap[];


                DWORD _GetCursorKeysModifierState(_In_ const unsigned short* const rgusParams, _In_ const unsigned short cParams);
                DWORD _GetGenericKeysModifierState(_In_ const unsigned short* const rgusParams, _In_ const unsigned short cParams);
                void _GenerateKeyFromChar(_In_ const wchar_t wch, _Out_ short* const pVkey, _Out_ DWORD* const pdwModifierState);

                bool _IsModified(_In_ const unsigned short cParams);
                DWORD _GetModifier(_In_ const unsigned short modifierParam);

                bool _GetGenericVkey(_In_ const unsigned short* const rgusParams, _In_ const unsigned short cParams, _Out_ short* const pVkey) const;
                bool _GetCursorKeysVkey(_In_ const wchar_t wch, _Out_ short* const pVkey) const;

                bool _SendCursorKey(wchar_t wch, DWORD dwModifier);
                bool _SendGenericKey(unsigned short identifier, DWORD dwModifier);

                bool _WriteSingleKey(short vkey, DWORD dwModifierState);
                bool _WriteSingleKey(wchar_t wch, short vkey, DWORD dwModifierState);
                size_t _GenerateWrappedSequence(const wchar_t wch, const short vkey, const DWORD dwModifierState, INPUT_RECORD* rgInput, size_t cInput);
                size_t _GetSingleKeypress(wchar_t wch, short vkey, DWORD dwModifierState, _Inout_ INPUT_RECORD* const rgInput, _In_ size_t cRecords);

            };
        }
    }
}
