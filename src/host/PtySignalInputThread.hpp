/*++
Copyright (c) Microsoft Corporation

Module Name:
- PtySignalInputThread.hpp

Abstract:
- Defines methods that wrap the thread that will wait for Pty Signals
  if a Pty server (VT server) is running.

Author(s):
- Mike Griese (migrie) 15 Aug 2017
- Michael Niksa (miniksa) 19 Jan 2018
--*/
#pragma once


namespace Microsoft
{
    namespace Console
    {
        class PtySignalInputThread final
        {
        public:
            PtySignalInputThread(_In_ wil::unique_hfile hPipe);

            HRESULT Start();
            static DWORD StaticThreadProc(_In_ LPVOID lpParameter);

            // Prevent copying and assignment.
            PtySignalInputThread(const PtySignalInputThread&) = delete;
            PtySignalInputThread& operator=(const PtySignalInputThread&) = delete;

        private:
            DWORD _InputThread();

            wil::unique_hfile _hFile;
            wil::unique_handle _hThread;
            DWORD _dwThreadId;
        };
    }
};