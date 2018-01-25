/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "..\..\host\cursor.h"
#include "..\..\host\textBuffer.hpp"
#include "..\..\host\conimeinfo.h"

#include "renderer.hpp"

#include <algorithm>

#pragma hdrstop

using namespace Microsoft::Console::Render;
using namespace Microsoft::Console::Types;

// Routine Description:
// - Creates a new renderer controller for a console.
// Arguments:
// - pData - The interface to console data structures required for rendering
// - pEngine - The output engine for targeting each rendering frame
// Return Value:
// - An instance of a Renderer.
// NOTE: CAN THROW IF MEMORY ALLOCATION FAILS.
Renderer::Renderer(_In_ std::unique_ptr<IRenderData> pData,
                   _In_reads_(cEngines) IRenderEngine** const rgpEngines,
                   _In_ size_t const cEngines) :
    _pData(std::move(pData)),
    _pThread(nullptr)
{
    THROW_IF_NULL_ALLOC(_pData);

    _srViewportPrevious = { 0 };

    for (size_t i = 0; i < cEngines; i++)
    {
        IRenderEngine* engine = rgpEngines[i];
        // NOTE: THIS CAN THROW IF MEMORY ALLOCATION FAILS.
        AddRenderEngine(engine);
    }
}

// Routine Description:
// - Destroys an instance of a renderer
// Arguments:
// - <none>
// Return Value:
// - <none>
Renderer::~Renderer()
{
    if (_pThread != nullptr)
    {
        delete _pThread;
    }

    std::for_each(_rgpEngines.begin(), _rgpEngines.end(), [&](IRenderEngine* const pEngine) {
        delete pEngine;
    });
}

HRESULT Renderer::s_CreateInstance(_In_ std::unique_ptr<IRenderData> pData,
                                   _Outptr_result_nullonfailure_ Renderer** const ppRenderer)
{
    return Renderer::s_CreateInstance(std::move(pData), nullptr, 0,  ppRenderer);
}

HRESULT Renderer::s_CreateInstance(_In_ std::unique_ptr<IRenderData> pData,
                                   _In_reads_(cEngines) IRenderEngine** const rgpEngines,
                                   _In_ size_t const cEngines,
                                   _Outptr_result_nullonfailure_ Renderer** const ppRenderer)
{
    HRESULT hr = S_OK;

    Renderer* pNewRenderer = nullptr;
    try
    {
        pNewRenderer = new Renderer(std::move(pData), rgpEngines, cEngines);

        if (pNewRenderer == nullptr)
        {
            hr = E_OUTOFMEMORY;
        }
    }
    CATCH_RETURN();

    // Attempt to create renderer thread
    if (SUCCEEDED(hr))
    {
        RenderThread* pNewThread;

        hr = RenderThread::s_CreateInstance(pNewRenderer, &pNewThread);

        if (SUCCEEDED(hr))
        {
            pNewRenderer->_pThread = pNewThread;
        }
    }

    if (SUCCEEDED(hr))
    {
        *ppRenderer = pNewRenderer;
    }
    else
    {
        delete pNewRenderer;
    }

    return hr;
}

// Routine Description:
// - Walks through the console data structures to compose a new frame based on the data that has changed since last call and outputs it to the connected rendering engine.
// Arguments:
// - <none>
// Return Value:
// - HRESULT S_OK, GDI error, Safe Math error, or state/argument errors.
HRESULT Renderer::PaintFrame()
{
    for (IRenderEngine* const pEngine : _rgpEngines)
    {
        LOG_IF_FAILED(_PaintFrameForEngine(pEngine));
    }

    return S_OK;
}

HRESULT Renderer::_PaintFrameForEngine(_In_ IRenderEngine* const pEngine)
{
    THROW_HR_IF_NULL(HRESULT_FROM_WIN32(ERROR_INVALID_STATE), pEngine);

    // Last chance check if anything scrolled without an explicit invalidate notification since the last frame.
    _CheckViewportAndScroll();

    // Try to start painting a frame
    HRESULT const hr = pEngine->StartPaint();
    THROW_IF_FAILED(hr); // Return errors
    RETURN_HR_IF(S_OK, S_FALSE == hr); // Return early if there's nothing to paint.

    auto endPaint = wil::ScopeExit([&]()
    {
        LOG_IF_FAILED(pEngine->EndPaint());
    });

    // A. Prep Colors
    RETURN_IF_FAILED(_UpdateDrawingBrushes(pEngine, _pData->GetDefaultBrushColors(), true));

    // B. Clear Overlays
    RETURN_IF_FAILED(_ClearOverlays(pEngine));

    // C. Perform Scroll Operations
    RETURN_IF_FAILED(_PerformScrolling(pEngine));

    // 1. Paint Background
    RETURN_IF_FAILED(_PaintBackground(pEngine));

    // 2. Paint Rows of Text
    _PaintBufferOutput(pEngine);

    // 3. Paint Input
    //_PaintCookedInput(); // unnecessary, input is also stored in the output buffer.

    // 4. Paint IME composition area
    _PaintImeCompositionString(pEngine);

    // 5. Paint Selection
    _PaintSelection(pEngine);

    // 6. Paint Cursor
    _PaintCursor(pEngine);

    // As we leave the scope, EndPaint will be called (declared above)
    return S_OK;
}

void Renderer::_NotifyPaintFrame()
{
    // The thread will provide throttling for us.
    _pThread->NotifyPaint();
}

