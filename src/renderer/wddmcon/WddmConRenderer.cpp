#include "precomp.h"

#include "WddmConRenderer.hpp"

#pragma hdrstop

//
// Default non-bright white.
//

#define DEFAULT_COLOR_ATTRIBUTE  (0xC)

#define DEFAULT_FONT_WIDTH       (8)
#define DEFAULT_FONT_HEIGHT      (12)

using namespace Microsoft::Console::Render;

WddmConEngine::WddmConEngine()
    : _hWddmConCtx(INVALID_HANDLE_VALUE),
    _displayHeight(0),
    _displayWidth(0),
    _displayState(nullptr),
    _currentLegacyColorAttribute(DEFAULT_COLOR_ATTRIBUTE)
{

}

void WddmConEngine::FreeResources(ULONG displayHeight)
{
    if (_displayState)
    {
        for (ULONG i = 0; i < displayHeight; i++)
        {
            if (_displayState[i])
            {
                if (_displayState[i]->Old)
                {
                    free(_displayState[i]->Old);
                }
                if (_displayState[i]->New)
                {
                    free(_displayState[i]->New);
                }

                free(_displayState[i]);
            }
        }

        free(_displayState);
    }

    if (_hWddmConCtx != INVALID_HANDLE_VALUE)
    {
        WDDMConDestroy(_hWddmConCtx);
        _hWddmConCtx = INVALID_HANDLE_VALUE;
    }
}

WddmConEngine::~WddmConEngine()
{
    FreeResources(_displayHeight);
}

HRESULT WddmConEngine::Initialize()
{
    HRESULT hr;
    RECT DisplaySize;
    CD_IO_DISPLAY_SIZE DisplaySizeIoctl;

    if (_hWddmConCtx == INVALID_HANDLE_VALUE)
    {
        hr = WDDMConCreate(&_hWddmConCtx);

        if (SUCCEEDED(hr))
        {
            hr = WDDMConGetDisplaySize(_hWddmConCtx, &DisplaySizeIoctl);

            if (SUCCEEDED(hr))
            {
                DisplaySize.top = 0;
                DisplaySize.left = 0;
                DisplaySize.bottom = (LONG)DisplaySizeIoctl.Height;
                DisplaySize.right = (LONG)DisplaySizeIoctl.Width;

                _displayState = (PCD_IO_ROW_INFORMATION *)calloc(DisplaySize.bottom, sizeof(PCD_IO_ROW_INFORMATION));

                if (_displayState != nullptr)
                {
                    for (LONG i = 0; i < DisplaySize.bottom; i++)
                    {
                        _displayState[i] = (PCD_IO_ROW_INFORMATION)calloc(1, sizeof(CD_IO_ROW_INFORMATION));
                        if (_displayState[i] == nullptr)
                        {
                            hr = E_OUTOFMEMORY;

                            break;
                        }

                        _displayState[i]->Index = (SHORT)i;
                        _displayState[i]->Old = (PCD_IO_CHARACTER)calloc(DisplaySize.right, sizeof(CD_IO_CHARACTER));
                        _displayState[i]->New = (PCD_IO_CHARACTER)calloc(DisplaySize.right, sizeof(CD_IO_CHARACTER));

                        if (_displayState[i]->Old == nullptr || _displayState[i]->New == nullptr)
                        {
                            hr = E_OUTOFMEMORY;

                            break;
                        }
                    }

                    if (SUCCEEDED(hr))
                    {
                        _displayHeight = DisplaySize.bottom;
                        _displayWidth = DisplaySize.right;
                    }
                    else
                    {
                        FreeResources(DisplaySize.bottom);
                    }
                }
                else
                {
                    WDDMConDestroy(_hWddmConCtx);

                    hr = E_OUTOFMEMORY;
                }
            }
            else
            {
                WDDMConDestroy(_hWddmConCtx);
            }
        }
    }
    else
    {
        hr = E_HANDLE;
    }

    return hr;
}

bool WddmConEngine::IsInitialized()
{
    return _hWddmConCtx != INVALID_HANDLE_VALUE;
}

HRESULT WddmConEngine::Enable()
{
    RETURN_IF_HANDLE_INVALID(_hWddmConCtx);
    return WDDMConEnableDisplayAccess((PHANDLE)_hWddmConCtx, TRUE);
}

