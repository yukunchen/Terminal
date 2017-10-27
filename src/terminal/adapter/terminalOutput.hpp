/*++
Copyright (c) Microsoft Corporation

Module Name:
- terminalOutput.hpp

Abstract:
- Provides a set of functions for translating certain characters into other 
    characters. There are special VT modes where the display characters (values 
    x20 - x7f) should be displayed as other characters. This module provides an
    componentization of that logic.

Author(s):
- Mike Griese (migrie) 03-Mar-2016
--*/
#pragma once

#include "termDispatch.hpp"

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