// Routine Description:
// - Called when the system has requested we redraw a portion of the console.
// Arguments:
// - <none>
// Return Value:
// - <none>
void Renderer::TriggerSystemRedraw(_In_ const RECT* const prcDirtyClient)
{
    std::for_each(_rgpEngines.begin(), _rgpEngines.end(), [&](IRenderEngine* const pEngine) {
        LOG_IF_FAILED(pEngine->InvalidateSystem(prcDirtyClient));
    });

    _NotifyPaintFrame();
}

// Routine Description:
// - Called when a particular region within the console buffer has changed.
// Arguments:
// - <none>
// Return Value:
// - <none>
void Renderer::TriggerRedraw(_In_ const SMALL_RECT* const psrRegion)
{
    Viewport view(_pData->GetViewport());
    SMALL_RECT srUpdateRegion = *psrRegion;

    if (view.TrimToViewport(&srUpdateRegion))
    {
        view.ConvertToOrigin(&srUpdateRegion);
        std::for_each(_rgpEngines.begin(), _rgpEngines.end(), [&](IRenderEngine* const pEngine) {
            LOG_IF_FAILED(pEngine->Invalidate(&srUpdateRegion));
        });

        _NotifyPaintFrame();
    }
}

// Routine Description:
// - Called when a particular coordinate within the console buffer has changed.
// Arguments:
// - <none>
// Return Value:
// - <none>
void Renderer::TriggerRedraw(_In_ const COORD* const pcoord)
{
    SMALL_RECT srRegion = _RegionFromCoord(pcoord);
    TriggerRedraw(&srRegion); // this will notify to paint if we need it.
}

// Routine Description:
// - Called when a particular coordinate within the console buffer has changed.
// Arguments:
// - <none>
// Return Value:
// - <none>
void Renderer::TriggerRedrawCursor(_In_ const COORD* const pcoord)
{
    Viewport view(_pData->GetViewport());
    COORD updateCoord = *pcoord;

    if (view.IsWithinViewport(&updateCoord))
    {
        view.ConvertToOrigin(&updateCoord);
        for (IRenderEngine* pEngine : _rgpEngines)
        {
            LOG_IF_FAILED(pEngine->InvalidateCursor(&updateCoord));
        }

        _NotifyPaintFrame();
    }
}

// Routine Description:
// - Called when something that changes the output state has occurred and the entire frame is now potentially invalid.
// - NOTE: Use sparingly. Try to reduce the refresh region where possible. Only use when a global state change has occurred.
// Arguments:
// - <none>
// Return Value:
// - <none>
void Renderer::TriggerRedrawAll()
{
    std::for_each(_rgpEngines.begin(), _rgpEngines.end(), [&](IRenderEngine* const pEngine) {
        LOG_IF_FAILED(pEngine->InvalidateAll());
    });

    _NotifyPaintFrame();
}

// Method Description:
// - Called when the host is about to die, to give the renderer one last chance 
//      to paint before the host exits. 
// Arguments:
// - <none>
// Return Value:
// - <none>
void Renderer::TriggerTeardown()
{
    for (IRenderEngine* const pEngine : _rgpEngines)
    {
        bool fEngineRequestsRepaint = false;
        HRESULT hr = pEngine->PrepareForTeardown(&fEngineRequestsRepaint);
        LOG_IF_FAILED(hr);

        if (SUCCEEDED(hr) && fEngineRequestsRepaint)
        {
            _PaintFrameForEngine(pEngine);
        }
    }
}

// Routine Description:
// - Called when the selected area in the console has changed.
// Arguments:
// - <none>
// Return Value:
// - <none>
void Renderer::TriggerSelection()
{
    // Get selection rectangles
    SMALL_RECT* rgsrSelection;
    UINT cRectsSelected;

    if (NT_SUCCESS(_GetSelectionRects(&rgsrSelection, &cRectsSelected)))
    {
        std::for_each(_rgpEngines.begin(), _rgpEngines.end(), [&](IRenderEngine* const pEngine) {
            LOG_IF_FAILED(pEngine->InvalidateSelection(rgsrSelection, cRectsSelected));
        });

        delete[] rgsrSelection;

        _NotifyPaintFrame();
    }
}

// Routine Description:
// - Called when we want to check if the viewport has moved and scroll accordingly if so.
// Arguments:
// - <none>
// Return Value:
// - True if something changed and we scrolled. False otherwise.
bool Renderer::_CheckViewportAndScroll()
{
    SMALL_RECT const srOldViewport = _srViewportPrevious;
    SMALL_RECT const srNewViewport = _pData->GetViewport();

    COORD coordDelta;
    coordDelta.X = srOldViewport.Left - srNewViewport.Left;
    coordDelta.Y = srOldViewport.Top - srNewViewport.Top;

    std::for_each(_rgpEngines.begin(), _rgpEngines.end(), [&](IRenderEngine* const pEngine) {
        LOG_IF_FAILED(pEngine->UpdateViewport(srNewViewport));
        LOG_IF_FAILED(pEngine->InvalidateScroll(&coordDelta));
    });
    _srViewportPrevious = srNewViewport;

    return coordDelta.X != 0 || coordDelta.Y != 0;
}

// Routine Description:
// - Called when a scroll operation has occurred by manipulating the viewport.
// - This is a special case as calling out scrolls explicitly drastically improves performance.
// Arguments:
// - <none>
// Return Value:
// - <none>
void Renderer::TriggerScroll()
{
    if (_CheckViewportAndScroll())
    {
        _NotifyPaintFrame();
    }
}

