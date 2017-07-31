/*++
Copyright (c) Microsoft Corporation

Module Name:
- VtInputThread.hpp
--*/

#include "precomp.h"

#include "..\inc\IConsoleInputThread.hpp"

namespace Microsoft
{
    namespace Console
    {
        namespace Interactivity
        {
            namespace Win32
            {
                class VtInputThread sealed : public IConsoleInputThread
                {
                public:
                    HANDLE Start();
                private:
                    wil::unique_hfile _hFile;
                };
            }
        };
    };
};