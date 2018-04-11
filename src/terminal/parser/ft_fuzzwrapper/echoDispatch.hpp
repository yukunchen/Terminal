/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "../../adapter/termDispatch.hpp"

namespace Microsoft
{
    namespace Console
    {
        namespace VirtualTerminal
        {
            class EchoDispatch : public TermDispatch
            {
            public: 
                void Print(const wchar_t wchPrintable);
                void PrintString(_In_reads_(cch) wchar_t* const rgwch, const size_t cch);
                void Execute(const wchar_t wchControl);
            };
        };
    };
};
