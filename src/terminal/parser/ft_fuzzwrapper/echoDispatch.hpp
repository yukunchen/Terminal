/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "..\termDispatch.hpp"

namespace Microsoft
{
    namespace Console
    {
        namespace VirtualTerminal
        {
            class EchoDispatch : public TermDispatch
            {
            public: 
                void Print(_In_ wchar_t const wchPrintable);
                void PrintString(_In_reads_(cch) wchar_t* const rgwch, _In_ size_t const cch);
                void Execute(_In_ wchar_t const wchControl);
            };
        };
    };
};
