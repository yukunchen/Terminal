/*++
Copyright (c) Microsoft Corporation

Module Name:
- WindowMetrics.hpp

Abstract:
- Win32 implementation of the IWindowMetrics interface.

Author(s):
- Hernan Gatta (HeGatta) 29-Mar-2017
--*/

#include "..\inc\IWindowMetrics.hpp"

namespace Microsoft
{
    namespace Console
    {
        namespace Interactivity
        {
            namespace Win32
            {
                class WindowMetrics : public IWindowMetrics
                {
                public:
                    // IWindowMetrics Members
                    RECT GetMinClientRectInPixels();
                    RECT GetMaxClientRectInPixels();

                    // Public Members
                    RECT GetMaxWindowRectInPixels();
                    RECT GetMaxWindowRectInPixels(_In_ const RECT * const prcSuggested, _Out_opt_ UINT * pDpiSuggested);

                    BOOL AdjustWindowRectEx(_Inout_ LPRECT prc, _In_ const DWORD dwStyle, _In_ const BOOL fMenu, _In_ const DWORD dwExStyle);
                    BOOL AdjustWindowRectEx(_Inout_ LPRECT prc, _In_ const DWORD dwStyle, _In_ const BOOL fMenu, _In_ const DWORD dwExStyle, _In_ const int iDpi);

                    void ConvertClientRectToWindowRect(_Inout_ RECT * const prc);
                    void ConvertWindowRectToClientRect(_Inout_ RECT * const prc);

                private:
                    enum ConvertRectangle
                    {
                        CLIENT_TO_WINDOW,
                        WINDOW_TO_CLIENT
                    };

                    BOOL UnadjustWindowRectEx(_Inout_ LPRECT prc, _In_ const DWORD dwStyle, _In_ const BOOL fMenu, _In_ const DWORD dwExStyle);

                    void ConvertRect(_Inout_ RECT* const prc, _In_ ConvertRectangle const crDirection);
                };
            };
        };
    };
};