/*++
Copyright (c) Microsoft Corporation

Module Name:
- IConsoleControl.hpp

Abstract:
- Defines methods that delegate the execution of privileged operations or notify
  Windows subsystems about console state.

Author(s):
- Hernan Gatta (HeGatta) 29-Mar-2017
--*/

#pragma once

namespace Microsoft
{
    namespace Console
    {
        namespace Interactivity
        {
            class IConsoleControl
            {
            public:
                virtual ~IConsoleControl() = 0;
                virtual NTSTATUS NotifyConsoleApplication(DWORD dwProcessId) = 0;
                virtual NTSTATUS SetForeground(HANDLE hProcess, BOOL fForeground) = 0;
                virtual NTSTATUS EndTask(HANDLE hProcessId, DWORD dwEventType, ULONG ulCtrlFlags) = 0;

            protected:
                IConsoleControl() { }

                IConsoleControl(IConsoleControl const&) = delete;
                IConsoleControl& operator=(IConsoleControl const &) = delete;
            };

            inline IConsoleControl::~IConsoleControl() {}
        }
    }
}