HRESULT WddmConEngine::Disable()
{
    RETURN_IF_HANDLE_INVALID(_hWddmConCtx);
    return WDDMConEnableDisplayAccess((PHANDLE)_hWddmConCtx, FALSE);
}

HRESULT WddmConEngine::Invalidate(const SMALL_RECT* const psrRegion)
{
    UNREFERENCED_PARAMETER(psrRegion);

    return S_OK;
}

HRESULT WddmConEngine::InvalidateSystem(const RECT* const prcDirtyClient)
{
    UNREFERENCED_PARAMETER(prcDirtyClient);

    return S_OK;
}

HRESULT WddmConEngine::InvalidateSelection(const SMALL_RECT* const rgsrSelection, UINT const cRectangles)
{
    UNREFERENCED_PARAMETER(rgsrSelection);
    UNREFERENCED_PARAMETER(cRectangles);

    return S_OK;
}

HRESULT WddmConEngine::InvalidateScroll(const COORD* const pcoordDelta)
{
    UNREFERENCED_PARAMETER(pcoordDelta);

    return S_OK;
}

HRESULT WddmConEngine::InvalidateAll()
{
    return S_OK;
}

HRESULT WddmConEngine::InvalidateCircling(_Out_ bool* const pForcePaint)
{
    *pForcePaint = false;
    return S_FALSE;
}

HRESULT WddmConEngine::StartPaint()
{
    RETURN_IF_HANDLE_INVALID(_hWddmConCtx);
    return WDDMConBeginUpdateDisplayBatch(_hWddmConCtx);
}

HRESULT WddmConEngine::EndPaint()
{
    RETURN_IF_HANDLE_INVALID(_hWddmConCtx);
    return WDDMConEndUpdateDisplayBatch(_hWddmConCtx);
}

HRESULT WddmConEngine::ScrollFrame()
{
    return S_OK;
}

HRESULT WddmConEngine::PaintBackground()
{
    RETURN_IF_HANDLE_INVALID(_hWddmConCtx);

    PCD_IO_CHARACTER OldChar;
    PCD_IO_CHARACTER NewChar;

    for (LONG rowIndex = 0; rowIndex < _displayHeight; rowIndex++)
    {
        for (LONG colIndex = 0; colIndex < _displayWidth; colIndex++)
        {
            OldChar = &_displayState[rowIndex]->Old[colIndex];
            NewChar = &_displayState[rowIndex]->New[colIndex];

            OldChar->Character = NewChar->Character;
            OldChar->Atribute = NewChar->Atribute;

            NewChar->Character = L' ';
            NewChar->Atribute = 0x0;
        }
    }


    return S_OK;
}

HRESULT WddmConEngine::PaintBufferLine(PCWCHAR const pwsLine, const unsigned char* const rgWidths, size_t const cchLine, COORD const coord, bool const fTrimLeft)
{
    UNREFERENCED_PARAMETER(rgWidths);
    UNREFERENCED_PARAMETER(fTrimLeft);

    RETURN_IF_HANDLE_INVALID(_hWddmConCtx);

    PCD_IO_CHARACTER OldChar;
    PCD_IO_CHARACTER NewChar;

    for (size_t i = 0; i < cchLine && i < (size_t)_displayWidth; i++)
    {
        OldChar = &_displayState[coord.Y]->Old[coord.X + i];
        NewChar = &_displayState[coord.Y]->New[coord.X + i];

        OldChar->Character = NewChar->Character;
        OldChar->Atribute = NewChar->Atribute;

        NewChar->Character = pwsLine[i];
        NewChar->Atribute = _currentLegacyColorAttribute;
    }

    return WDDMConUpdateDisplay(_hWddmConCtx, _displayState[coord.Y], FALSE);
}

HRESULT WddmConEngine::PaintBufferGridLines(GridLines const lines, COLORREF const color, size_t const cchLine, COORD const coordTarget)
{
    UNREFERENCED_PARAMETER(lines);
    UNREFERENCED_PARAMETER(color);
    UNREFERENCED_PARAMETER(cchLine);
    UNREFERENCED_PARAMETER(coordTarget);

    return S_OK;
}

