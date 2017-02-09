/*++
Copyright (c) Microsoft Corporation

Module Name:
- IRenderer.hpp

Abstract:
- This serves as the entry point for console rendering activites.

Author(s):
- Michael Niksa (MiNiksa) 17-Nov-2015
--*/

#pragma once

#include "FontInfo.hpp"

namespace Microsoft
{
    namespace Console
    {
        namespace Render
        {
            class IRenderer
            {
            public:
                virtual ~IRenderer() {};

                virtual HRESULT PaintFrame() = 0;

                virtual void TriggerSystemRedraw(_In_ const RECT* const prcDirtyClient) = 0;

                virtual void TriggerRedraw(_In_ const SMALL_RECT* const psrRegion) = 0;
                virtual void TriggerRedraw(_In_ const COORD* const pcoord) = 0;

                virtual void TriggerRedrawAll() = 0;

                virtual void TriggerSelection() = 0;
                virtual void TriggerScroll() = 0;
                virtual void TriggerScroll(_In_ const COORD* const pcoordDelta) = 0;

                virtual void TriggerFontChange(_In_ int const iDpi, _In_ FontInfoDesired const * const pFontInfoDesired, _Out_ FontInfo* const pFontInfo) = 0;

                virtual HRESULT GetProposedFont(_In_ int const iDpi,  _In_ FontInfoDesired const * const pFontInfoDesired, _Out_ FontInfo* const pFontInfo) = 0;

                virtual COORD GetFontSize() = 0;
                virtual bool IsCharFullWidthByFont(_In_ WCHAR const wch) = 0;
            };
        };
    };
};
