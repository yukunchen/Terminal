/*++
Copyright (c) Microsoft Corporation

Module Name:
- InputServices.hpp

Abstract:
- Win32 implementation of the IInputServices interface.

Author(s):
- Hernan Gatta (HeGatta) 29-Mar-2017
--*/

#include "..\inc\IInputServices.hpp"

namespace Microsoft
{
    namespace Console
    {
        namespace Interactivity
        {
            namespace Win32
            {
                class InputServices final : public IInputServices
                {
                    // Inherited via IInputServices
                    ~InputServices() = default;
                    UINT MapVirtualKeyW(UINT uCode, UINT uMapType);
                    SHORT VkKeyScanW(WCHAR ch);
                    SHORT GetKeyState(int nVirtKey);
                    BOOL TranslateCharsetInfo(DWORD * lpSrc, LPCHARSETINFO lpCs, DWORD dwFlags);
                };
            }
        }
    }
}