// Routine Description:
// - Called when a scroll operation wishes to explicitly adjust the frame by the given coordinate distance.
// - This is a special case as calling out scrolls explicitly drastically improves performance.
// - This should only be used when the viewport is not modified. It lets us know we can "scroll anyway" to save perf,
//   because the backing circular buffer rotated out from behind the viewport.
// Arguments:
// - <none>
// Return Value:
// - <none>
void Renderer::TriggerScroll(_In_ const COORD* const pcoordDelta)
{
    std::for_each(_rgpEngines.begin(), _rgpEngines.end(), [&](IRenderEngine* const pEngine){
        LOG_IF_FAILED(pEngine->InvalidateScroll(pcoordDelta));
    });

    _NotifyPaintFrame();
}

// Routine Description:
// - Called when the text buffer is about to circle it's backing buffer.
//      A renderer might want to get painted before that happens.
// Arguments:
// - <none>
// Return Value:
// - <none>
void Renderer::TriggerCircling()
{
    for (IRenderEngine* const pEngine : _rgpEngines)
    {
        bool fEngineRequestsRepaint = false;
        HRESULT hr = pEngine->InvalidateCircling(&fEngineRequestsRepaint);
        LOG_IF_FAILED(hr);

        if (SUCCEEDED(hr) && fEngineRequestsRepaint)
        {
            _PaintFrameForEngine(pEngine);
        }
    }
}

// Routine Description:
// - Called when a change in font or DPI has been detected.
// Arguments:
// - iDpi - New DPI value
// - pFontInfoDesired - A description of the font we would like to have.
// - pFontInfo - Data that will be fixed up/filled on return with the chosen font data.
// Return Value:
// - <none>
void Renderer::TriggerFontChange(_In_ int const iDpi, _In_ FontInfoDesired const * const pFontInfoDesired, _Out_ FontInfo* const pFontInfo)
{
    std::for_each(_rgpEngines.begin(), _rgpEngines.end(), [&](IRenderEngine* const pEngine){
        LOG_IF_FAILED(pEngine->UpdateDpi(iDpi));
        LOG_IF_FAILED(pEngine->UpdateFont(pFontInfoDesired, pFontInfo));
    });

    _NotifyPaintFrame();
}

// Routine Description:
// - Get the information on what font we would be using if we decided to create a font with the given parameters
// - This is for use with speculative calculations.
// Arguments:
// - iDpi - The DPI of the target display
// - pFontInfoDesired - A description of the font we would like to have.
// - pFontInfo - Data that will be fixed up/filled on return with the chosen font data.
// Return Value:
// - S_OK if set successfully or relevant GDI error via HRESULT.
HRESULT Renderer::GetProposedFont(_In_ int const iDpi, _In_ FontInfoDesired const * const pFontInfoDesired, _Out_ FontInfo* const pFontInfo)
{
    // If there's no head, return E_FAIL. The caller should decide how to
    //      handle this.
    // Currently, the only caller is the WindowProc:WM_GETDPISCALEDSIZE handler.
    //      It will assume that the proposed font is 1x1, regardless of DPI.
    if (_rgpEngines.size() < 1)
    {
        return E_FAIL;
    }

    // There will only every really be two engines - the real head and the VT
    //      renderer. We won't know which is which, so iterate over them.
    //      Only return the result of the successful one if it's not S_FALSE (which is the VT renderer)
    // TODO: 14560740 - The Window might be able to get at this info in a more sane manner
    assert(_rgpEngines.size() <= 2);
    for (IRenderEngine* const pEngine : _rgpEngines)
    {
        const HRESULT hr = LOG_IF_FAILED(pEngine->GetProposedFont(pFontInfoDesired, pFontInfo, iDpi));
        // We're looking for specifically S_OK, S_FALSE is not good enough.
        if (hr == S_OK)
        {
            return hr;
        }
    };

    return E_FAIL;
}

// Routine Description:
// - Retrieves the current X by Y (in pixels) size of the font in active use for drawing
// - NOTE: Generally the console host should avoid doing math in pixels unless absolutely necessary. Try to handle everything in character units and only let the renderer/window convert to pixels as necessary.
// Arguments:
// - <none>
// Return Value:
// - COORD representing the current pixel size of the selected font
COORD Renderer::GetFontSize()
{
    COORD fontSize = {1, 1};
    // There will only every really be two engines - the real head and the VT
    //      renderer. We won't know which is which, so iterate over them.
    //      Only return the result of the successful one if it's not S_FALSE (which is the VT renderer)
    // TODO: 14560740 - The Window might be able to get at this info in a more sane manner
    assert(_rgpEngines.size() <= 2);

    for (IRenderEngine* const pEngine : _rgpEngines)
    {
        const HRESULT hr = LOG_IF_FAILED(pEngine->GetFontSize(&fontSize));
        // We're looking for specifically S_OK, S_FALSE is not good enough.
        if (hr == S_OK)
        {
            return fontSize;
        }
    };

    return fontSize;
}

// Routine Description:
// - Tests against the current rendering engine to see if this particular character would be considered full-width (inscribed in a square, twice as wide as a standard Western character, typically used for CJK languages) or half-width.
// - Typically used to determine how many positions in the backing buffer a particular character should fill.
// NOTE: This only handles 1 or 2 wide (in monospace terms) characters.
// Arguments:
// - wch - Character to test
// Return Value:
// - True if the character is full-width (two wide), false if it is half-width (one wide).
bool Renderer::IsCharFullWidthByFont(_In_ WCHAR const wch)
{
    bool fIsFullWidth = false;

    // There will only every really be two engines - the real head and the VT
    //      renderer. We won't know which is which, so iterate over them.
    //      Only return the result of the successful one if it's not S_FALSE (which is the VT renderer)
    // TODO: 14560740 - The Window might be able to get at this info in a more sane manner
    assert(_rgpEngines.size() <= 2);
    for (IRenderEngine* const pEngine : _rgpEngines)
    {
        const HRESULT hr = LOG_IF_FAILED(pEngine->IsCharFullWidthByFont(wch, &fIsFullWidth));
        // We're looking for specifically S_OK, S_FALSE is not good enough.
        if (hr == S_OK)
        {
            return fIsFullWidth;
        }
    }

    return fIsFullWidth;
}

