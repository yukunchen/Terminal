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
        Renderer(IRenderData* pData,
                 _In_reads_(cEngines) IRenderEngine** const pEngine,
                 const size_t cEngines);

        [[nodiscard]]
        static HRESULT s_CreateInstance(IRenderData* pData,
                                        _In_reads_(cEngines) IRenderEngine** const rgpEngines,
                                        const size_t cEngines,
                                        _Outptr_result_nullonfailure_ Renderer** const ppRenderer);

        [[nodiscard]]
        static HRESULT s_CreateInstance(IRenderData* pData,
                                        _Outptr_result_nullonfailure_ Renderer** const ppRenderer);

        ~Renderer();

        [[nodiscard]]
        HRESULT PaintFrame();

        void TriggerSystemRedraw(const RECT* const prcDirtyClient) override;
        void TriggerRedraw(const Microsoft::Console::Types::Viewport& region) override;
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

        void SetThread(IRenderThread* pThread);
    private:
        std::deque<IRenderEngine*> _rgpEngines;
        IRenderData* _pData;

        IRenderThread* _pThread;

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

        static IRenderEngine::GridLines s_GetGridlines(const TextAttribute& textAttribute) noexcept;

        void _PaintBufferOutputGridLineHelper(_In_ IRenderEngine* const pEngine,
                                              const TextAttribute textAttribute,
                                              const size_t cchLine,
                                              const COORD coordTarget);

        void _PaintSelection(_In_ IRenderEngine* const pEngine);
        void _PaintCursor(_In_ IRenderEngine* const pEngine);

        // TODO: add this to conhost seperately
        // void _PaintIme(_In_ IRenderEngine* const pEngine,
        //                const ConversionAreaInfo& AreaInfo,
        //                const TextBuffer& textBuffer);
        void _PaintImeCompositionString(_In_ IRenderEngine* const pEngine);

        [[nodiscard]]
        HRESULT _UpdateDrawingBrushes(_In_ IRenderEngine* const pEngine, const TextAttribute attr, const bool fIncludeBackground);

        [[nodiscard]]
        HRESULT _PerformScrolling(_In_ IRenderEngine* const pEngine);

        SMALL_RECT _srViewportPrevious;

        std::vector<SMALL_RECT> _GetSelectionRects() const;
        std::vector<SMALL_RECT> _previousSelection;

        [[nodiscard]]
        HRESULT _PaintTitle(IRenderEngine* const pEngine);

#ifdef DBG
        // Helper functions to diagnose issues with painting and layout.
        // These are only actually effective/on in Debug builds when the flag is set using an attached debugger.
        bool _fDebug = false;
#endif
    };
}
