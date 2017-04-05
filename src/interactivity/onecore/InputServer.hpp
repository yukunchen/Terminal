/*++
Copyright (c) Microsoft Corporation

Module Name:
- InputServer.hpp

Abstract:
- OneCore implementation of the IInputServer interface.

Author(s):
- Hernan Gatta (HeGatta) 29-Mar-2017
--*/

#pragma once

#include <condrv.h>

#include "..\..\ConIoSrv\ConIoSrv.h"
#include "..\..\inc\IInputServices.hpp"

#include "InputServer.hpp"

#pragma hdrstop

namespace Microsoft
{
    namespace Console
    {
        namespace Interactivity
        {
            namespace OneCore
            {
                class InputServer sealed : public IInputServices
                {
                public:
                    NTSTATUS Connect();
                    NTSTATUS ServiceInputPipe();

                    NTSTATUS RequestGetDisplaySize(_Inout_ PCD_IO_DISPLAY_SIZE pCdDisplaySize);
                    NTSTATUS RequestGetFontSize(_Inout_ PCD_IO_FONT_SIZE pCdFontSize);
                    NTSTATUS RequestSetCursor(_In_ CD_IO_CURSOR_INFORMATION* const pCdCursorInformation);
                    NTSTATUS RequestUpdateDisplay(_In_ SHORT RowIndex);

                    NTSTATUS RequestMapVirtualKey(_In_ UINT uCode, _In_ UINT uMapType, _Out_ UINT* puReturnValue);
                    NTSTATUS RequestVkKeyScan(_In_ WCHAR wCharacter, _Out_ SHORT* psReturnValue);
                    NTSTATUS RequestGetKeyState(_In_ int iVirtualKey, _Out_ SHORT *psReturnValue);

                    PVOID GetSharedViewBase();

                    // IInputServices Members
                    UINT MapVirtualKeyW(UINT uCode, UINT uMapType);
                    SHORT VkKeyScanW(WCHAR ch);
                    SHORT GetKeyState(int nVirtKey);
                    BOOL TranslateCharsetInfo(DWORD * lpSrc, LPCHARSETINFO lpCs, DWORD dwFlags);
                
                private:
                    NTSTATUS SendRequestReceiveReply(PCIS_MSG Message);

                    HANDLE _pipeReadHandle;

                    HANDLE _alpcClientCommunicationPort;
                    SIZE_T _alpcSharedViewSize;
                    PVOID _alpcSharedViewBase;
                };
            };
        };
    };
};
