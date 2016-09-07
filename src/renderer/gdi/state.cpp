/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "gdirenderer.hpp"

#include <winuserp.h> // for GWL_CONSOLE_BKCOLOR

#pragma hdrstop

using namespace Microsoft::Console::Render;

// Routine Description:
// - Creates a new GDI-based rendering engine
// Arguments:
// - <none>
// Return Value:
// - An instance of a Renderer.
GdiEngine::GdiEngine() :
    _hwndTargetWindow((HWND)INVALID_HANDLE_VALUE),
    _iCurrentDpi(s_iBaseDpi),
    _hbitmapMemorySurface(nullptr),
    _cPolyText(0),
    _fInvalidRectUsed(false),
    _fPaintStarted(false)
{
    ZeroMemory(_pPolyText, sizeof(POLYTEXTW) * s_cPolyTextCache);
    _rcInvalid = { 0 };
    _rcCursorInvert = { 0 };
    _szInvalidScroll = { 0 };
    _szMemorySurface = { 0 };
    _hrgnGdiPaintedSelection = CreateRectRgn(0, 0, 0, 0);
    _hdcMemoryContext = CreateCompatibleDC(nullptr);
}

// Routine Description:
// - Destroys an instance of a GDI-based rendering engine
// Arguments:
// - <none>
// Return Value:
// - <none>
GdiEngine::~GdiEngine()
{
    for (size_t iPoly = 0; iPoly < _cPolyText; iPoly++)
    {
        if (_pPolyText[iPoly].lpstr != nullptr)
        {
            delete[] _pPolyText[iPoly].lpstr;
        }
    }

    if (_hrgnGdiPaintedSelection != nullptr)
    {
        DeleteObject(_hrgnGdiPaintedSelection);
        _hrgnGdiPaintedSelection = nullptr;
    }

    if (_hbitmapMemorySurface != nullptr)
    {
        DeleteObject(_hbitmapMemorySurface);
        _hbitmapMemorySurface = nullptr;
    }

    if (_hfont != nullptr)
    {
        DeleteObject(_hfont);
        _hfont = nullptr;
    }

    if (_hdcMemoryContext != nullptr)
    {
        DeleteObject(_hdcMemoryContext);
        _hdcMemoryContext = nullptr;
    }
}

// Routine Description:
// - Updates the window to which this GDI renderer will be bound.
// - A window handle is required for determining the client area and other properties about the rendering surface and monitor.
// Arguments:
// - hwnd - Handle to the window on which we will be drawing.
// Return Value:
// - <none>
void GdiEngine::SetHwnd(_In_ HWND const hwnd)
{
    _hwndTargetWindow = hwnd;

    if (_hdcMemoryContext != nullptr)
    {
        DeleteObject(_hdcMemoryContext);
        _hdcMemoryContext = nullptr;
    }

    HDC hdcRealWindow = GetDC(_hwndTargetWindow);

    if (hdcRealWindow != nullptr)
    {
        _hdcMemoryContext = CreateCompatibleDC(hdcRealWindow);

        if (_hfont != nullptr)
        {
            SelectObject(_hdcMemoryContext, _hfont);
        }

        ReleaseDC(_hwndTargetWindow, hdcRealWindow);
    }
}

// Routine Description:
// - This method will set the GDI brushes in the drawing context (and update the hung-window background color)
// Arguments:
// - wTextAttributes - A console attributes bit field specifying the brush colors we should use.
// Return Value:
// - <none>
void GdiEngine::UpdateDrawingBrushes(_In_ COLORREF const colorForeground, _In_ COLORREF const colorBackground, _In_ bool const fIncludeBackgrounds)
{
    _FlushBufferLines();

    // Set the colors for painting text
    SetTextColor(_hdcMemoryContext, colorForeground);
    SetBkColor(_hdcMemoryContext, colorBackground);

    if (fIncludeBackgrounds)
    {
        // Set the color for painting the extra DC background area
        SetDCBrushColor(_hdcMemoryContext, colorBackground);

        // Set the hung app background painting color
        SetWindowLongW(_hwndTargetWindow, GWL_CONSOLE_BKCOLOR, colorBackground); // Store background color
    }
}

