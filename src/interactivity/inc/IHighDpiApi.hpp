/*++
Copyright (c) Microsoft Corporation

Module Name:
- IHighDpiApi.hpp

Abstract:
- Defines the methods used by the console to support high DPI displays.

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
            class IHighDpiApi
            {
            public:
                virtual BOOL SetProcessDpiAwarenessContext() = 0;
                virtual HRESULT SetProcessPerMonitorDpiAwareness() = 0;
                virtual BOOL EnablePerMonitorDialogScaling() = 0;

            protected:
                IHighDpiApi() { }

                IHighDpiApi(IHighDpiApi const&) = delete;
                IHighDpiApi& operator=(IHighDpiApi const&) = delete;
            };
        };
    };
};