// Routine Description:
// - Sets an event in the render thread that allows it to proceed, thus enabling painting.
// Arguments:
// - <none>
// Return Value:
// - <none>
void Renderer::EnablePainting()
{
    _pThread->EnablePainting();
}

// Routine Description:
// - Waits for the current paint operation to complete, if any, up to the specified timeout.
// - Resets an event in the render thread that precludes it from advancing, thus disabling rendering.
// - If no paint operation is currently underway, returns immediately.
// Arguments:
// - dwTimeoutMs - Milliseconds to wait for the current paint operation to complete, if any (can be INFINITE).
// Return Value:
// - <none>
void Renderer::WaitForPaintCompletionAndDisable(const DWORD dwTimeoutMs)
{
    _pThread->WaitForPaintCompletionAndDisable(dwTimeoutMs);
}

// Routine Description:
// - Paint helper to fill in the background color of the invalid area within the frame.
// Arguments:
// - <none>
// Return Value:
// - <none>
HRESULT Renderer::_PaintBackground(_In_ IRenderEngine* const pEngine)
{
    return pEngine->PaintBackground();
}

// Routine Description:
// - Paint helper to copy the primary console buffer text onto the screen.
// - This portion primarily handles figuring the current viewport, comparing it/trimming it versus the invalid portion of the frame, and queuing up, row by row, which pieces of text need to be further processed.
// - See also: Helper functions that seperate out each complexity of text rendering.
// Arguments:
// - <none>
// Return Value:
// - <none>
void Renderer::_PaintBufferOutput(_In_ IRenderEngine* const pEngine)
{
    Viewport view(_pData->GetViewport());

    SMALL_RECT srDirty = pEngine->GetDirtyRectInChars();
    view.ConvertFromOrigin(&srDirty);

    const TEXT_BUFFER_INFO* const ptbi = _pData->GetTextBuffer();

    // The dirty rectangle may be larger than the backing buffer (anything, including the system, may have
    // requested that we render under the scroll bars). To prevent issues, trim down to the max buffer size
    // (a.k.a. ensure the viewport is between 0 and the max size of the buffer.)
    COORD const coordBufferSize = ptbi->GetCoordBufferSize();
    srDirty.Top = max(srDirty.Top, 0);
    srDirty.Left = max(srDirty.Left, 0);
    srDirty.Right = min(srDirty.Right, coordBufferSize.X - 1);
    srDirty.Bottom = min(srDirty.Bottom, coordBufferSize.Y - 1);

    // Also ensure that the dirty rect still fits inside the screen viewport.
    srDirty.Top = max(srDirty.Top, view.Top());
    srDirty.Left = max(srDirty.Left, view.Left());
    srDirty.Right = min(srDirty.Right, view.RightInclusive());
    srDirty.Bottom = min(srDirty.Bottom, view.BottomInclusive());

    Viewport viewDirty(srDirty);
    for (SHORT iRow = viewDirty.Top(); iRow <= viewDirty.BottomInclusive(); iRow++)
    {
        // Get row of text data
        const ROW* const pRow = ptbi->GetRowByOffset(iRow);

        // Get the requested left and right positions from the dirty rectangle.
        size_t iLeft = viewDirty.Left();
        size_t iRight = viewDirty.RightExclusive();

        // If there's anything to draw... draw it.
        if (iRight > iLeft)
        {
            // Get the pointer to the beginning of the text and the maximum length of the line we'll be writing.
            PWCHAR const pwsLine = pRow->CharRow.Chars + iLeft;
            PBYTE const pbKAttrs = pRow->CharRow.KAttrs + iLeft; // the double byte flags corresponding to the characters above.
            size_t const cchLine = iRight - iLeft;

            // Calculate the target position in the buffer where we should start writing.
            COORD coordTarget;
            coordTarget.X = (SHORT)iLeft - view.Left();
            coordTarget.Y = iRow - view.Top();

            // Now draw it.
            _PaintBufferOutputRasterFontHelper(pEngine, pRow, pwsLine, pbKAttrs, cchLine, iLeft, coordTarget);

#if DBG
            if (_fDebug)
            {
                // Draw a frame shape around the last character of a wrapped row to identify where there are soft wraps versus hard newlines.
                if (iRight == (size_t)pRow->CharRow.Right && pRow->CharRow.WasWrapForced())
                {
                    IRenderEngine::GridLines lines = IRenderEngine::GridLines::Right | IRenderEngine::GridLines::Bottom;
                    COORD coordDebugTarget;
                    coordDebugTarget.Y = iRow - view.Top();
                    coordDebugTarget.X = (SHORT)iRight - view.Left() - 1;
                    pEngine->PaintBufferGridLines(lines, RGB(0x99, 0x77, 0x31), 1, coordDebugTarget);
                }
            }
#endif
        }
    }
}

