/*++
Copyright (c) Microsoft Corporation

Module Name:
- VtInputThread.hpp
--*/

#include "precomp.h"

#include "..\inc\IConsoleInputThread.hpp"
#include "..\..\terminal\parser\StateMachine.hpp"

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
                    VtInputThread();
                    ~VtInputThread();

                    HANDLE Start();
                    static DWORD StaticVtInputThreadProc(LPVOID lpParameter);

                private:
                    void _HandleRunInput(char* charBuffer, int cch);
                    DWORD _InputThread();

                    wil::unique_hfile _hFile;
                    StateMachine* _pInputStateMachine;

                };
            }
        };
    };
};