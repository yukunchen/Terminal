/*++
Copyright (c) Microsoft Corporation

Module Name:
- VtInputThread.hpp
--*/
#pragma once
// #include "precomp.h"

#include "..\interactivity\inc\IConsoleInputThread.hpp"
#include "..\terminal\parser\StateMachine.hpp"

namespace Microsoft
{
    namespace Console
    {
        class VtInputThread sealed : public IConsoleInputThread
        {
        public:
            VtInputThread(HANDLE hPipe);
            ~VtInputThread();

            HANDLE Start();
            static DWORD StaticVtInputThreadProc(LPVOID lpParameter);

        private:
            void _HandleRunInput(char* charBuffer, int cch);
            DWORD _InputThread();

            // wil::unique_hfile _hFile;
            HANDLE _hFile;
            StateMachine* _pInputStateMachine;

        };
    }
};