// Routine Description:
// - Paint helper for raster font text. It will pass through to ColorHelper when it's done and cascade from there.
// - This particular helper checks the current font and converts the text, if necessary, back to the original OEM codepage.
// - This is required for raster fonts in GDI as it won't adapt them back on our behalf.
// - See also: All related helpers and buffer output functions.
// Arguments:
// - pRow - Pointer to the row structure for the current line of text
// - pwsLine - Pointer to the first character in the string/substring to be drawn.
// - pbKAttrsLine - Pointer to the first attribute in a sequence that is perfectly in sync with the pwsLine parameter. e.g. The first attribute here goes with the first character in pwsLine.
// - cchLine - The length of both pwsLine and pbKAttrsLine.
// - iFirstAttr - Index into the row corresponding to pwsLine[0] to match up the appropriate color.
// - coordTarget - The X/Y coordinate position on the screen which we're attempting to render to.
// Return Value:
// - <none>
void Renderer::_PaintBufferOutputRasterFontHelper(_In_ IRenderEngine* const pEngine,
                                                  _In_ const ROW* const pRow,
                                                  _In_reads_(cchLine) PCWCHAR const pwsLine,
                                                  _In_reads_(cchLine) PBYTE pbKAttrsLine,
                                                  _In_ size_t cchLine,
                                                  _In_ size_t iFirstAttr,
                                                  _In_ COORD const coordTarget)
{
    const FontInfo* const pFontInfo = _pData->GetFontInfo();

    PWCHAR pwsConvert = nullptr;
    PCWCHAR pwsData = pwsLine; // by default, use the line data.

    // If we're not using a TrueType font, we'll have to re-interpret the line of text to make the raster font happy.
    if (!pFontInfo->IsTrueTypeFont())
    {
        UINT const uiCodePage = pFontInfo->GetCodePage();

        // dispatch conversion into our codepage

        // Find out the bytes required
        int const cbRequired = WideCharToMultiByte(uiCodePage, 0, pwsLine, (int)cchLine, nullptr, 0, nullptr, nullptr);

        if (cbRequired != 0)
        {
            // Allocate buffer for MultiByte
            PCHAR psConverted = new CHAR[cbRequired];

            if (psConverted != nullptr)
            {
                // Attempt conversion to current codepage
                int const cbConverted = WideCharToMultiByte(uiCodePage, 0, pwsLine, (int)cchLine, psConverted, cbRequired, nullptr, nullptr);

                // If successful...
                if (cbConverted != 0)
                {
                    // Now we have to convert back to Unicode but using the system ANSI codepage. Find buffer size first.
                    int const cchRequired = MultiByteToWideChar(CP_ACP, 0, psConverted, cbRequired, nullptr, 0);

                    if (cchRequired != 0)
                    {
                        pwsConvert = new WCHAR[cchRequired];

                        if (pwsConvert != nullptr)
                        {

                            // Then do the actual conversion.
                            int const cchConverted = MultiByteToWideChar(CP_ACP, 0, psConverted, cbRequired, pwsConvert, cchRequired);

                            if (cchConverted != 0)
                            {
                                // If all successful, use this instead.
                                pwsData = pwsConvert;
                            }
                        }
                    }
                }

                delete[] psConverted;
            }
        }

    }

    // If we are using a TrueType font, just call the next helper down.
    _PaintBufferOutputColorHelper(pEngine, pRow, pwsData, pbKAttrsLine, cchLine, iFirstAttr, coordTarget);

    if (pwsConvert != nullptr)
    {
        delete[] pwsConvert;
    }
}

// Routine Description:
// - Paint helper for primary buffer output function.
// - This particular helper unspools the run-length-encoded color attributes, updates the brushes, and effectively substrings the text for each different color.
// - It also identifies box drawing attributes and calls the respective helper.
// - See also: All related helpers and buffer output functions.
// Arguments:
// - pRow - Pointer to the row structure for the current line of text
// - pwsLine - Pointer to the first character in the string/substring to be drawn.
// - pbKAttrsLine - Pointer to the first attribute in a sequence that is perfectly in sync with the pwsLine parameter. e.g. The first attribute here goes with the first character in pwsLine.
// - cchLine - The length of both pwsLine and pbKAttrsLine.
// - iFirstAttr - Index into the row corresponding to pwsLine[0] to match up the appropriate color.
// - coordTarget - The X/Y coordinate position on the screen which we're attempting to render to.
// Return Value:
// - <none>
void Renderer::_PaintBufferOutputColorHelper(_In_ IRenderEngine* const pEngine,
                                             _In_ const ROW* const pRow,
                                             _In_reads_(cchLine) PCWCHAR const pwsLine,
                                             _In_reads_(cchLine) PBYTE pbKAttrsLine,
                                             _In_ size_t cchLine,
                                             _In_ size_t iFirstAttr,
                                             _In_ COORD const coordTarget)
{
    // We may have to write this string in several pieces based on the colors.

    // Count up how many characters we've written so we know when we're done.
    size_t cchWritten = 0;

    // The offset from the target point starts at the target point (and may be adjusted rightward for another string segment
    // if this attribute/color doesn't have enough 'length' to apply to all the text we want to draw.)
    COORD coordOffset = coordTarget;

    // The line segment we'll write will start at the beginning of the text.
    PCWCHAR pwsSegment = pwsLine;
    PBYTE pbKAttrsSegment = pbKAttrsLine; // corresponding double byte flags pointer

    do
    {
        // First retrieve the attribute that applies starting at the target position and the length for which it applies.
        TextAttributeRun* pRun = nullptr;
        UINT cAttrApplies = 0;
        pRow->AttrRow.FindAttrIndex((UINT)(iFirstAttr + cchWritten), &pRun, &cAttrApplies);

        // Set the brushes in GDI to this color
        LOG_IF_FAILED(_UpdateDrawingBrushes(pEngine, pRun->GetAttributes(), false));

        // The segment we'll write is the shorter of the entire segment we want to draw or the amount of applicable color (Attr applies)
        size_t cchSegment = min(cchLine - cchWritten, cAttrApplies);

        if (cchSegment <= 0)
        {
            // If we ever have an invalid segment length, stop looping through the rendering.
            break;
        }

        // Draw the line via double-byte helper to strip duplicates
        LOG_IF_FAILED(_PaintBufferOutputDoubleByteHelper(pEngine, pwsSegment, pbKAttrsSegment, cchSegment, coordOffset));

        // Draw the grid shapes without the double-byte helper as they need to be exactly proportional to what's in the buffer
        if (_pData->IsGridLineDrawingAllowed())
        {
            // We're only allowed to draw the grid lines under certain circumstances.
            _PaintBufferOutputGridLineHelper(pEngine, pRun->GetAttributes(), cchSegment, coordOffset);
        }

        // Update how much we've written.
        cchWritten += cchSegment;

        // Update the offset and text segment pointer by moving right by the previously written segment length
        coordOffset.X += (SHORT)cchSegment;
        pwsSegment += cchSegment;
        pbKAttrsSegment += cchSegment;

    } while (cchWritten < cchLine);
}