// Routine Description:
// - This method will update the active font on the current device context
// - NOTE: It is left up to the underling rendering system to choose the nearest font. Please ask for the font dimensions if they are required using the interface. Do not use the size you requested with this structure.
// Arguments:
// - pfiFont - Pointer to font information we should use while instantiating a font.
// Return Value:
// - <none>
void GdiEngine::UpdateFont(_Inout_ FontInfo* const pfiFont)
{
    HFONT hFont;
    GetProposedFont(pfiFont, _iCurrentDpi, &hFont);

    // Select into DC
    SelectObject(_hdcMemoryContext, hFont);

    // Persist font for cleanup (and free existing if necessary)
    if (_hfont != nullptr)
    {
        DeleteObject(_hfont);
        _hfont = nullptr;
    }

    _hfont = hFont;

    // Save off the font metrics for various other calculations
    GetTextMetricsW(_hdcMemoryContext, &_tmFontMetrics);

    // Now find the size of a 0 in this current font and save it for conversions done later.
    _coordFontLast = pfiFont->GetSize();
}

// Routine Description:
// - This method will modify the DPI we're using for scaling calculations.
// Arguments:
// - iDpi - The Dots Per Inch to use for scaling. We will use this relative to the system default DPI defined in Windows headers as a constant.
// Return Value:
// - <none>
void GdiEngine::UpdateDpi(_In_ int const iDpi)
{
    _iCurrentDpi = iDpi;
}

