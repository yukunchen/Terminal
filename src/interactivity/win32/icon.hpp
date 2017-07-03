/*++
Copyright (c) Microsoft Corporation

Module Name:
- icon.hpp

Abstract:
- This module is used for managing icons

Author(s):
- Michael Niksa (miniksa) 14-Oct-2014
- Paul Campbell (paulcam) 14-Oct-2014
--*/

#pragma once

namespace Microsoft
{
    namespace Console
    {
        namespace Interactivity
        {
            namespace Win32
            {
                class Icon sealed
                {
                public:
                    static Icon& Instance();

                    HRESULT GetIcons(_Out_opt_ HICON* const phIcon, _Out_opt_ HICON* const phSmIcon);
                    HRESULT SetIcons(_In_ HICON const hIcon, _In_ HICON const hSmIcon);

                    HRESULT LoadIconsFromPath(_In_ PCWSTR pwszIconLocation, _In_ int const nIconIndex);

                    HRESULT ApplyWindowMessageWorkaround(_In_ HWND const hwnd);

                protected:
                    Icon();
                    ~Icon();
                    Icon(Icon const&);
                    void operator=(Icon const&);

                private:
                    HRESULT _Initialize();

                    void _DestroyNonDefaultIcons();

                    // Helper methods
                    HRESULT _GetAvailableIconFromReference(_In_ HICON& hIconRef, _In_ HICON& hDefaultIconRef, _Out_ HICON* const phIcon);
                    HRESULT _GetDefaultIconFromReference(_In_ HICON& hIconRef, _Out_ HICON* const phIcon);
                    HRESULT _SetIconFromReference(_In_ HICON& hIconRef, _In_ HICON const hNewIcon);
                    void _FreeIconFromReference(_In_ HICON& hIconRef);

                    bool _fInitialized;
                    HICON _hDefaultIcon;
                    HICON _hDefaultSmIcon;
                    HICON _hIcon;
                    HICON _hSmIcon;
                };
            };
        };
    };
};
