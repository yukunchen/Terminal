/*++
Copyright (c) Microsoft Corporation

Module Name:
- terminalOutput.hpp

Abstract:
- 

Author(s):
- Mike Griese (migrie) 03-Mar-2016
--*/
#pragma once

#include "..\parser\termDispatch.hpp"

namespace Microsoft
{
    namespace Console
    {
        namespace VirtualTerminal
        {
            class TerminalOutput sealed
            {
            public:

                TerminalOutput();
                ~TerminalOutput();

                wchar_t TranslateKey(_In_ const wchar_t wch) const;
                bool DesignateCharset(_In_ const wchar_t wchNewCharset);
                bool NeedToTranslate() const;

            private:
                wchar_t _wchCurrentCharset = TermDispatch::VTCharacterSets::USASCII;

                // The tables only ever change the values x20 - x7f (96 display characters)
                static const unsigned int s_uiNumDisplayCharacters = 96;
                static const wchar_t s_rgDECSpecialGraphicsTranslations[s_uiNumDisplayCharacters];
            
                const wchar_t* _GetTranslationTable() const;
                
            };
        };
    };
};