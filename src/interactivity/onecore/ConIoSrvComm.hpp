/*++
Copyright (c) Microsoft Corporation

Module Name:
- ConIoSrvComm.hpp

Abstract:
- OneCore implementation of the IConIoSrvComm interface.

Author(s):
- Hernan Gatta (HeGatta) 29-Mar-2017
--*/

#pragma once

#include <condrv.h>

#include "ConIoSrv.h"
#include "..\..\inc\IInputServices.hpp"

#include "ConIoSrvComm.hpp"

#pragma hdrstop

namespace Microsoft
{
    namespace Console
    {
        namespace Interactivity
        {
            namespace OneCore
            {
                class ConIoSrvComm final : public IInputServices
                {
                public:
                    ConIoSrvComm();
                    ~ConIoSrvComm() override;

                    NTSTATUS Connect();
                    VOID ServiceInputPipe();

                    NTSTATUS RequestGetDisplaySize(_Inout_ PCD_IO_DISPLAY_SIZE pCdDisplaySize) const;
                    NTSTATUS RequestGetFontSize(_Inout_ PCD_IO_FONT_SIZE pCdFontSize) const;
                    NTSTATUS RequestSetCursor(_In_ CD_IO_CURSOR_INFORMATION* const pCdCursorInformation) const;
                    NTSTATUS RequestUpdateDisplay(_In_ SHORT RowIndex) const;

                    NTSTATUS RequestMapVirtualKey(_In_ UINT uCode, _In_ UINT uMapType, _Out_ UINT* puReturnValue);
                    NTSTATUS RequestVkKeyScan(_In_ WCHAR wCharacter, _Out_ SHORT* psReturnValue);
                    NTSTATUS RequestGetKeyState(_In_ int iVirtualKey, _Out_ SHORT *psReturnValue);
                    
                    NTSTATUS RequestGetDisplayMode(_Out_ USHORT *psDisplayMode);

                    PVOID GetSharedViewBase() const;
                    
                    VOID CleanupForHeadless(_In_ NTSTATUS const status);

                    // IInputServices Members
                    UINT MapVirtualKeyW(UINT uCode, UINT uMapType);
                    SHORT VkKeyScanW(WCHAR ch);
                    SHORT GetKeyState(int nVirtKey);
                    BOOL TranslateCharsetInfo(DWORD * lpSrc, LPCHARSETINFO lpCs, DWORD dwFlags);
                
                private:
                    NTSTATUS EnsureConnection();
                    NTSTATUS SendRequestReceiveReply(PCIS_MSG Message) const;

                    VOID HandleFocusEvent(PCIS_EVENT const FocusEvent);

                    HANDLE _pipeReadHandle;
                    HANDLE _pipeWriteHandle;

                    HANDLE _alpcClientCommunicationPort;
                    SIZE_T _alpcSharedViewSize;
                    PVOID _alpcSharedViewBase;
                    
                    USHORT _displayMode;

                    bool _fIsInputInitialized;
                };
            };
        };
    };
};
