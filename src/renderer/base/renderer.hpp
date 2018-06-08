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

#include "../inc/IRenderer.hpp"
#include "../inc/IRenderEngine.hpp"
#include "../inc/IRenderData.hpp"

#include "thread.hpp"

#include "../../buffer/out/textBuffer.hpp"
#include "../../buffer/out/CharRow.hpp"

namespace Microsoft::Console::Render
{
    class Renderer sealed : public IRenderer
    {
    public:
        [[nodiscard]]
        static HRESULT s_CreateInstance(_In_ std::unique_ptr<IRenderData> pData,
                                        _In_reads_(cEngines) IRenderEngine** const rgpEngines,
                                        const size_t cEngines,
                                        _Outptr_result_nullonfailure_ Renderer** const ppRenderer);

        [[nodiscard]]
        static HRESULT s_CreateInstance(_In_ std::unique_ptr<IRenderData> pData,
                                        _Outptr_result_nullonfailure_ Renderer** const ppRenderer);

        ~Renderer();

        [[nodiscard]]
        HRESULT PaintFrame();

        void TriggerSystemRedraw(const RECT* const prcDirtyClient) override;
        void TriggerRedraw(const SMALL_RECT* const psrRegion) override;
        void TriggerRedraw(const COORD* const pcoord) override;
        void TriggerRedrawCursor(const COORD* const pcoord) override;
        void TriggerRedrawAll() override;
        void TriggerTeardown() override;

        void TriggerSelection() override;
        void TriggerScroll() override;
        void TriggerScroll(const COORD* const pcoordDelta) override;

        void TriggerCircling() override;
        void TriggerTitleChange() override;

        void TriggerFontChange(const int iDpi,
                               const FontInfoDesired& FontInfoDesired,
                               _Out_ FontInfo& FontInfo) override;

        [[nodiscard]]
        HRESULT GetProposedFont(const int iDpi,
                                const FontInfoDesired& FontInfoDesired,
                                _Out_ FontInfo& FontInfo) override;

        COORD GetFontSize() override;
        bool IsGlyphWideByFont(const std::wstring_view glyph) override;

        void EnablePainting() override;
        void WaitForPaintCompletionAndDisable(const DWORD dwTimeoutMs) override;

        void AddRenderEngine(_In_ IRenderEngine* const pEngine) override;
        void RemoveRenderEngine(_In_ IRenderEngine* const pEngine) override;

    private:
        Renderer(_In_ std::unique_ptr<IRenderData> pData,
                    _In_reads_(cEngines) IRenderEngine** const pEngine,
                    const size_t cEngines);
        std::deque<IRenderEngine*> _rgpEngines;
        std::mutex _enginesLock;
        const std::unique_ptr<IRenderData> _pData;

        std::wstring _lastTitle;
        bool _titleChanged;

        RenderThread* _pThread;

        void _NotifyPaintFrame();

        [[nodiscard]]
        HRESULT _PaintFrameForEngine(_In_ IRenderEngine* const pEngine);

        bool _CheckViewportAndScroll();

        [[nodiscard]]
        HRESULT _PaintBackground(_In_ IRenderEngine* const pEngine);

        void _PaintBufferOutput(_In_ IRenderEngine* const pEngine);
        void _PaintBufferOutputRasterFontHelper(_In_ IRenderEngine* const pEngine,
                                                const ROW& pRow,
                                                _In_reads_(cchLine) PCWCHAR const pwsLine,
                                                const CharRow::const_iterator it,
                                                const CharRow::const_iterator itEnd,
                                                _In_ size_t cchLine,
                                                _In_ size_t iFirst,
                                                const COORD coordTarget,
                                                const bool lineWrapped);

        void _PaintBufferOutputColorHelper(_In_ IRenderEngine* const pEngine,
                                           const ROW& pRow,
                                           _In_reads_(cchLine) PCWCHAR const pwsLine,
                                           const CharRow::const_iterator it,
                                           const CharRow::const_iterator itEnd,
                                           _In_ size_t cchLine,
                                           _In_ size_t iFirst,
                                           const COORD coordTarget,
                                           const bool lineWrapped);

        [[nodiscard]]
        HRESULT _PaintBufferOutputDoubleByteHelper(_In_ IRenderEngine* const pEngine,
                                                   _In_reads_(cchLine) PCWCHAR const pwsLine,
                                                   const CharRow::const_iterator it,
                                                   const CharRow::const_iterator itEnd,
                                                   const size_t cchLine,
                                                   const COORD coordTarget,
                                                   const bool lineWrapped);

        void _PaintBufferOutputGridLineHelper(_In_ IRenderEngine* const pEngine,
                                              const TextAttribute textAttribute,
                                              const size_t cchLine,
                                              const COORD coordTarget);

        void _PaintSelection(_In_ IRenderEngine* const pEngine);
        void _PaintCursor(_In_ IRenderEngine* const pEngine);

        void _PaintIme(_In_ IRenderEngine* const pEngine,
                       const ConversionAreaInfo& AreaInfo,
                       const TextBuffer& textBuffer);
        void _PaintImeCompositionString(_In_ IRenderEngine* const pEngine);

        [[nodiscard]]
        HRESULT _UpdateDrawingBrushes(_In_ IRenderEngine* const pEngine, const TextAttribute attr, const bool fIncludeBackground);

        [[nodiscard]]
        HRESULT _ClearOverlays(_In_ IRenderEngine* const pEngine);
        [[nodiscard]]
        HRESULT _PerformScrolling(_In_ IRenderEngine* const pEngine);

        SMALL_RECT _srViewportPrevious;

        std::vector<SMALL_RECT> _GetSelectionRects() const;

        SMALL_RECT _RegionFromCoord(const COORD* const pcoord) const;
        COLORREF _ConvertAttrToRGB(const BYTE bAttr);

        [[nodiscard]]
        HRESULT _PaintTitle(IRenderEngine* const pEngine);

#ifdef DBG
        // Helper functions to diagnose issues with painting and layout.
        // These are only actually effective/on in Debug builds when the flag is set using an attached debugger.
        bool _fDebug = false;
#endif
    };
}