HRESULT WddmConEngine::PaintSelection(const SMALL_RECT* const rgsrSelection, UINT const cRectangles)
{
    UNREFERENCED_PARAMETER(rgsrSelection);
    UNREFERENCED_PARAMETER(cRectangles);

    return S_OK;
}

HRESULT WddmConEngine::PaintCursor(COORD const /*coordCursor*/, ULONG const /*ulCursorHeightPercent*/, bool const /*fIsDoubleWidth*/)
{
    return S_OK;
}

HRESULT WddmConEngine::ClearCursor()
{
    return S_OK;
}

HRESULT WddmConEngine::UpdateDrawingBrushes(COLORREF const colorForeground, COLORREF const colorBackground, _In_ WORD const legacyColorAttribute, bool const fIncludeBackgrounds)
{
    UNREFERENCED_PARAMETER(colorForeground);
    UNREFERENCED_PARAMETER(colorBackground);
    UNREFERENCED_PARAMETER(fIncludeBackgrounds);

    _currentLegacyColorAttribute = legacyColorAttribute;

    return S_OK;
}

HRESULT WddmConEngine::UpdateFont(FontInfoDesired const* const pfiFontInfoDesired, FontInfo* const pfiFontInfo)
{
    UNREFERENCED_PARAMETER(pfiFontInfoDesired);

    COORD coordSize = {0};
    GetFontSize(&coordSize);

    pfiFontInfo->SetFromEngine(pfiFontInfo->GetFaceName(),
                               pfiFontInfo->GetFamily(),
                               pfiFontInfo->GetWeight(),
                               pfiFontInfo->IsTrueTypeFont(),
                               coordSize,
                               coordSize);

    return S_OK;
}

HRESULT WddmConEngine::UpdateDpi(int const iDpi)
{
    UNREFERENCED_PARAMETER(iDpi);

    return S_OK;
}
              
// Method Description:
// - This method will update our internal reference for how big the viewport is.
//      Does nothing for WDDMCon.
// Arguments:
// - srNewViewport - The bounds of the new viewport.
// Return Value:
// - HRESULT S_OK  
HRESULT WddmConEngine::UpdateViewport(_In_ SMALL_RECT const /*srNewViewport*/)
{
    return S_OK;
}

HRESULT WddmConEngine::GetProposedFont(FontInfoDesired const* const pfiFontInfoDesired, FontInfo* const pfiFontInfo, int const iDpi)
{
    UNREFERENCED_PARAMETER(pfiFontInfoDesired);
    UNREFERENCED_PARAMETER(pfiFontInfo);
    UNREFERENCED_PARAMETER(iDpi);

    return S_OK;
}

SMALL_RECT WddmConEngine::GetDirtyRectInChars()
{
    SMALL_RECT r;
    r.Bottom = _displayHeight > 0 ? (SHORT)(_displayHeight - 1) : 0;
    r.Top = 0;
    r.Left = 0;
    r.Right = _displayWidth > 0 ? (SHORT)(_displayWidth - 1) : 0;

    return r;
}

RECT WddmConEngine::GetDisplaySize()
{
    RECT r;
    r.top = 0;
    r.left = 0;
    r.bottom = _displayHeight;
    r.right = _displayWidth;

    return r;
}

HRESULT WddmConEngine::GetFontSize(_Out_ COORD* const pFontSize)
{
    // In order to retrieve the font size being used by DirectX, it is necessary
    // to modify the API set that defines the contract for WddmCon. However, the
    // intention is to subsume WddmCon into ConhostV2 directly once the issue of
    // building in the OneCore 'depot' including DirectX headers and libs is
    // resolved. The font size has no bearing on the behavior of the console
    // since it is used to determine the invalid rectangle whenever the console
    // buffer changes. However, given that no invalidation logic exists for this
    // renderer, the value returned by this function is irrelevant.
    //
    // TODO: MSFT 11851921 - Subsume WddmCon into ConhostV2 and remove the API
    //       set extension.
    COORD c;
    c.X = DEFAULT_FONT_WIDTH;
    c.Y = DEFAULT_FONT_HEIGHT;

    *pFontSize = c;
    return S_OK;
}

HRESULT WddmConEngine::IsCharFullWidthByFont(WCHAR const /*wch*/, _Out_ bool* const pResult)
{
    *pResult = false;
    return S_OK;
}