// Routine Description:
// - Paint helper for primary buffer output function.
// - This particular helper processes full-width (sometimes called double-wide or double-byte) characters. They're typically stored twice in the backing buffer to represent their width, so this function will help strip that down to one copy each as it's passed along to the final output.
// - See also: All related helpers and buffer output functions.
// Arguments:
// - pwsLine - Pointer to the first character in the string/substring to be drawn.
// - pbKAttrsLine - Pointer to the first attribute in a sequence that is perfectly in sync with the pwsLine parameter. e.g. The first attribute here goes with the first character in pwsLine.
// - cchLine - The length of both pwsLine and pbKAttrsLine.
// - coordTarget - The X/Y coordinate position in the buffer which we're attempting to start rendering from. pwsLine[0] will be the character at position coordTarget within the original console buffer before it was prepared for this function.
// Return Value:
// - S_OK or memory allocation error
HRESULT Renderer::_PaintBufferOutputDoubleByteHelper(_In_ IRenderEngine* const pEngine,
                                                     _In_reads_(cchLine) PCWCHAR const pwsLine,
                                                     _In_reads_(cchLine) PBYTE const pbKAttrsLine,
                                                     _In_ size_t const cchLine,
                                                     _In_ COORD const coordTarget)
{
    // We need the ability to move the target back to the left slightly in case we start with a trailing byte character.
    COORD coordTargetAdjustable = coordTarget;
    bool fTrimLeft = false;

    // We need to filter out double-copies of characters that get introduced for full-width characters (sometimes "double-width" or erroneously "double-byte")
    wistd::unique_ptr<WCHAR[]> pwsSegment = wil::make_unique_nothrow<WCHAR[]>(cchLine);
    RETURN_IF_NULL_ALLOC(pwsSegment);

    // We will need to pass the expected widths along so characters can be spaced to fit.
    wistd::unique_ptr<unsigned char[]> rgSegmentWidth = wil::make_unique_nothrow<unsigned char[]>(cchLine);
    RETURN_IF_NULL_ALLOC(rgSegmentWidth);

    size_t cchSegment = 0;
    // Walk through the line given character by character and copy necessary items into our local array.
    for (size_t iLine = 0; iLine < cchLine; iLine++)
    {
        // skip copy of trailing bytes. we'll copy leading and single bytes into the final write array.
        if ((pbKAttrsLine[iLine] & CHAR_ROW::ATTR_TRAILING_BYTE) == 0)
        {
            pwsSegment[cchSegment] = pwsLine[iLine];
            rgSegmentWidth[cchSegment] = 1;

            // If this is a leading byte, add 1 more to width as it is double wide
            if ((pbKAttrsLine[iLine] & CHAR_ROW::ATTR_LEADING_BYTE) != 0)
            {
                rgSegmentWidth[cchSegment] = 2;
            }

            cchSegment++;
        }
        else if (iLine == 0)
        {
            // If we are a trailing byte, but we're the first character in the run, it's a special case.
            // Someone has asked us to redraw the right half of the character, but we can't do that.
            // We'll have to draw the entire thing at once which requires:
            // 1. We have to copy the character into the segment buffer (which we normally don't do for trailing bytes)
            // 2. We have to back the draw target up by one character width so the right half will be struck over where we expect

            // Copy character
            pwsSegment[cchSegment] = pwsLine[iLine];

            // This character is double-width
            rgSegmentWidth[cchSegment] = 2;
            cchSegment++;

            // Move the target back one so we can strike left of what we want.
            coordTargetAdjustable.X--;

            // And set the flag so the engine will trim off the extra left half of the character
            // Clipping the left half of the character is important because leaving it there will interfere with the line drawing routines
            // which have no knowledge of the half/fullwidthness of characters and won't necessarily restrike the lines on the left half of the character.
            fTrimLeft = true;
        }
    }

    // Draw the line
    RETURN_IF_FAILED(pEngine->PaintBufferLine(pwsSegment.get(), rgSegmentWidth.get(), cchSegment, coordTargetAdjustable, fTrimLeft));

    return S_OK;
}