// Routine Description:
// - This method will figure out what the new font should be given the starting font information and a DPI.
// - When the final font is determined, the FontInfo structure given will be updated with the actual resulting font chosen as the nearest match.
// - NOTE: It is left up to the underling rendering system to choose the nearest font. Please ask for the font dimensions if they are required using the interface. Do not use the size you requested with this structure.
// - If the intent is to immediately turn around and use this font, pass the optional handle parameter and use it immediately.
// Arguments:
// - pfiFont - Pointer to font information we should use while instantiating a font.
// - iDpi - The DPI we will have when rendering
// - phFont - Optionally a pointer to receive a handle to a ready-to-use GDI font.
// Return Value:
// - <none>
void GdiEngine::GetProposedFont(_Inout_ FontInfo* const pfiFont, _In_ int const iDpi, _Out_opt_ HFONT* const phFont /*= nullptr */)
{
    HDC hdcTemp = CreateCompatibleDC(_hdcMemoryContext);

    if (hdcTemp != nullptr)
    {
        // Get a special engine size because TT fonts can't specify X or we'll get weird scaling under some circumstances.
        COORD coordFontRequested = pfiFont->GetEngineSize();


        // For future reference, here is the engine weighting and internal details on Windows Font Mapping:
        // https://msdn.microsoft.com/en-us/library/ms969909.aspx
        // More relevant links:
        // https://support.microsoft.com/en-us/kb/94646

        // IMPORTANT: Be very careful when modifying the values being passed in below. Even the slightest change can cause
        // GDI to return a font other than the one being requested. If you must change the below for any reason, make sure
        // these fonts continue to work correctly, as they've been known to break:
        //       * Monofur
        //       * Iosevka Extralight
        //
        // While you're at it, make sure that the behavior matches what happens in the Fonts property sheet. Pay very close
        // attention to the font previews to ensure that the font being selected by GDI is exactly the font requested --
        // some monospace fonts look very similar.
        LOGFONTW lf = { 0 };
        lf.lfHeight = s_ScaleByDpi(coordFontRequested.Y, iDpi);
        lf.lfWidth = s_ScaleByDpi(coordFontRequested.X, iDpi);
        lf.lfWeight = pfiFont->GetWeight();
        lf.lfCharSet = pfiFont->GetCharSet();
        lf.lfQuality = DRAFT_QUALITY;

        // NOTE: not using what GDI gave us because some fonts don't quite roundtrip (e.g. MS Gothic and VL Gothic)
        lf.lfPitchAndFamily = (FIXED_PITCH | FF_MODERN);

        wcscpy_s(lf.lfFaceName, ARRAYSIZE(lf.lfFaceName), pfiFont->GetFaceName());

        // Create font.
        HFONT hFont = CreateFontIndirectW(&lf);
        if (hFont == nullptr)
        {
            assert(false);
        }

        // Select into DC
        HFONT hFontOld = (HFONT)SelectObject(hdcTemp, hFont);

        // Save off the font metrics for various other calculations
        TEXTMETRICW tm;
        GetTextMetricsW(hdcTemp, &tm);

        // Now find the size of a 0 in this current font and save it for conversions done later.
        SIZE sz;
        GetTextExtentPoint32W(hdcTemp, L"0", 1, &sz);

        COORD coordFont;
        coordFont.X = static_cast<SHORT>(sz.cx);
        coordFont.Y = static_cast<SHORT>(sz.cy);

        // The extent point won't necessarily be perfect for the width, so get the ABC metrics for the 0 if possible to improve the measurement.
        // This will fail for non-TrueType fonts and we'll fall back to what GetTextExtentPoint said. 
        {
            ABC abc;
            if (0 != GetCharABCWidthsW(hdcTemp, '0', '0', &abc))
            {
                int const abcTotal = abc.abcA + abc.abcB + abc.abcC;

                // No negatives or zeros or we'll have bad character-to-pixel math later.
                if (abcTotal > 0)
                {
                    coordFont.X = static_cast<SHORT>(abcTotal);
                }
            }
        }
        
        
        // Now fill up the FontInfo we were passed with the full details of which font we actually chose
        {
            // Get the actual font face that we chose
            WCHAR pwszFaceName[LF_FACESIZE];
            GetTextFaceW(hdcTemp, ARRAYSIZE(pwszFaceName), pwszFaceName);

            if (coordFontRequested.X == 0)
            {
                coordFontRequested.X = (SHORT)s_ShrinkByDpi(coordFont.X, iDpi);
            }

            pfiFont->SetFromEngine(pwszFaceName,
                                   tm.tmPitchAndFamily,
                                   tm.tmWeight,
                                   coordFont,
                                   coordFontRequested);
        }

        if (phFont != nullptr)
        {
            // If someone wanted the font handle, give it back.
            *phFont = hFont;
        }
        else
        {
            // Otherwise free it.

            // Select the original font back into the DC first or we might not be able to free this one.
            SelectObject(hdcTemp, hFontOld);

            DeleteObject(hFont);
            hFont = nullptr;
        }

        DeleteDC(hdcTemp);
    }
}

// Routine Description:
// - Retrieves the current pixel size of the font we have selected for drawing.
// Arguments:
// - <none>
// Return Value:
// - X by Y size of the font.
COORD GdiEngine::GetFontSize()
{
    return _GetFontSize();
}

// Routine Description:
// - Retrieves the current pixel size of the font we have selected for drawing.
// Arguments:
// - <none>
// Return Value:
// - X by Y size of the font.
COORD GdiEngine::_GetFontSize() const
{
    return _coordFontLast;
}

// Routine Description:
// - Retrieves whether or not the window is currently minimized.
// Arguments:
// - <none>
// Return Value:
// - True if minimized (don't need to draw anything). False otherwise.
bool GdiEngine::_IsMinimized() const
{
    return !!IsIconic(_hwndTargetWindow);
}

// Routine Description:
// - Determines whether or not we have a TrueType font selected.
// - Intended only for determining whether we need to perform special raster font scaling.
// Arguments:
// - <none>
// Return Value:
// - True if TrueType. False otherwise (and generally assumed to be raster font type.)
bool GdiEngine::_IsFontTrueType() const
{
    return !!(_tmFontMetrics.tmPitchAndFamily & TMPF_TRUETYPE);
}
