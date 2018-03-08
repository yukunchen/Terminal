/*++
Copyright (c) Microsoft Corporation

Module Name:
- VtInputThread.hpp

Abstract:
- Defines methods that wrap the thread that reads VT input from a pipe and
  feeds it into the console's input buffer.

Author(s):
- Mike Griese (migrie) 15 Aug 2017
--*/
#pragma once

#include "..\terminal\parser\StateMachine.hpp"
#include "utf8ToWideCharParser.hpp"

namespace Microsoft::Console
{
    class VtInputThread
    {
    public:
        VtInputThread(_In_ wil::unique_hfile hPipe, _In_ const bool inheritCursor);

        HRESULT Start();
        static DWORD StaticVtInputThreadProc(_In_ LPVOID lpParameter);
        void DoReadInput(_In_ const bool throwOnFail);

    private:
        HRESULT _HandleRunInput(_In_reads_(cch) const byte* const charBuffer, _In_ const int cch);
        DWORD _InputThread();

        wil::unique_hfile _hFile;
        wil::unique_handle _hThread;
        DWORD _dwThreadId;

        std::unique_ptr<StateMachine> _pInputStateMachine;
        Utf8ToWideCharParser _utf8Parser;
    };
}
