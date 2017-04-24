/*++
Copyright (c) Microsoft Corporation

Module Name:
- IWindowMetrics.hpp

Abstract:
- Defines methods that return the maximum and minimum dimensions permissible for
  the console window.

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
            class IWindowMetrics
            {
            public:
                virtual RECT GetMinClientRectInPixels() = 0;
                virtual RECT GetMaxClientRectInPixels() = 0;

            protected:
                IWindowMetrics() { }

                IWindowMetrics(IWindowMetrics const&) = delete;
                IWindowMetrics& operator=(IWindowMetrics const&) = delete;
            };
        };
    };
};
