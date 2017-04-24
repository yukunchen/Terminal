/*++
Copyright (c) Microsoft Corporation

Module Name:
- SystemConfigurationProvider.hpp

Abstract:
- OneCore implementation of the ISystemConfigurationProvider interface.

Author(s):
- Hernan Gatta (HeGatta) 29-Mar-2017
--*/

#pragma once

#include "..\inc\ISystemConfigurationProvider.hpp"

#pragma hdrstop

namespace Microsoft
{
    namespace Console
    {
        namespace Interactivity
        {
            namespace OneCore
            {
                class SystemConfigurationProvider sealed : public ISystemConfigurationProvider
                {
                public:
                    bool IsCaretBlinkingEnabled();

                    UINT GetCaretBlinkTime();
                    int GetNumberOfMouseButtons();
                    ULONG GetNumberOfWheelScrollLines();
                    ULONG GetNumberOfWheelScrollCharacters();

                    void GetSettingsFromLink(_Inout_ Settings* pLinkSettings,
                                            _Inout_updates_bytes_(*pdwTitleLength) LPWSTR pwszTitle,
                                            _Inout_ PDWORD pdwTitleLength,
                                            _In_ PCWSTR pwszCurrDir,
                                            _In_ PCWSTR pwszAppName);
                };
            };
        };
    };
};