// Routine Description:
// - Paint helper for primary buffer output function.
// - This particular helper sets up the various box drawing lines that can be inscribed around any character in the buffer (left, right, top, underline).
// - See also: All related helpers and buffer output functions.
// Arguments:
// - textAttribute - The line/box drawing attributes to use for this particular run.
// - cchLine - The length of both pwsLine and pbKAttrsLine.
// - coordTarget - The X/Y coordinate position in the buffer which we're attempting to start rendering from.
// Return Value:
// - <none>
void Renderer::_PaintBufferOutputGridLineHelper(_In_ IRenderEngine* const pEngine,
                                                _In_ const TextAttribute textAttribute,
                                                _In_ size_t const cchLine,
                                                _In_ COORD const coordTarget)
{
    COLORREF rgb = textAttribute.CalculateRgbForeground();

    // Convert console grid line representations into rendering engine enum representations.
    IRenderEngine::GridLines lines = IRenderEngine::GridLines::None;

    if (textAttribute.IsTopHorizontalDisplayed())
    {
        lines |= IRenderEngine::GridLines::Top;
    }

    if (textAttribute.IsBottomHorizontalDisplayed())
    {
        lines |= IRenderEngine::GridLines::Bottom;
    }

    if (textAttribute.IsLeftVerticalDisplayed())
    {
        lines |= IRenderEngine::GridLines::Left;
    }

    if (textAttribute.IsRightVerticalDisplayed())
    {
        lines |= IRenderEngine::GridLines::Right;
    }

    // Draw the lines
    LOG_IF_FAILED(pEngine->PaintBufferGridLines(lines, rgb, cchLine, coordTarget));
}

// Routine Description:
// - Paint helper to draw the cursor within the buffer.
// Arguments:
// - <none>
// Return Value:
// - <none>
void Renderer::_PaintCursor(_In_ IRenderEngine* const pEngine)
{
    const Cursor* const pCursor = _pData->GetCursor();

    if (pCursor->IsVisible() && pCursor->IsOn() && !pCursor->IsPopupShown())
    {
        // Get cursor position in buffer
        COORD coordCursor = pCursor->GetPosition();

        Viewport view(_pData->GetViewport());

        // Always attempt to paint the cursor, even if it's not within the 
        //      "dirty" part of the viewport.

        // Determine cursor height
        ULONG ulHeight = pCursor->GetSize();

        // Now adjust the height for the overwrite/insert mode. If we're in overwrite mode, IsDouble will be set.
        // When IsDouble is set, we either need to double the height of the cursor, or if it's already too big,
        // then we need to shrink it by half.
        if (pCursor->IsDouble())
        {
            if (ulHeight > 50) // 50 because 50 percent is half of 100 percent which is the max size.
            {
                ulHeight >>= 1;
            }
            else
            {
                ulHeight <<= 1;
            }
        }

        // Determine cursor width
        bool const fIsDoubleWidth = !!pCursor->IsDoubleWidth();

        // Adjust cursor to viewport
        view.ConvertToOrigin(&coordCursor);

        // Draw it within the viewport
        LOG_IF_FAILED(pEngine->PaintCursor(coordCursor, ulHeight, fIsDoubleWidth));
    }
}

// Routine Description:
// - Paint helper to draw the IME within the buffer.
// - This supports composition drawing area.
// Arguments:
// - pAreaInfo - Special IME area screen buffer metadata
// - pTextInfo - Text backing buffer for the special IME area.
// Return Value:
// - <none>
void Renderer::_PaintIme(_In_ IRenderEngine* const pEngine, _In_ const ConversionAreaInfo* const pAreaInfo, _In_ const TEXT_BUFFER_INFO* const pTextInfo)
{
    // If this conversion area isn't hidden (because it is off) or hidden for a scroll operation, then draw it.
    if (!pAreaInfo->IsHidden())
    {
        // First get the screen buffer's viewport.
        Viewport view(_pData->GetViewport());

        // Now get the IME's viewport and adjust it to where it is supposed to be relative to the window.
        // The IME's buffer is typically only one row in size. Some segments are the whole row, some are only a partial row.
        // Then from those, there is a "view" much like there is a view into the main console buffer.
        // Use the "window" and "view" relative to the IME-specific special buffer to figure out the coordinates to draw at within the real console buffer.
        SMALL_RECT srCaView = pAreaInfo->CaInfo.rcViewCaWindow;
        srCaView.Top += pAreaInfo->CaInfo.coordConView.Y;
        srCaView.Bottom += pAreaInfo->CaInfo.coordConView.Y;
        srCaView.Left += pAreaInfo->CaInfo.coordConView.X;
        srCaView.Right += pAreaInfo->CaInfo.coordConView.X;

        // Set it up in a Viewport helper structure and trim it the IME viewport to be within the full console viewport.
        Viewport viewConv(srCaView);

        SMALL_RECT srDirty = pEngine->GetDirtyRectInChars();

        // Dirty is an inclusive rectangle, but oddly enough the IME was an exclusive one, so correct it.
        srDirty.Bottom++;
        srDirty.Right++;

        if (viewConv.TrimToViewport(&srDirty))
        {
            Viewport viewDirty(srDirty);

            for (SHORT iRow = viewDirty.Top(); iRow < viewDirty.BottomInclusive(); iRow++)
            {
                // Get row of text data
                const ROW* const pRow = pTextInfo->GetRowByOffset(iRow - pAreaInfo->CaInfo.coordConView.Y);

                // Get the pointer to the beginning of the text and the maximum length of the line we'll be writing.
                PWCHAR const pwsLine = pRow->CharRow.Chars + viewDirty.Left() - pAreaInfo->CaInfo.coordConView.X;
                PBYTE const pbKAttrs = pRow->CharRow.KAttrs + viewDirty.Left() - pAreaInfo->CaInfo.coordConView.X; // the double byte flags corresponding to the characters above.
                size_t const cchLine = viewDirty.Width() - 1;

                // Calculate the target position in the buffer where we should start writing.
                COORD coordTarget;
                coordTarget.X = viewDirty.Left();
                coordTarget.Y = iRow;

                _PaintBufferOutputRasterFontHelper(pEngine, pRow, pwsLine, pbKAttrs, cchLine, viewDirty.Left(), coordTarget);
            }
        }
    }
}

