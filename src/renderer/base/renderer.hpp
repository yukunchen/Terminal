/*++
Copyright (c) Microsoft Corporation

Module Name:
- Renderer.hpp

Abstract:
- This is the definition of our renderer.
- It provides interfaces for the console application to notify when various portions of the console state have changed and need to be redrawn.
- It requires a data interface to fetch relevant console structures required for drawing and a drawing engine target for output.

Author(s):
- Michael Niksa (MiNiksa) 17-Nov-2015
--*/

#pragma once

#include "..\inc\IRenderer.hpp"
#include "..\inc\IRenderEngine.hpp"
#include "..\inc\IRenderData.hpp"

#include "thread.hpp"

#include "..\..\host\textBuffer.hpp"

namespace Microsoft
{
    namespace Console
    {
        namespace Render
        {
            class Renderer sealed : public IRenderer
            {
            public:
                static HRESULT s_CreateInstance(_In_ IRenderData* const pData, _In_ IRenderEngine* const pEngine, _Outptr_result_nullonfailure_ Renderer** const ppRenderer);
                ~Renderer();

                void PaintFrame();

                void TriggerSystemRedraw(_In_ const RECT* const prcDirtyClient);
                void TriggerRedraw(_In_ const SMALL_RECT* const psrRegion);
                void TriggerRedraw(_In_ const COORD* const pcoord);
                void TriggerRedrawAll();

                void TriggerSelection();
                void TriggerScroll();
                void TriggerScroll(_In_ const COORD* const pcoordDelta);

                void TriggerFontChange(_In_ int const iDpi, _Inout_ FontInfo* const pFontInfo);

                void GetProposedFont(_In_ int const iDpi, _Inout_ FontInfo* const pFontInfo);

                COORD GetFontSize();
                bool IsCharFullWidthByFont(_In_ WCHAR const wch);

            private:
                Renderer(_In_ IRenderData* const pData, _In_ IRenderEngine* const pEngine);
                IRenderEngine* _pEngine;
                IRenderData* _pData;

                RenderThread* _pThread;
                void _NotifyPaintFrame();

                bool _CheckViewportAndScroll();
                
                void _PaintBackground();
                
                void _PaintBufferOutput();
                void _PaintBufferOutputRasterFontHelper(_In_ const ROW* const pRow, _In_reads_(cchLine) PCWCHAR const pwsLine, _In_reads_(cchLine) PBYTE pbKAttrsLine, _In_ size_t cchLine, _In_ size_t iFirstAttr, _In_ COORD const coordTarget);
                void _PaintBufferOutputColorHelper(_In_ const ROW* const pRow, _In_reads_(cchLine) PCWCHAR const pwsLine, _In_reads_(cchLine) PBYTE pbKAttrsLine, _In_ size_t cchLine, _In_ size_t iFirstAttr, _In_ COORD const coordTarget);
                void _PaintBufferOutputDoubleByteHelper(_In_reads_(cchLine) PCWCHAR const pwsLine, _In_reads_(cchLine) PBYTE const pbKAttrsLine, _In_ size_t const cchLine, _In_ COORD const coordTarget);
                void _PaintBufferOutputGridLineHelper(_In_ const TextAttribute* const pAttr, _In_ size_t const cchLine, _In_ COORD const coordTarget);

                void _PaintSelection();
                void _PaintCursor();

                void _PaintIme(_In_ const ConversionAreaInfo* const pAreaInfo, _In_ const TEXT_BUFFER_INFO* const pTextInfo);
                void _PaintImeCompositionString();

                void _UpdateDrawingBrushes(_In_ const TextAttribute* const pAttr, _In_ bool const fIncludeBackground);
                void _ClearOverlays();
                void _PerformScrolling();

                SMALL_RECT _srViewportPrevious;

                _Check_return_
                    NTSTATUS _GetSelectionRects(
                        _Outptr_result_buffer_all_(*pcRectangles) SMALL_RECT** const prgsrSelection,
                        _Out_ UINT* const pcRectangles) const;

                SMALL_RECT _RegionFromCoord(_In_ const COORD* const pcoord) const;
                COLORREF _ConvertAttrToRGB(_In_ const BYTE bAttr);
            };
        };
    };
};