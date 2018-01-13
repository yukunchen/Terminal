/*++
Copyright (c) Microsoft Corporation

Module Name:
- terminalInput.hpp

Abstract:
- This serves as an adapter between virtual key input from a user and the virtual terminal sequences that are
  typically emitted by an xterm-compatible console.

Author(s):
- Michael Niksa (MiNiksa) 30-Oct-2015
--*/

#include <functional>
#include "../../types/inc/IInputEvent.hpp"
#pragma once

namespace Microsoft
{
    namespace Console
    {
        namespace VirtualTerminal
        {
            class TerminalInput final
            {
            public:
                TerminalInput(_In_ std::function<void(std::deque<std::unique_ptr<IInputEvent>>&)> pfn);
                ~TerminalInput();

                bool HandleKey(_In_ const IInputEvent* const pInEvent) const;
                void ChangeKeypadMode(_In_ bool const fApplicationMode);
                void ChangeCursorKeysMode(_In_ bool const fApplicationMode);

            private:

                std::function<void(std::deque<std::unique_ptr<IInputEvent>>&)> _pfnWriteEvents;
                bool _fKeypadApplicationMode = false;
                bool _fCursorApplicationMode = false;

                void _SendNullInputSequence(_In_ DWORD const dwControlKeyState) const;
                void _SendInputSequence(_In_ PCWSTR const pwszSequence) const;
                void _SendEscapedInputSequence(_In_ const wchar_t wch) const;

                struct _TermKeyMap
                {
                    WORD const wVirtualKey;
                    PCWSTR const pwszSequence;
                    DWORD const dwModifiers;

                    static const size_t s_cchMaxSequenceLength;

                    _TermKeyMap(_In_ WORD const wVirtualKey, _In_ PCWSTR const pwszSequence) :
                        wVirtualKey(wVirtualKey),
                        pwszSequence(pwszSequence),
                        dwModifiers(0) {};

                    _TermKeyMap(_In_ WORD const wVirtualKey, _In_ const DWORD dwModifiers, _In_ PCWSTR const pwszSequence) :
                        wVirtualKey(wVirtualKey),
                        pwszSequence(pwszSequence),
                        dwModifiers(dwModifiers) {};

                    // C++11 syntax for prohibiting assignment
                    // We can't assign, everything here is const.
                    // We also shouldn't need to, this is only for a specific table.
                    _TermKeyMap& operator=(const _TermKeyMap&) = delete;
                };

                static const _TermKeyMap s_rgCursorKeysNormalMapping[];
                static const _TermKeyMap s_rgCursorKeysApplicationMapping[];
                static const _TermKeyMap s_rgKeypadNumericMapping[];
                static const _TermKeyMap s_rgKeypadApplicationMapping[];
                static const _TermKeyMap s_rgModifierKeyMapping[];
                static const _TermKeyMap s_rgSimpleModifedKeyMapping[];

                static const size_t s_cCursorKeysNormalMapping;
                static const size_t s_cCursorKeysApplicationMapping;
                static const size_t s_cKeypadNumericMapping;
                static const size_t s_cKeypadApplicationMapping;
                static const size_t s_cModifierKeyMapping;
                static const size_t s_cSimpleModifedKeyMapping;

                bool _SearchKeyMapping(_In_ const KeyEvent& keyEvent,
                                       _In_reads_(cKeyMapping) const TerminalInput::_TermKeyMap* keyMapping,
                                       _In_ size_t const cKeyMapping,
                                       _Out_ const TerminalInput::_TermKeyMap** pMatchingMapping) const;
                bool _TranslateDefaultMapping(_In_ const KeyEvent& keyEvent,
                                              _In_reads_(cKeyMapping) const TerminalInput::_TermKeyMap* keyMapping,
                                              _In_ size_t const cKeyMapping) const;
                bool _SearchWithModifier(_In_ const KeyEvent& keyEvent) const;


            public:
                const size_t GetKeyMappingLength(_In_ const KeyEvent& keyEvent) const;
                const _TermKeyMap* GetKeyMapping(_In_ const KeyEvent& keyEvent) const;

            };
        };
    };
};