// Routine Description:
// - Paint helper to draw the composition string portion of the IME.
// - This specifically is the string that appears at the cursor on the input line showing what the user is currently typing.
// - See also: Generic Paint IME helper method.
// Arguments:
// - <none>
// Return Value:
// - <none>
void Renderer::_PaintImeCompositionString(_In_ IRenderEngine* const pEngine)
{
    const ConsoleImeInfo* const pImeData = _pData->GetImeData();

    for (size_t i = 0; i < pImeData->ConvAreaCompStr.size(); i++)
    {
        ConversionAreaInfo* pAreaInfo = pImeData->ConvAreaCompStr[i];

        if (pAreaInfo != nullptr)
        {
            const TEXT_BUFFER_INFO* const ptbi = _pData->GetImeCompositionStringBuffer(i);
            _PaintIme(pEngine, pAreaInfo, ptbi);
        }
    }
}

// Routine Description:
// - Paint helper to draw the selected area of the window.
// Arguments:
// - <none>
// Return Value:
// - <none>
void Renderer::_PaintSelection(_In_ IRenderEngine* const pEngine)
{
    // Get selection rectangles
    SMALL_RECT* rgsrSelection;
    UINT cRectsSelected;

    if (NT_SUCCESS(_GetSelectionRects(&rgsrSelection, &cRectsSelected)))
    {
        if (cRectsSelected > 0)
        {
            LOG_IF_FAILED(pEngine->PaintSelection(rgsrSelection, cRectsSelected));
        }

        delete[] rgsrSelection;
    }
}

// Routine Description:
// - Helper to convert the text attributes to actual RGB colors and update the rendering pen/brush within the rendering engine before the next draw operation.
// Arguments:
// - textAttributes - The 16 color foreground/background combination to set
// - fIncludeBackground - Whether or not to include the hung window/erase window brushes in this operation. (Usually only happens when the default is changed, not when each individual color is swapped in a multi-color run.)
// Return Value:
// - <none>
HRESULT Renderer::_UpdateDrawingBrushes(_In_ IRenderEngine* const pEngine, _In_ const TextAttribute textAttributes, _In_ bool const fIncludeBackground)
{
    COLORREF rgbForeground = textAttributes.CalculateRgbForeground();
    COLORREF rgbBackground = textAttributes.CalculateRgbBackground();
    WORD legacyAttributes = textAttributes.GetLegacyAttributes();

    // The last color need's to be each engine's responsibility. If it's local to this function,
    //      then on the next engine we might not update the color.
    RETURN_IF_FAILED(pEngine->UpdateDrawingBrushes(rgbForeground, rgbBackground, legacyAttributes, fIncludeBackground));

    return S_OK;
}

// Routine Description:
// - Helper called at the beginning of a frame to clear any overlaid text, cursors, or other items that we don't
//   want to have included when the existing frame is scrolled
// Arguments:
// - <none>
// Return Value:
// - <none>
HRESULT Renderer::_ClearOverlays(_In_ IRenderEngine* const pEngine)
{
    return pEngine->ClearCursor();
}

// Routine Description:
// - Helper called before a majority of paint operations to scroll most of the previous frame into the appropriate
//   position before we paint the remaining invalid area.
// - Used to save drawing time/improve performance
// Arguments:
// - <none>
// Return Value:
// - <none>
HRESULT Renderer::_PerformScrolling(_In_ IRenderEngine* const pEngine)
{
    return pEngine->ScrollFrame();
}

// Routine Description:
// - Helper to determine the selected region of the buffer.
// - NOTE: CALLER MUST FREE THE ARRAY RECEIVED IF THE SIZE WAS NON-ZERO
// Arguments:
// - prgsrSelection - Pointer to callee allocated array of rectangles representing the selection area, one per line of the selection.
// - pcRectangles - Count of how many rectangles are in the above array.
// Return Value:
// - Success status if we managed to retrieve rectangles. Check with NT_SUCCESS.
_Check_return_
NTSTATUS Renderer::_GetSelectionRects(
    _Outptr_result_buffer_all_(*pcRectangles) SMALL_RECT** const prgsrSelection,
    _Out_ UINT* const pcRectangles) const
{
    NTSTATUS status = _pData->GetSelectionRects(prgsrSelection, pcRectangles);

    if (NT_SUCCESS(status))
    {
        // Adjust rectangles to viewport
        Viewport view(_pData->GetViewport());
        for (UINT iRect = 0; iRect < *pcRectangles; iRect++)
        {
            view.ConvertToOrigin(&(*prgsrSelection)[iRect]);

            // hopefully temporary, we should be receiving the right selection sizes without correction.
            (*prgsrSelection)[iRect].Right++;
            (*prgsrSelection)[iRect].Bottom++;
        }
    }

    return status;
}

// Method Description:
// - Adds another Render engine to this renderer. Future rendering calls will
//      also be sent to the new renderer.
// Arguments:
// - pEngine: The new render engine to be added
// Return Value:
// - <none>
// Throws if we ran out of memory or there was some other error appending the
//      engine to our collection.
void Renderer::AddRenderEngine(_In_ IRenderEngine* const pEngine)
{
    THROW_IF_NULL_ALLOC(pEngine);
    _rgpEngines.push_back(pEngine);
}
