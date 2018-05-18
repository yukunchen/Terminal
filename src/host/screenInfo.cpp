/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "screenInfo.hpp"
#include "dbcs.h"
#include "output.h"
#include "_output.h"
#include "misc.h"
#include "../buffer/out/CharRow.hpp"

#include <math.h>

#include "../interactivity/inc/ServiceLocator.hpp"
#include "../types/inc/Viewport.hpp"
#include "../terminal/parser/OutputStateMachineEngine.hpp"

#include "../types/inc/convert.hpp"
#include "../types/inc/RepeatingIterator.hpp"

#pragma hdrstop
using namespace Microsoft::Console;
using namespace Microsoft::Console::Types;

#pragma region Construct/Destruct

SCREEN_INFORMATION::SCREEN_INFORMATION(
    _In_ IWindowMetrics *pMetrics,
    _In_ IAccessibilityNotifier *pNotifier,
    const CHAR_INFO ciFill,
    const CHAR_INFO ciPopupFill) :
    Header{ },
    OutputMode{ ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT },
    ResizingWindow{ 0 },
    WheelDelta{ 0 },
    HWheelDelta{ 0 },
    _textBuffer{ nullptr },
    Next{ nullptr },
    WriteConsoleDbcsLeadByte{ 0, 0 },
    FillOutDbcsLeadChar{ 0 },
    // LineChar initialized below.
    ConvScreenInfo{ nullptr },
    ScrollScale{ 1ul },
    _pConsoleWindowMetrics{ pMetrics },
    _pAccessibilityNotifier{ pNotifier },
    _pConApi{ nullptr },
    _pBufferWriter{ nullptr },
    _pAdapter{ nullptr },
    _pStateMachine{ nullptr },
    _pEngine{ nullptr },
    _coordScreenBufferSize{ 0 },
    _srScrollMargins{ 0 },
    _viewport({ 0 }),
    _psiAlternateBuffer{ nullptr },
    _psiMainBuffer{ nullptr },
    _rcAltSavedClientNew{ 0 },
    _rcAltSavedClientOld{ 0 },
    _fAltWindowChanged{ false },
    _ptsTabs{ nullptr },
    _Attributes{ ciFill.Attributes },
    _PopupAttributes{ ciPopupFill.Attributes }
{
    LineChar[0] = UNICODE_BOX_DRAW_LIGHT_DOWN_AND_RIGHT;
    LineChar[1] = UNICODE_BOX_DRAW_LIGHT_DOWN_AND_LEFT;
    LineChar[2] = UNICODE_BOX_DRAW_LIGHT_HORIZONTAL;
    LineChar[3] = UNICODE_BOX_DRAW_LIGHT_VERTICAL;
    LineChar[4] = UNICODE_BOX_DRAW_LIGHT_UP_AND_RIGHT;
    LineChar[5] = UNICODE_BOX_DRAW_LIGHT_UP_AND_LEFT;
    const CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    if (gci.GetVirtTermLevel() != 0)
    {
        OutputMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    }
}

// Routine Description:
// - This routine frees the memory associated with a screen buffer.
// Arguments:
// - ScreenInfo - screen buffer data to free.
// Return Value:
// Note:
// - console handle table lock must be held when calling this routine
SCREEN_INFORMATION::~SCREEN_INFORMATION()
{
    _FreeOutputStateMachine();
    ClearTabStops();
}

// Routine Description:
// - This routine allocates and initializes the data associated with a screen buffer.
// Arguments:
// - ScreenInformation - the new screen buffer.
// - coordWindowSize - the initial size of screen buffer's window (in rows/columns)
// - nFont - the initial font to generate text with.
// - dwScreenBufferSize - the initial size of the screen buffer (in rows/columns).
// Return Value:
[[nodiscard]]
NTSTATUS SCREEN_INFORMATION::CreateInstance(_In_ COORD coordWindowSize,
                                            const FontInfo fontInfo,
                                            _In_ COORD coordScreenBufferSize,
                                            const CHAR_INFO ciFill,
                                            const CHAR_INFO ciPopupFill,
                                            const UINT uiCursorSize,
                                            _Outptr_ SCREEN_INFORMATION** const ppScreen)
{
    *ppScreen = nullptr;

    try
    {
        IWindowMetrics *pMetrics = ServiceLocator::LocateWindowMetrics();
        THROW_IF_NULL_ALLOC(pMetrics);

        IAccessibilityNotifier *pNotifier = ServiceLocator::LocateAccessibilityNotifier();
        THROW_IF_NULL_ALLOC(pNotifier);

        PSCREEN_INFORMATION const pScreen = new SCREEN_INFORMATION(pMetrics, pNotifier, ciFill, ciPopupFill);

        pScreen->_InitializeBufferDimensions(coordScreenBufferSize, coordWindowSize);

        pScreen->_textBuffer = std::make_unique<TextBuffer>(fontInfo,
                                                            pScreen->GetScreenBufferSize(),
                                                            ciFill,
                                                            uiCursorSize);
        const NTSTATUS status = pScreen->_InitializeOutputStateMachine();

        if (NT_SUCCESS(status))
        {
            *ppScreen = pScreen;
        }

        LOG_IF_NTSTATUS_FAILED(status);
        return status;
    }
    catch (...)
    {
        return NTSTATUS_FROM_HRESULT(wil::ResultFromCaughtException());
    }
}

void SCREEN_INFORMATION::SetScreenBufferSize(const COORD coordNewBufferSize)
{
    COORD coordCandidate;
    coordCandidate.X = std::max(1i16, coordNewBufferSize.X);
    coordCandidate.Y = std::max(1i16, coordNewBufferSize.Y);
    _coordScreenBufferSize = coordCandidate;
}

COORD SCREEN_INFORMATION::GetScreenBufferSize() const
{
    return _coordScreenBufferSize;
}

WriteBuffer* SCREEN_INFORMATION::GetBufferWriter() const
{
    return _pBufferWriter;
}

AdaptDispatch* SCREEN_INFORMATION::GetAdapterDispatch() const
{
    return _pAdapter;
}

StateMachine* SCREEN_INFORMATION::GetStateMachine() const
{
    return _pStateMachine;
}

// Method Description:
// - returns true if this buffer is in Virtual Terminal Output mode.
// Arguments:
// <none>
// Return Value:
// true iff this buffer is in Virtual Terminal Output mode.
bool SCREEN_INFORMATION::InVTMode() const
{
    return IsFlagSet(OutputMode, ENABLE_VIRTUAL_TERMINAL_PROCESSING);
}

// Routine Description:
// - This routine inserts the screen buffer pointer into the console's list of screen buffers.
// Arguments:
// - ScreenInfo - Pointer to screen information structure.
// Return Value:
// Note:
// - The console lock must be held when calling this routine.
void SCREEN_INFORMATION::s_InsertScreenBuffer(_In_ PSCREEN_INFORMATION pScreenInfo)
{
    CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    FAIL_FAST_IF_FALSE(gci.IsConsoleLocked());

    pScreenInfo->Next = gci.ScreenBuffers;
    gci.ScreenBuffers = pScreenInfo;
}

// Routine Description:
// - This routine removes the screen buffer pointer from the console's list of screen buffers.
// Arguments:
// - ScreenInfo - Pointer to screen information structure.
// Return Value:
// Note:
// - The console lock must be held when calling this routine.
void SCREEN_INFORMATION::s_RemoveScreenBuffer(_In_ SCREEN_INFORMATION* const pScreenInfo)
{
    CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    if (pScreenInfo == gci.ScreenBuffers)
    {
        gci.ScreenBuffers = pScreenInfo->Next;
    }
    else
    {
        PSCREEN_INFORMATION Cur = gci.ScreenBuffers;
        PSCREEN_INFORMATION Prev = Cur;
        while (Cur != nullptr)
        {
            if (pScreenInfo == Cur)
            {
                break;
            }

            Prev = Cur;
            Cur = Cur->Next;
        }

        FAIL_FAST_IF_NULL(Cur);
        Prev->Next = Cur->Next;
    }

    if (pScreenInfo == gci.pCurrentScreenBuffer &&
        gci.ScreenBuffers != gci.pCurrentScreenBuffer)
    {
        if (gci.ScreenBuffers != nullptr)
        {
            SetActiveScreenBuffer(*gci.ScreenBuffers);
        }
        else
        {
            gci.pCurrentScreenBuffer = nullptr;
        }
    }

    delete pScreenInfo;
}

#pragma endregion

#pragma region Output State Machine

[[nodiscard]]
NTSTATUS SCREEN_INFORMATION::_InitializeOutputStateMachine()
{
    CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    try
    {
        _pConApi = new ConhostInternalGetSet(gci);

        FAIL_FAST_IF(_pBufferWriter != nullptr);
        _pBufferWriter = new WriteBuffer(gci);

        FAIL_FAST_IF(_pAdapter != nullptr);
        _pAdapter = new AdaptDispatch(_pConApi, _pBufferWriter, _Attributes.GetLegacyAttributes());

        // Note that at this point in the setup, we haven't determined if we're
        //      in VtIo mode or not yet. We'll set the OutputStateMachine's
        //      TerminalConnection later, in VtIo::StartIfNeeded
        _pEngine = std::make_shared<OutputStateMachineEngine>(_pAdapter);

        FAIL_FAST_IF(_pStateMachine != nullptr);
        _pStateMachine = new StateMachine(_pEngine);
    }
    catch (...)
    {
        // if any part of initialization failed, free the allocated ones.
        _FreeOutputStateMachine();

        return NTSTATUS_FROM_HRESULT(wil::ResultFromCaughtException());
    }

    return STATUS_SUCCESS;
}

// If we're an alternate buffer, we want to give the GetSet back to our main
void SCREEN_INFORMATION::_FreeOutputStateMachine()
{
    if (_psiMainBuffer == nullptr) // If this is a main buffer
    {
        if (_psiAlternateBuffer != nullptr)
        {
            s_RemoveScreenBuffer(_psiAlternateBuffer);
        }

        if (_pStateMachine != nullptr)
        {
            delete _pStateMachine;
        }

        _pEngine.reset();

        if (_pAdapter != nullptr)
        {
            delete _pAdapter;
        }

        if (_pBufferWriter != nullptr)
        {
            delete _pBufferWriter;
        }

        if (_pConApi != nullptr)
        {
            delete _pConApi;
        }
    }
}
#pragma endregion

#pragma region Get Data

bool SCREEN_INFORMATION::IsActiveScreenBuffer() const
{
    // the following macro returns TRUE if the given screen buffer is the active screen buffer.

    //#define ACTIVE_SCREEN_BUFFER(SCREEN_INFO) (gci.CurrentScreenBuffer == SCREEN_INFO)
    const CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    return (gci.pCurrentScreenBuffer == this);
}

// Routine Description:
// - This routine returns data about the screen buffer.
// Arguments:
// - Size - Pointer to location in which to store screen buffer size.
// - CursorPosition - Pointer to location in which to store the cursor position.
// - ScrollPosition - Pointer to location in which to store the scroll position.
// - Attributes - Pointer to location in which to store the default attributes.
// - CurrentWindowSize - Pointer to location in which to store current window size.
// - MaximumWindowSize - Pointer to location in which to store maximum window size.
// Return Value:
// - None
void SCREEN_INFORMATION::GetScreenBufferInformation(_Out_ PCOORD pcoordSize,
                                                    _Out_ PCOORD pcoordCursorPosition,
                                                    _Out_ PSMALL_RECT psrWindow,
                                                    _Out_ PWORD pwAttributes,
                                                    _Out_ PCOORD pcoordMaximumWindowSize,
                                                    _Out_ PWORD pwPopupAttributes,
                                                    _Out_writes_(COLOR_TABLE_SIZE) LPCOLORREF lpColorTable) const
{
    const CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    *pcoordSize = GetScreenBufferSize();

    *pcoordCursorPosition = _textBuffer->GetCursor().GetPosition();

    *psrWindow = _viewport.ToInclusive();

    *pwAttributes = _Attributes.GetLegacyAttributes();
    *pwPopupAttributes = _PopupAttributes.GetLegacyAttributes();

    // the copy length must be constant for now to keep OACR happy with buffer overruns.
    memmove(lpColorTable, gci.GetColorTable(), COLOR_TABLE_SIZE * sizeof(COLORREF));

    *pcoordMaximumWindowSize = GetMaxWindowSizeInCharacters();
}

// Routine Description:
// - Gets the smallest possible client area in characters. Takes the window client area and divides by the active font dimensions.
// Arguments:
// - coordFontSize - The font size to use for calculation if a screen buffer is not yet attached.
// Return Value:
// - COORD containing the width and height representing the minimum character grid that can be rendered in the window.
COORD SCREEN_INFORMATION::GetMinWindowSizeInCharacters(const COORD coordFontSize /*= { 1, 1 }*/) const
{
    FAIL_FAST_IF(coordFontSize.X == 0);
    FAIL_FAST_IF(coordFontSize.Y == 0);

    // prepare rectangle
    RECT const rcWindowInPixels = _pConsoleWindowMetrics->GetMinClientRectInPixels();

    // assign the pixel widths and heights to the final output
    COORD coordClientAreaSize;
    coordClientAreaSize.X = (SHORT)RECT_WIDTH(&rcWindowInPixels);
    coordClientAreaSize.Y = (SHORT)RECT_HEIGHT(&rcWindowInPixels);

    // now retrieve the font size and divide the pixel counts into character counts
    COORD coordFont = coordFontSize; // by default, use the size we were given

    // If text info has been set up, instead retrieve its font size
    if (_textBuffer != nullptr)
    {
        coordFont = GetScreenFontSize();
    }

    FAIL_FAST_IF(coordFont.X == 0);
    FAIL_FAST_IF(coordFont.Y == 0);

    coordClientAreaSize.X /= coordFont.X;
    coordClientAreaSize.Y /= coordFont.Y;

    return coordClientAreaSize;
}

// Routine Description:
// - Gets the maximum client area in characters that would fit on the current monitor or given the current buffer size.
//   Takes the monitor work area and divides by the active font dimensions then limits by buffer size.
// Arguments:
// - coordFontSize - The font size to use for calculation if a screen buffer is not yet attached.
// Return Value:
// - COORD containing the width and height representing the largest character grid that can be rendered on the current monitor and/or from the current buffer size.
COORD SCREEN_INFORMATION::GetMaxWindowSizeInCharacters(const COORD coordFontSize /*= { 1, 1 }*/) const
{
    FAIL_FAST_IF(coordFontSize.X == 0);
    FAIL_FAST_IF(coordFontSize.Y == 0);

    const COORD coordScreenBufferSize = GetScreenBufferSize();
    COORD coordClientAreaSize = coordScreenBufferSize;

    //  Important re: headless consoles on onecore (for telnetd, etc.)
    // GetConsoleScreenBufferInfoEx hits this to get the max size of the display.
    // Because we're headless, we don't really care about the max size of the display.
    // In that case, we'll just return the buffer size as the "max" window size.
    if (!ServiceLocator::LocateGlobals().IsHeadless())
    {
        const COORD coordWindowRestrictedSize = GetLargestWindowSizeInCharacters(coordFontSize);
        // If the buffer is smaller than what the max window would allow, then the max client area can only be as big as the
        // buffer we have.
        coordClientAreaSize.X = std::min(coordScreenBufferSize.X, coordWindowRestrictedSize.X);
        coordClientAreaSize.Y = std::min(coordScreenBufferSize.Y, coordWindowRestrictedSize.Y);
    }

    return coordClientAreaSize;
}

// Routine Description:
// - Gets the largest possible client area in characters if the window were stretched as large as it could go.
// - Takes the window client area and divides by the active font dimensions.
// Arguments:
// - coordFontSize - The font size to use for calculation if a screen buffer is not yet attached.
// Return Value:
// - COORD containing the width and height representing the largest character grid that can be rendered on the current monitor with the maximum size window.
COORD SCREEN_INFORMATION::GetLargestWindowSizeInCharacters(const COORD coordFontSize /*= { 1, 1 }*/) const
{
    FAIL_FAST_IF(coordFontSize.X == 0);
    FAIL_FAST_IF(coordFontSize.Y == 0);

    RECT const rcClientInPixels = _pConsoleWindowMetrics->GetMaxClientRectInPixels();

    // first assign the pixel widths and heights to the final output
    COORD coordClientAreaSize;
    coordClientAreaSize.X = (SHORT)RECT_WIDTH(&rcClientInPixels);
    coordClientAreaSize.Y = (SHORT)RECT_HEIGHT(&rcClientInPixels);

    // now retrieve the font size and divide the pixel counts into character counts
    COORD coordFont = coordFontSize; // by default, use the size we were given

    // If renderer has been set up, instead retrieve its font size
    if (ServiceLocator::LocateGlobals().pRender != nullptr)
    {
        coordFont = GetScreenFontSize();
    }

    FAIL_FAST_IF(coordFont.X == 0);
    FAIL_FAST_IF(coordFont.Y == 0);

    coordClientAreaSize.X /= coordFont.X;
    coordClientAreaSize.Y /= coordFont.Y;

    return coordClientAreaSize;
}

COORD SCREEN_INFORMATION::GetScrollBarSizesInCharacters() const
{
    COORD coordFont = GetScreenFontSize();

    SHORT vScrollSize = ServiceLocator::LocateGlobals().sVerticalScrollSize;
    SHORT hScrollSize = ServiceLocator::LocateGlobals().sHorizontalScrollSize;

    COORD coordBarSizes;
    coordBarSizes.X = (vScrollSize / coordFont.X) + ((vScrollSize % coordFont.X) != 0 ? 1 : 0);
    coordBarSizes.Y = (hScrollSize / coordFont.Y) + ((hScrollSize % coordFont.Y) != 0 ? 1 : 0);

    return coordBarSizes;
}

void SCREEN_INFORMATION::GetRequiredConsoleSizeInPixels(_Out_ PSIZE const pRequiredSize) const
{
    COORD const coordFontSize = _textBuffer->GetCurrentFont().GetSize();

    // TODO: Assert valid size boundaries
    pRequiredSize->cx = GetScreenWindowSizeX() * coordFontSize.X;
    pRequiredSize->cy = GetScreenWindowSizeY() * coordFontSize.Y;
}

// Method Description:
// - Returns the width of the viewport, in characters.
// Arguments:
// - <none>
// Return Value:
// - the width of the viewport, in characters.
SHORT SCREEN_INFORMATION::GetScreenWindowSizeX() const
{
    return _viewport.Width();
}

// Method Description:
// - Returns the height of the viewport, in characters.
// Arguments:
// - <none>
// Return Value:
// - the height of the viewport, in characters.
SHORT SCREEN_INFORMATION::GetScreenWindowSizeY() const
{
    return _viewport.Height();
}

COORD SCREEN_INFORMATION::GetScreenFontSize() const
{
    // If we have no renderer, then we don't really need any sort of pixel math. so the "font size" for the scale factor
    // (which is used almost everywhere around the code as * and / calls) should just be 1,1 so those operations will do
    // effectively nothing.
    COORD coordRet = { 1, 1 };
    if (ServiceLocator::LocateGlobals().pRender != nullptr)
    {
        coordRet = ServiceLocator::LocateGlobals().pRender->GetFontSize();
    }

    // For sanity's sake, make sure not to leak 0 out as a possible value. These values are used in division operations.
    coordRet.X = std::max(coordRet.X, 1i16);
    coordRet.Y = std::max(coordRet.Y, 1i16);

    return coordRet;
}

#pragma endregion

#pragma region Set Data

void SCREEN_INFORMATION::RefreshFontWithRenderer()
{
    if (IsActiveScreenBuffer())
    {
        // Hand the handle to our internal structure to the font change trigger in case it updates it based on what's appropriate.
        if (ServiceLocator::LocateGlobals().pRender != nullptr)
        {
            ServiceLocator::LocateGlobals().pRender
                ->TriggerFontChange(ServiceLocator::LocateGlobals().dpi,
                                    _textBuffer->GetDesiredFont(),
                                    _textBuffer->GetCurrentFont());
        }
    }
}

void SCREEN_INFORMATION::UpdateFont(const FontInfo* const pfiNewFont)
{
    FontInfoDesired fiDesiredFont(*pfiNewFont);

    _textBuffer->GetDesiredFont() = fiDesiredFont;

    RefreshFontWithRenderer();

    // If we're the active screen buffer...
    if (IsActiveScreenBuffer())
    {
        // If there is a window attached, let it know that it should try to update so the rows/columns are now accounting for the new font.
        IConsoleWindow* const pWindow = ServiceLocator::LocateConsoleWindow();
        if (nullptr != pWindow)
        {
            COORD coordViewport;
            coordViewport.X = GetScreenWindowSizeX();
            coordViewport.Y = GetScreenWindowSizeY();
            pWindow->UpdateWindowSize(coordViewport);
        }
    }

    // If we're an alt buffer, also update our main buffer.
    if (_psiMainBuffer)
    {
        _psiMainBuffer->UpdateFont(pfiNewFont);
    }
}

// NOTE: This method was historically used to notify accessibility apps AND
// to aggregate drawing metadata to determine whether or not to use PolyTextOut.
// After the Nov 2015 graphics refactor, the metadata drawing flag calculation is no longer necessary.
// This now only notifies accessibility apps of a change.
void SCREEN_INFORMATION::NotifyAccessibilityEventing(const short sStartX,
                                                     const short sStartY,
                                                     const short sEndX,
                                                     const short sEndY)
{
    const CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();

    // Fire off a winevent to let accessibility apps know what changed.
    if (IsActiveScreenBuffer())
    {
        const COORD coordScreenBufferSize = GetScreenBufferSize();
        FAIL_FAST_IF_FALSE(sEndX < coordScreenBufferSize.X);

        if (sStartX == sEndX && sStartY == sEndY)
        {
            const auto RowIndex = (_textBuffer->GetFirstRowIndex() + sStartY) % coordScreenBufferSize.Y;

            try
            {
                const OutputCell cell = ReadLine(RowIndex, sStartX, 1).at(0);
                const LONG charAndAttr = MAKELONG(Utf16ToUcs2(cell.Chars()),
                                                  gci.GenerateLegacyAttributes(cell.TextAttr()));
                _pAccessibilityNotifier->NotifyConsoleUpdateSimpleEvent(MAKELONG(sStartX, sStartY),
                                                                        charAndAttr);
            }
            catch (...)
            {
                LOG_HR(wil::ResultFromCaughtException());
                return;
            }
        }
        else
        {
            _pAccessibilityNotifier->NotifyConsoleUpdateRegionEvent(MAKELONG(sStartX, sStartY),
                                                                    MAKELONG(sEndX, sEndY));
        }
        IConsoleWindow* pConsoleWindow = ServiceLocator::LocateConsoleWindow();
        if (pConsoleWindow)
        {
            LOG_IF_FAILED(pConsoleWindow->SignalUia(UIA_Text_TextChangedEventId));
            // TODO MSFT 7960168 do we really need this event to not signal?
            //pConsoleWindow->SignalUia(UIA_LayoutInvalidatedEventId);
        }
    }
}

#pragma endregion

#pragma region UI/Refresh

VOID SCREEN_INFORMATION::UpdateScrollBars()
{
    CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    if (!IsActiveScreenBuffer())
    {
        return;
    }

    if (gci.Flags & CONSOLE_UPDATING_SCROLL_BARS)
    {
        return;
    }

    gci.Flags |= CONSOLE_UPDATING_SCROLL_BARS;

    if (ServiceLocator::LocateConsoleWindow() != nullptr)
    {
        ServiceLocator::LocateConsoleWindow()->PostUpdateScrollBars();
    }
}

VOID SCREEN_INFORMATION::InternalUpdateScrollBars()
{
    CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    IConsoleWindow* const pWindow = ServiceLocator::LocateConsoleWindow();

    ClearFlag(gci.Flags, CONSOLE_UPDATING_SCROLL_BARS);

    if (!IsActiveScreenBuffer())
    {
        return;
    }

    ResizingWindow++;

    if (pWindow != nullptr)
    {
        const COORD coordScreenBufferSize = GetScreenBufferSize();

        // If this is the main buffer, make sure we enable both of the scroll bars.
        //      The alt buffer likely disabled the scroll bars, this is the only
        //      way to re-enable it.
        if(!_IsAltBuffer())
        {
            pWindow->EnableBothScrollBars();
        }

        pWindow->UpdateScrollBar(true,
                                 _IsAltBuffer(),
                                 GetScreenWindowSizeY(),
                                 coordScreenBufferSize.Y - 1,
                                 _viewport.Top());
        pWindow->UpdateScrollBar(false,
                                 _IsAltBuffer(),
                                 GetScreenWindowSizeX(),
                                 coordScreenBufferSize.X - 1,
                                 _viewport.Left());
    }

    // Fire off an event to let accessibility apps know the layout has changed.
    _pAccessibilityNotifier->NotifyConsoleLayoutEvent();

    ResizingWindow--;
}

// Routine Description:
// - Modifies the size of the current viewport to match the width/height of the request given.
// - This will act like a resize operation from the bottom right corner of the window.
// Arguments:
// - pcoordSize - Requested viewport width/heights in characters
// Return Value:
// - <none>
void SCREEN_INFORMATION::SetViewportSize(const COORD* const pcoordSize)
{
    // If this is the alt buffer or a VT I/O buffer:
    //      first resize ourselves to match the new viewport
    //      then also make sure that the main buffer gets the same call
    //      (if necessary)
    if (_IsInPtyMode())
    {
        LOG_IF_FAILED(ResizeScreenBuffer(*pcoordSize, TRUE));

        if (_psiMainBuffer)
        {
            _psiMainBuffer->SetViewportSize(&_coordScreenBufferSize);
        }
    }
    _InternalSetViewportSize(pcoordSize, false, false);
}

[[nodiscard]]
NTSTATUS SCREEN_INFORMATION::SetViewportOrigin(const bool fAbsolute,
                                               const COORD coordWindowOrigin)
{
    // calculate window size
    COORD WindowSize = _viewport.Dimensions();

    SMALL_RECT NewWindow;
    // if relative coordinates, figure out absolute coords.
    if (!fAbsolute)
    {
        if (coordWindowOrigin.X == 0 && coordWindowOrigin.Y == 0)
        {
            return STATUS_SUCCESS;
        }
        NewWindow.Left = _viewport.Left() + coordWindowOrigin.X;
        NewWindow.Top = _viewport.Top() + coordWindowOrigin.Y;
    }
    else
    {
        if (coordWindowOrigin == _viewport.Origin())
        {
            return STATUS_SUCCESS;
        }
        NewWindow.Left = coordWindowOrigin.X;
        NewWindow.Top = coordWindowOrigin.Y;
    }
    NewWindow.Right = (SHORT)(NewWindow.Left + WindowSize.X - 1);
    NewWindow.Bottom = (SHORT)(NewWindow.Top + WindowSize.Y - 1);

    // see if new window origin would extend window beyond extent of screen buffer
    const COORD coordScreenBufferSize = GetScreenBufferSize();
    if (NewWindow.Left < 0 ||
        NewWindow.Top < 0 ||
        NewWindow.Right < 0 ||
        NewWindow.Bottom < 0 ||
        NewWindow.Right >= coordScreenBufferSize.X ||
        NewWindow.Bottom >= coordScreenBufferSize.Y)
    {
        return STATUS_INVALID_PARAMETER;
    }

    if (IsActiveScreenBuffer() && ServiceLocator::LocateConsoleWindow() != nullptr)
    {
        // Tell the window that it needs to set itself to the new origin if we're the active buffer.
        LOG_IF_FAILED(ServiceLocator::LocateConsoleWindow()->SetViewportOrigin(NewWindow));
    }
    else
    {
        // Otherwise, just store the new position and go on.
        _viewport = Viewport::FromInclusive(NewWindow);
        Tracing::s_TraceWindowViewport(NewWindow);
    }

    return STATUS_SUCCESS;
}

// Routine Description:
// - This routine updates the size of the rectangle representing the viewport into the text buffer.
// - It is specified in character count within the buffer.
// - It will be corrected to not exceed the limits of the current screen buffer dimensions.
// Arguments:
// newViewport: The new viewport to use. If it's out of bounds in the negative
//      direction it will be shifted to positive coordinates. If it's bigger
//      that the screen buffer, it will be clamped to the size of the buffer.
// Return Value:
// - None
void SCREEN_INFORMATION::SetViewportRect(const Viewport newViewport)
{
    // make sure there's something to do
    if (newViewport == _viewport)
    {
        return;
    }

    // do adjustments on a copy that's easily manipulated.
    SMALL_RECT srCorrected = newViewport.ToInclusive();

    if (srCorrected.Left < 0)
    {
        srCorrected.Right -= srCorrected.Left;
        srCorrected.Left = 0;
    }
    if (srCorrected.Top < 0)
    {
        srCorrected.Bottom -= srCorrected.Top;
        srCorrected.Top = 0;
    }

    const COORD coordScreenBufferSize = GetScreenBufferSize();
    if (srCorrected.Right >= coordScreenBufferSize.X)
    {
        srCorrected.Right = coordScreenBufferSize.X;
    }
    if (srCorrected.Bottom >= coordScreenBufferSize.Y)
    {
        srCorrected.Bottom = coordScreenBufferSize.Y;
    }

    _viewport = Viewport::FromInclusive(srCorrected);
    Tracing::s_TraceWindowViewport(srCorrected);
}

bool SCREEN_INFORMATION::SendNotifyBeep() const
{
    if (IsActiveScreenBuffer())
    {
        if (ServiceLocator::LocateConsoleWindow() != nullptr)
        {
            return ServiceLocator::LocateConsoleWindow()->SendNotifyBeep();
        }
    }

    return false;
}

bool SCREEN_INFORMATION::PostUpdateWindowSize() const
{
    if (IsActiveScreenBuffer())
    {
        if (ServiceLocator::LocateConsoleWindow() != nullptr)
        {
            return ServiceLocator::LocateConsoleWindow()->PostUpdateWindowSize();
        }
    }

    return false;
}

// Routine Description:
// - Modifies the screen buffer and viewport dimensions when the available client area rendering space changes.
// Arguments:
// - prcClientNew - Client rectangle in pixels after this update
// - prcClientOld - Client rectangle in pixels before this update
// Return Value:
// - <none>
void SCREEN_INFORMATION::ProcessResizeWindow(const RECT* const prcClientNew, const RECT* const prcClientOld)
{
    if (_IsAltBuffer())
    {
        // Stash away the size of the window, we'll need to do this to the main when we pop back
        //  We set this on the main, so that main->alt(resize)->alt keeps the resize
        _psiMainBuffer->_fAltWindowChanged = true;
        _psiMainBuffer->_rcAltSavedClientNew = *prcClientNew;
        _psiMainBuffer->_rcAltSavedClientOld = *prcClientOld;
    }

    // 1. In some modes, the screen buffer size needs to change on window size, so do that first.
    LOG_IF_FAILED(_AdjustScreenBuffer(prcClientNew));

    // 2. Now calculate how large the new viewport should be
    COORD coordViewportSize;
    _CalculateViewportSize(prcClientNew, &coordViewportSize);

    // 3. And adjust the existing viewport to match the same dimensions. The old/new comparison is to figure out which side the window was resized from.
    _AdjustViewportSize(prcClientNew, prcClientOld, &coordViewportSize);

    // 4. Finally, update the scroll bars.
    UpdateScrollBars();

    FAIL_FAST_IF_FALSE(_viewport.Top() >= 0);
    FAIL_FAST_IF_FALSE(_viewport.IsValid());
}

#pragma endregion

#pragma region Support/Calculation

// Routine Description:
// - This helper converts client pixel areas into the number of characters that could fit into the client window.
// - It requires the buffer size to figure out whether it needs to reserve space for the scroll bars (or not).
// Arguments:
// - prcClientNew - Client region of window in pixels
// - coordBufferOld - Size of backing buffer in characters
// - pcoordClientNewCharacters - The maximum number of characters X by Y that can be displayed in the window with the given backing buffer.
// Return Value:
// - S_OK if math was successful. Check with SUCCEEDED/FAILED macro.
[[nodiscard]]
HRESULT SCREEN_INFORMATION::_AdjustScreenBufferHelper(const RECT* const prcClientNew,
                                                      const COORD coordBufferOld,
                                                      _Out_ COORD* const pcoordClientNewCharacters)
{
    // Get the font size ready.
    COORD const coordFontSize = GetScreenFontSize();

    // We cannot operate if the font size is 0. This shouldn't happen, but stop early if it does.
    RETURN_HR_IF(E_NOT_VALID_STATE, 0 == coordFontSize.X || 0 == coordFontSize.Y);

    // Find out how much client space we have to work with in the new area.
    SIZE sizeClientNewPixels = { 0 };
    sizeClientNewPixels.cx = RECT_WIDTH(prcClientNew);
    sizeClientNewPixels.cy = RECT_HEIGHT(prcClientNew);

    // Subtract out scroll bar space if scroll bars will be necessary.
    bool fIsHorizontalVisible = false;
    bool fIsVerticalVisible = false;
    s_CalculateScrollbarVisibility(prcClientNew, &coordBufferOld, &coordFontSize, &fIsHorizontalVisible, &fIsVerticalVisible);

    if (fIsHorizontalVisible)
    {
        sizeClientNewPixels.cy -= ServiceLocator::LocateGlobals().sHorizontalScrollSize;
    }
    if (fIsVerticalVisible)
    {
        sizeClientNewPixels.cx -= ServiceLocator::LocateGlobals().sVerticalScrollSize;
    }

    // Now with the scroll bars removed, calculate how many characters could fit into the new window area.
    pcoordClientNewCharacters->X = (SHORT)(sizeClientNewPixels.cx / coordFontSize.X);
    pcoordClientNewCharacters->Y = (SHORT)(sizeClientNewPixels.cy / coordFontSize.Y);

    return S_OK;
}

// Routine Description:
// - Modifies the size of the backing text buffer when the window changes to support "intuitive" resizing modes by grabbing the window edges.
// - This function will compensate for scroll bars.
// - Buffer size changes will happen internally to this function.
// Arguments:
// - prcClientNew - Client rectangle in pixels after this update
// Return Value:
// - appropriate HRESULT
[[nodiscard]]
HRESULT SCREEN_INFORMATION::_AdjustScreenBuffer(const RECT* const prcClientNew)
{
    const CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    // Prepare the buffer sizes.
    // We need the main's size here to maintain the right scrollbar visibility.
    COORD const coordBufferSizeOld = _IsAltBuffer() ? _psiMainBuffer->GetScreenBufferSize() : GetScreenBufferSize();
    COORD coordBufferSizeNew = coordBufferSizeOld;

    // First figure out how many characters we could fit into the new window given the old buffer size
    COORD coordClientNewCharacters;

    RETURN_IF_FAILED(_AdjustScreenBufferHelper(prcClientNew, coordBufferSizeOld, &coordClientNewCharacters));

    // If we're in wrap text mode, then we want to be fixed to the window size. So use the character calculation we just got
    // to fix the buffer and window width together.
    if (gci.GetWrapText())
    {
        coordBufferSizeNew.X = coordClientNewCharacters.X;
    }

    // Reanalyze scroll bars in case we fixed the edge together for word wrap.
    // Use the new buffer client size.
    RETURN_IF_FAILED(_AdjustScreenBufferHelper(prcClientNew, coordBufferSizeNew, &coordClientNewCharacters));

    // Now reanalyze the buffer size and grow if we can fit more characters into the window no matter the console mode.
    if (_IsInPtyMode())
    {
        // The alt buffer always wants to be exactly the size of the screen, never more or less.
        // This prevents scrollbars when you increase the alt buffer size, then decrease it.
        // Can't have a buffer dimension of 0 - that'll cause divide by zeros in the future.
        coordBufferSizeNew.X = std::max(coordClientNewCharacters.X, 1i16);
        coordBufferSizeNew.Y = std::max(coordClientNewCharacters.Y, 1i16);
    }
    else
    {
        if (coordClientNewCharacters.X > coordBufferSizeNew.X)
        {
            coordBufferSizeNew.X = coordClientNewCharacters.X;
        }
        if (coordClientNewCharacters.Y > coordBufferSizeNew.Y)
        {
            coordBufferSizeNew.Y = coordClientNewCharacters.Y;
        }
    }

    // Only attempt to modify the buffer if something changed. Expensive operation.
    if (coordBufferSizeOld.X != coordBufferSizeNew.X ||
        coordBufferSizeOld.Y != coordBufferSizeNew.Y)
    {
        CommandLine* const pCommandLine = &CommandLine::Instance();

        // TODO: Deleting and redrawing the command line during resizing can cause flickering. See: http://osgvsowi/658439
        // 1. Delete input string if necessary (see menu.c)
        pCommandLine->Hide(FALSE);
        _textBuffer->GetCursor().SetIsVisible(false);

        // 2. Call the resize screen buffer method (expensive) to redimension the backing buffer (and reflow)
        LOG_IF_FAILED(ResizeScreenBuffer(coordBufferSizeNew, FALSE));

        // 3.  Reprint console input string
        pCommandLine->Show();
        _textBuffer->GetCursor().SetIsVisible(true);
    }

    return S_OK;
}

// Routine Description:
// - Calculates what width/height the viewport must have to consume all the available space in the given client area.
// - This compensates for scroll bars and will leave space in the client area for the bars if necessary.
// Arguments:
// - prcClientArea - The client rectangle in pixels of available rendering space.
// - pcoordSize - Filled with the width/height to which the viewport should be set.
// Return Value:
// - <none>
void SCREEN_INFORMATION::_CalculateViewportSize(const RECT* const prcClientArea, _Out_ COORD* const pcoordSize)
{
    COORD const coordBufferSize = GetScreenBufferSize();
    COORD const coordFontSize = GetScreenFontSize();

    SIZE sizeClientPixels = { 0 };
    sizeClientPixels.cx = RECT_WIDTH(prcClientArea);
    sizeClientPixels.cy = RECT_HEIGHT(prcClientArea);

    bool fIsHorizontalVisible;
    bool fIsVerticalVisible;
    s_CalculateScrollbarVisibility(prcClientArea,
                                   &coordBufferSize,
                                   &coordFontSize,
                                   &fIsHorizontalVisible,
                                   &fIsVerticalVisible);

    if (fIsHorizontalVisible)
    {
        sizeClientPixels.cy -= ServiceLocator::LocateGlobals().sHorizontalScrollSize;
    }
    if (fIsVerticalVisible)
    {
        sizeClientPixels.cx -= ServiceLocator::LocateGlobals().sVerticalScrollSize;
    }

    pcoordSize->X = (SHORT)(sizeClientPixels.cx / coordFontSize.X);
    pcoordSize->Y = (SHORT)(sizeClientPixels.cy / coordFontSize.Y);
}

// Routine Description:
// - Modifies the size of the current viewport to match the width/height of the request given.
// - Must specify which corner to adjust from. Default to false/false to resize from the bottom right corner.
// Arguments:
// - pcoordSize - Requested viewport width/heights in characters
// - fResizeFromTop - If false, will trim/add to bottom of viewport first. If true, will trim/add to top.
// - fResizeFromBottom - If false, will trim/add to top of viewport first. If true, will trim/add to left.
// Return Value:
// - <none>
void SCREEN_INFORMATION::_InternalSetViewportSize(const COORD* const pcoordSize,
                                                  const bool fResizeFromTop,
                                                  const bool fResizeFromLeft)
{
    const short DeltaX = pcoordSize->X - GetScreenWindowSizeX();
    const short DeltaY = pcoordSize->Y - GetScreenWindowSizeY();
    const COORD coordScreenBufferSize = GetScreenBufferSize();

    // do adjustments on a copy that's easily manipulated.
    SMALL_RECT srNewViewport = _viewport.ToInclusive();

    // Now we need to determine what our new Window size should
    // be. Note that Window here refers to the character/row window.
    if (fResizeFromLeft)
    {
        // we're being horizontally sized from the left border
        const SHORT sLeftProposed = (srNewViewport.Left - DeltaX);
        if (sLeftProposed >= 0)
        {
            // there's enough room in the backlog to just expand left
           srNewViewport.Left -= DeltaX;
        }
        else
        {
            // if we're resizing horizontally, we want to show as much
            // content above as we can, but we can't show more
            // than the left of the window
            srNewViewport.Left = 0;
            srNewViewport.Right += (SHORT)abs(sLeftProposed);
        }
    }
    else
    {
        // we're being horizontally sized from the right border
        const SHORT sRightProposed = (srNewViewport.Right + DeltaX);
        if (sRightProposed <= (coordScreenBufferSize.X - 1))
        {
            srNewViewport.Right += DeltaX;
        }
        else
        {
            srNewViewport.Right = (coordScreenBufferSize.X - 1);
            srNewViewport.Left -= (sRightProposed - (coordScreenBufferSize.X - 1));
        }
    }

    if (fResizeFromTop)
    {
        const SHORT sTopProposed = (srNewViewport.Top - DeltaY);
        // we're being vertically sized from the top border
        if (sTopProposed >= 0)
        {
            // Special case: Only modify the top position if we're not
            // on the 0th row of the buffer.

            // If we're on the 0th row, people expect it to stay stuck
            // to the top of the window, not to start collapsing down
            // and hiding the top rows.
            if (srNewViewport.Top > 0)
            {
                // there's enough room in the backlog to just expand the top
                srNewViewport.Top -= DeltaY;
            }
            else
            {
                // If we didn't adjust the top, we need to trim off
                // the number of rows from the bottom instead.
                // NOTE: It's += because DeltaY will be negative
                // already for this circumstance.
                FAIL_FAST_IF_FALSE(DeltaY <= 0);
                srNewViewport.Bottom += DeltaY;
            }
        }
        else
        {
            // if we're resizing vertically, we want to show as much
            // content above as we can, but we can't show more
            // than the top of the window
            srNewViewport.Top = 0;
            srNewViewport.Bottom += (SHORT)abs(sTopProposed);
        }
    }
    else
    {
        // we're being vertically sized from the bottom border
        const SHORT sBottomProposed = (srNewViewport.Bottom + DeltaY);
        if (sBottomProposed <= (coordScreenBufferSize.Y - 1))
        {
            // If the new bottom is supposed to be before the final line of the buffer
            // Check to ensure that we don't hide the prompt by collapsing the window.

            // The final valid end position will be the coordinates of
            // the last character displayed (including any characters
            // in the input line)
            COORD coordValidEnd;
            Selection::Instance().GetValidAreaBoundaries(nullptr, &coordValidEnd);

            // If the bottom of the window when adjusted would be
            // above the final line of valid text...
            if (srNewViewport.Bottom + DeltaY < coordValidEnd.Y)
            {
                // Adjust the top of the window instead of the bottom
                // (so the lines slide upward)
                srNewViewport.Top -= DeltaY;

                // If we happened to move the top of the window past
                // the 0th row (first row in the buffer)
                if (srNewViewport.Top < 0)
                {
                    // Find the amount we went past 0, correct the top
                    // of the window back to 0, and instead adjust the
                    // bottom even though it will cause us to lose the
                    // prompt line.
                    const short cRemainder = 0 - srNewViewport.Top;
                    srNewViewport.Top += cRemainder;
                    FAIL_FAST_IF_FALSE(srNewViewport.Top == 0);
                    srNewViewport.Bottom += cRemainder;
                }
            }
            else
            {
                srNewViewport.Bottom += DeltaY;
            }
        }
        else
        {
            srNewViewport.Bottom = (coordScreenBufferSize.Y - 1);
            srNewViewport.Top -= (sBottomProposed - (coordScreenBufferSize.Y - 1));
        }
    }

    // Ensure the viewport is valid.
    // We can't have a negative left or top.
    if (srNewViewport.Left < 0)
    {
        srNewViewport.Right -= srNewViewport.Left;
        srNewViewport.Left = 0;
    }

    if (srNewViewport.Top < 0)
    {
        srNewViewport.Bottom -= srNewViewport.Top;
        srNewViewport.Top = 0;
    }

    // Bottom and right cannot pass the final characters in the array.
    srNewViewport.Right = std::min(srNewViewport.Right, gsl::narrow<SHORT>(coordScreenBufferSize.X - 1));
    srNewViewport.Bottom = std::min(srNewViewport.Bottom, gsl::narrow<SHORT>(coordScreenBufferSize.Y - 1));

    _viewport = Viewport::FromInclusive(srNewViewport);
    Tracing::s_TraceWindowViewport(srNewViewport);
}

// Routine Description:
// - Modifies the size of the current viewport to match the width/height of the request given.
// - Uses the old and new client areas to determine which side the window was resized from.
// Arguments:
// - prcClientNew - Client rectangle in pixels after this update
// - prcClientOld - Client rectangle in pixels before this update
// - pcoordSize - Requested viewport width/heights in characters
// Return Value:
// - <none>
void SCREEN_INFORMATION::_AdjustViewportSize(const RECT* const prcClientNew,
                                             const RECT* const prcClientOld,
                                             const COORD* const pcoordSize)
{
    // If the left is the only one that changed (and not the right
    // also), then adjust from the left. Otherwise if the right
    // changes or both changed, bias toward leaving the top-left
    // corner in place and resize from the bottom right.
    // --
    // Resizing from the bottom right is more expected by
    // users. Normally only one dimension (or one corner) will change
    // at a time if the user is moving it. However, if the window is
    // being dragged and forced to resize at a monitor boundary, all 4
    // will change. In this case especially, users expect the top left
    // to stay in place and the bottom right to adapt.
    bool const fResizeFromLeft = prcClientNew->left != prcClientOld->left &&
                                 prcClientNew->right == prcClientOld->right;
    bool const fResizeFromTop = prcClientNew->top != prcClientOld->top &&
                                prcClientNew->bottom == prcClientOld->bottom;

    const Viewport oldViewport = Viewport(_viewport);

    _InternalSetViewportSize(pcoordSize, fResizeFromLeft, fResizeFromTop);

    // MSFT 13194969, related to 12092729.
    // If we're in virtual terminal mode, and the viewport dimensions change,
    //      send a WindowBufferSizeEvent. If the client wants VT mode, then they
    //      probably want the viewport resizes, not just the screen buffer
    //      resizes. This does change the behavior of the API for v2 callers,
    //      but only callers who've requested VT mode. In 12092729, we enabled
    //      sending notifications from window resizes in cases where the buffer
    //      didn't resize, so this applies the same expansion to resizes using
    //      the window, not the API.
    if (IsInVirtualTerminalInputMode())
    {
        if ((_viewport.Width() != oldViewport.Width()) ||
            (_viewport.Height() != oldViewport.Height()))
        {
            ScreenBufferSizeChange(GetScreenBufferSize());
        }
    }
}

// Routine Description:
// - From a window client area in pixels, a buffer size, and the font size, this will determine
//   whether scroll bars will need to be shown (and consume a portion of the client area) for the
//   given buffer to be rendered.
// Arguments:
// - prcClientArea - Client area in pixels of the available space for rendering
// - pcoordBufferSize - Buffer size in characters
// - pcoordFontSize - Font size in pixels per character
// - pfIsHorizontalVisible - Indicates whether the horizontal scroll
//   bar (consuming vertical space) will need to be visible
// - pfIsVerticalVisible - Indicates whether the vertical scroll bar
//   (consuming horizontal space) will need to be visible
// Return Value:
// - <none>
void SCREEN_INFORMATION::s_CalculateScrollbarVisibility(const RECT* const prcClientArea,
                                                        const COORD* const pcoordBufferSize,
                                                        const COORD* const pcoordFontSize,
                                                        _Out_ bool* const pfIsHorizontalVisible,
                                                        _Out_ bool* const pfIsVerticalVisible)
{
    FAIL_FAST_IF_FALSE(prcClientArea->left < prcClientArea->right);
    FAIL_FAST_IF_FALSE(prcClientArea->top < prcClientArea->bottom);
    FAIL_FAST_IF_FALSE(pcoordBufferSize->X > 0);
    FAIL_FAST_IF_FALSE(pcoordBufferSize->Y > 0);
    FAIL_FAST_IF_FALSE(pcoordFontSize->X > 0);
    FAIL_FAST_IF_FALSE(pcoordFontSize->Y > 0);

    // Start with bars not visible as the initial state of the client area doesn't account for scroll bars.
    *pfIsHorizontalVisible = false;
    *pfIsVerticalVisible = false;

    // Set up the client area in pixels
    SIZE sizeClientPixels = { 0 };
    sizeClientPixels.cx = RECT_WIDTH(prcClientArea);
    sizeClientPixels.cy = RECT_HEIGHT(prcClientArea);

    // Set up the buffer area in pixels by multiplying the size by the font size scale factor
    SIZE sizeBufferPixels = { 0 };
    sizeBufferPixels.cx = pcoordBufferSize->X * pcoordFontSize->X;
    sizeBufferPixels.cy = pcoordBufferSize->Y * pcoordFontSize->Y;

    // Now figure out whether we need one or both scroll bars.
    // Showing a scroll bar in one direction may necessitate showing
    // the scroll bar in the other (as it will consume client area
    // space).

    if (sizeBufferPixels.cx > sizeClientPixels.cx)
    {
        *pfIsHorizontalVisible = true;

        // If we have a horizontal bar, remove it from available
        // vertical space and check that remaining client area is
        // enough.
        sizeClientPixels.cy -= ServiceLocator::LocateGlobals().sHorizontalScrollSize;

        if (sizeBufferPixels.cy > sizeClientPixels.cy)
        {
            *pfIsVerticalVisible = true;
        }
    }
    else if (sizeBufferPixels.cy > sizeClientPixels.cy)
    {
        *pfIsVerticalVisible = true;

        // If we have a vertical bar, remove it from available
        // horizontal space and check that remaining client area is
        // enough.
        sizeClientPixels.cx -= ServiceLocator::LocateGlobals().sVerticalScrollSize;

        if (sizeBufferPixels.cx > sizeClientPixels.cx)
        {
            *pfIsHorizontalVisible = true;
        }
    }
}

bool SCREEN_INFORMATION::IsMaximizedBoth() const
{
    return IsMaximizedX() && IsMaximizedY();
}

bool SCREEN_INFORMATION::IsMaximizedX() const
{
    // If the viewport is displaying the entire size of the allocated buffer, it's maximized.
    return _viewport.Left() == 0 && (_viewport.Width() == GetScreenBufferSize().X);
}

bool SCREEN_INFORMATION::IsMaximizedY() const
{
    // If the viewport is displaying the entire size of the allocated buffer, it's maximized.
    return _viewport.Top() == 0 && (_viewport.Height() == GetScreenBufferSize().Y);
}

#pragma endregion

// Routine Description:
// - This is a screen resize algorithm which will reflow the ends of lines based on the
//   line wrap state used for clipboard line-based copy.
// Arguments:
// - <in> Coordinates of the new screen size
// Return Value:
// - Success if successful. Invalid parameter if screen buffer size is unexpected. No memory if allocation failed.
[[nodiscard]]
NTSTATUS SCREEN_INFORMATION::ResizeWithReflow(const COORD coordNewScreenSize)
{
    if ((USHORT)coordNewScreenSize.X >= SHORT_MAX || (USHORT)coordNewScreenSize.Y >= SHORT_MAX)
    {
        RIPMSG2(RIP_WARNING, "Invalid screen buffer size (0x%x, 0x%x)", coordNewScreenSize.X, coordNewScreenSize.Y);
        return STATUS_INVALID_PARAMETER;
    }

    // First allocate a new text buffer to take the place of the current one.
    CHAR_INFO ciFill;
    ciFill.Attributes = _Attributes.GetLegacyAttributes();

    std::unique_ptr<TextBuffer> newTextBuffer;
    try
    {
        newTextBuffer = std::make_unique<TextBuffer>(_textBuffer->GetCurrentFont(),
                                                     coordNewScreenSize,
                                                     ciFill,
                                                     0); // temporarily set size to 0 so it won't render.
    }
    catch (...)
    {
        return NTSTATUS_FROM_HRESULT(wil::ResultFromCaughtException());
    }

    // Save cursor's relative height versus the viewport
    SHORT const sCursorHeightInViewportBefore = _textBuffer->GetCursor().GetPosition().Y - _viewport.Top();

    Cursor& oldCursor = _textBuffer->GetCursor();
    Cursor& newCursor = newTextBuffer->GetCursor();
    // skip any drawing updates that might occur as we manipulate the new buffer
    oldCursor.StartDeferDrawing();
    newCursor.StartDeferDrawing();

    // We need to save the old cursor position so that we can
    // place the new cursor back on the equivalent character in
    // the new buffer.
    COORD cOldCursorPos = oldCursor.GetPosition();
    COORD cOldLastChar = _textBuffer->GetLastNonSpaceCharacter();

    short const cOldRowsTotal = cOldLastChar.Y + 1;
    short const cOldColsTotal = GetScreenBufferSize().X;

    COORD cNewCursorPos = { 0 };
    bool fFoundCursorPos = false;
    NTSTATUS status = STATUS_SUCCESS;
    // Loop through all the rows of the old buffer and reprint them into the new buffer
    for (short iOldRow = 0; iOldRow < cOldRowsTotal; iOldRow++)
    {
        // Fetch the row and its "right" which is the last printable character.
        const ROW& Row = _textBuffer->GetRowByOffset(iOldRow);
        const CharRow& charRow = Row.GetCharRow();
        short iRight = static_cast<short>(charRow.MeasureRight());

        // There is a special case here. If the row has a "wrap"
        // flag on it, but the right isn't equal to the width (one
        // index past the final valid index in the row) then there
        // were a bunch trailing of spaces in the row.
        // (But the measuring functions for each row Left/Right do
        // not count spaces as "displayable" so they're not
        // included.)
        // As such, adjust the "right" to be the width of the row
        // to capture all these spaces
        if (charRow.WasWrapForced())
        {
            iRight = cOldColsTotal;

            // And a combined special case.
            // If we wrapped off the end of the row by adding a
            // piece of padding because of a double byte LEADING
            // character, then remove one from the "right" to
            // leave this padding out of the copy process.
            if (charRow.WasDoubleBytePadded())
            {
                iRight--;
            }
        }

        // Loop through every character in the current row (up to
        // the "right" boundary, which is one past the final valid
        // character)
        for (short iOldCol = 0; iOldCol < iRight; iOldCol++)
        {
            if (iOldCol == cOldCursorPos.X && iOldRow == cOldCursorPos.Y)
            {
                cNewCursorPos = newCursor.GetPosition();
                fFoundCursorPos = true;
            }

            try
            {
                // Insert it into the new buffer
                const OutputCell cell = Row.AsCells(iOldCol, 1).front();
                if (!newTextBuffer->InsertCharacter(cell.Chars(), cell.DbcsAttr(), cell.TextAttr()))
                {
                    status = STATUS_NO_MEMORY;
                    break;
                }
            }
            catch (...)
            {
                return NTSTATUS_FROM_HRESULT(wil::ResultFromCaughtException());
            }
        }
        if (NT_SUCCESS(status))
        {
            // If we didn't have a full row to copy, insert a new
            // line into the new buffer.
            // Only do so if we were not forced to wrap. If we did
            // force a word wrap, then the existing line break was
            // only because we ran out of space.
            if (iRight < cOldColsTotal && !charRow.WasWrapForced())
            {
                if (iRight == cOldCursorPos.X && iOldRow == cOldCursorPos.Y)
                {
                    cNewCursorPos = newCursor.GetPosition();
                    fFoundCursorPos = true;
                }
                // Only do this if it's not the final line in the buffer.
                // On the final line, we want the cursor to sit
                // where it is done printing for the cursor
                // adjustment to follow.
                if (iOldRow < cOldRowsTotal - 1)
                {
                    status = newTextBuffer->NewlineCursor() ? status : STATUS_NO_MEMORY;
                }
                else
                {
                    // If we are on the final line of the buffer, we have one more check.
                    // We got into this code path because we are at the right most column of a row in the old buffer
                    // that had a hard return (no wrap was forced).
                    // However, as we're inserting, the old row might have just barely fit into the new buffer and
                    // caused a new soft return (wrap was forced) putting the cursor at x=0 on the line just below.
                    // We need to preserve the memory of the hard return at this point by inserting one additional
                    // hard newline, otherwise we've lost that information.
                    // We only do this when the cursor has just barely poured over onto the next line so the hard return
                    // isn't covered by the soft one.
                    // e.g.
                    // The old line was:
                    // |aaaaaaaaaaaaaaaaaaa | with no wrap which means there was a newline after that final a.
                    // The cursor was here ^
                    // And the new line will be:
                    // |aaaaaaaaaaaaaaaaaaa| and show a wrap at the end
                    // |                   |
                    //  ^ and the cursor is now there.
                    // If we leave it like this, we've lost the newline information.
                    // So we insert one more newline so a continued reflow of this buffer by resizing larger will
                    // continue to look as the original output intended with the newline data.
                    // After this fix, it looks like this:
                    // |aaaaaaaaaaaaaaaaaaa| no wrap at the end (preserved hard newline)
                    // |                   |
                    //  ^ and the cursor is now here.
                    const COORD coordNewCursor = newCursor.GetPosition();
                    if (coordNewCursor.X == 0 && coordNewCursor.Y > 0)
                    {
                        if (newTextBuffer->GetRowByOffset(coordNewCursor.Y - 1).GetCharRow().WasWrapForced())
                        {
                            status = newTextBuffer->NewlineCursor() ? status : STATUS_NO_MEMORY;
                        }
                    }
                }
            }
        }
    }
    if (NT_SUCCESS(status))
    {
        // Finish copying remaining parameters from the old text buffer to the new one
        newTextBuffer->CopyProperties(*_textBuffer);

        // If we found where to put the cursor while placing characters into the buffer,
        //   just put the cursor there. Otherwise we have to advance manually.
        if (fFoundCursorPos)
        {
            newCursor.SetPosition(cNewCursorPos);
        }
        else
        {
            // Advance the cursor to the same offset as before
            // get the number of newlines and spaces between the old end of text and the old cursor,
            //   then advance that many newlines and chars
            int iNewlines = cOldCursorPos.Y - cOldLastChar.Y;
            int iIncrements = cOldCursorPos.X - cOldLastChar.X;
            const COORD cNewLastChar = newTextBuffer->GetLastNonSpaceCharacter();

            // If the last row of the new buffer wrapped, there's going to be one less newline needed,
            //   because the cursor is already on the next line
            if (newTextBuffer->GetRowByOffset(cNewLastChar.Y).GetCharRow().WasWrapForced())
            {
                iNewlines = std::max(iNewlines - 1, 0);
            }
            else
            {
                // if this buffer didn't wrap, but the old one DID, then the d(columns) of the
                //   old buffer will be one more than in this buffer, so new need one LESS.
                if (_textBuffer->GetRowByOffset(cOldLastChar.Y).GetCharRow().WasWrapForced())
                {
                    iNewlines = std::max(iNewlines - 1, 0);
                }
            }

            for (int r = 0; r < iNewlines; r++)
            {
                if (!newTextBuffer->NewlineCursor())
                {
                    status = STATUS_NO_MEMORY;
                    break;
                }
            }
            if (NT_SUCCESS(status))
            {
                for (int c = 0; c < iIncrements - 1; c++)
                {
                    if (!newTextBuffer->IncrementCursor())
                    {
                        status = STATUS_NO_MEMORY;
                        break;
                    }
                }
            }
        }
    }

    if (NT_SUCCESS(status))
    {
        // Adjust the viewport so the cursor doesn't wildly fly off up or down.
        SHORT const sCursorHeightInViewportAfter = newCursor.GetPosition().Y - _viewport.Top();
        COORD coordCursorHeightDiff = { 0 };
        coordCursorHeightDiff.Y = sCursorHeightInViewportAfter - sCursorHeightInViewportBefore;
        LOG_IF_FAILED(SetViewportOrigin(FALSE, coordCursorHeightDiff));

        // Save old cursor size before we delete it
        ULONG const ulSize = oldCursor.GetSize();

        _textBuffer.swap(newTextBuffer);

        // Set size back to real size as it will be taking over the rendering duties.
        newCursor.SetSize(ulSize);
        newCursor.EndDeferDrawing();
    }
    oldCursor.EndDeferDrawing();

    return status;
}

//
// Routine Description:
// - This is the legacy screen resize with minimal changes
// Arguments:
// - NewScreenSize - new size of screen.
// Return Value:
// - Success if successful. Invalid parameter if screen buffer size is unexpected. No memory if allocation failed.
[[nodiscard]]
NTSTATUS SCREEN_INFORMATION::ResizeTraditional(const COORD coordNewScreenSize)
{
    return NTSTATUS_FROM_HRESULT(_textBuffer->ResizeTraditional(GetScreenBufferSize(), coordNewScreenSize, _Attributes));
}

//
// Routine Description:
// - This routine resizes the screen buffer.
// Arguments:
// - NewScreenSize - new size of screen in characters
// - DoScrollBarUpdate - indicates whether to update scroll bars at the end
// Return Value:
// - Success if successful. Invalid parameter if screen buffer size is unexpected. No memory if allocation failed.
[[nodiscard]]
NTSTATUS SCREEN_INFORMATION::ResizeScreenBuffer(const COORD coordNewScreenSize,
                                                const bool fDoScrollBarUpdate)
{
    const CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    NTSTATUS status = STATUS_SUCCESS;

    // cancel any active selection before resizing or it will not necessarily line up with the new buffer positions
    Selection::Instance().ClearSelection();

    // cancel any popups before resizing or they will not necessarily line up with new buffer positions
    if (nullptr != gci.lpCookedReadData)
    {
        CleanUpPopups(gci.lpCookedReadData);
    }

    const bool fWrapText = gci.GetWrapText();
    if (fWrapText)
    {
        status = ResizeWithReflow(coordNewScreenSize);
    }
    else
    {
        status = NTSTATUS_FROM_HRESULT(ResizeTraditional(coordNewScreenSize));
    }

    if (NT_SUCCESS(status))
    {
        SetScreenBufferSize(coordNewScreenSize);

        const COORD coordSetScreenBufferSize = GetScreenBufferSize();
        _textBuffer->SetCoordBufferSize(coordSetScreenBufferSize);

        NotifyAccessibilityEventing(0, 0, (SHORT)(coordSetScreenBufferSize.X - 1), (SHORT)(coordSetScreenBufferSize.Y - 1));

        if ((!ConvScreenInfo))
        {
            if (FAILED(ConsoleImeResizeCompStrScreenBuffer(coordNewScreenSize)))
            {
                // If something went wrong, just bail out.
                return STATUS_INVALID_HANDLE;
            }
        }

        // Fire off an event to let accessibility apps know the layout has changed.
        if (IsActiveScreenBuffer())
        {
            _pAccessibilityNotifier->NotifyConsoleLayoutEvent();
        }

        if (fDoScrollBarUpdate)
        {
            UpdateScrollBars();
        }
        ScreenBufferSizeChange(coordSetScreenBufferSize);
    }

    return status;
}

// Routine Description:
// - Given a rectangle containing screen buffer coordinates (character-level positioning, not pixel)
//   This method will trim the rectangle to ensure it is within the buffer.
//   For example, if the rectangle given has a right position of 85, but the current screen buffer
//   is only reaching from 0-79, then the right position will be set to 79.
// Arguments:
// - psrRect - Pointer to rectangle holding data to be trimmed
// Return Value:
// - <none>
void SCREEN_INFORMATION::ClipToScreenBuffer(_Inout_ SMALL_RECT* const psrClip) const
{
    SMALL_RECT srEdges;
    GetScreenEdges(&srEdges);

    psrClip->Left = std::max(psrClip->Left, srEdges.Left);
    psrClip->Top = std::max(psrClip->Top, srEdges.Top);
    psrClip->Right = std::min(psrClip->Right, srEdges.Right);
    psrClip->Bottom = std::min(psrClip->Bottom, srEdges.Bottom);
}

// Routine Description:
// - Given a coordinate containing screen buffer coordinates (character-level positioning, not pixel)
//   This method will ensure that it is within the buffer.
// Arguments:
// - pcoordClip - Pointer to coordinate holding data to be trimmed
// Return Value:
// - <none>
void SCREEN_INFORMATION::ClipToScreenBuffer(_Inout_ COORD* const pcoordClip) const
{
    SMALL_RECT srEdges;
    GetScreenEdges(&srEdges);

    pcoordClip->X = std::max(pcoordClip->X, srEdges.Left);
    pcoordClip->Y = std::max(pcoordClip->Y, srEdges.Top);
    pcoordClip->X = std::min(pcoordClip->X, srEdges.Right);
    pcoordClip->Y = std::min(pcoordClip->Y, srEdges.Bottom);
}

// Routine Description:
// - Gets the edges of the screen buffer.
//   "Edges" refers to the inclusive final positions in each direction of the screen buffer area.
//   For example, a line that is 80 characters long will go from positions 0 to 79 in the buffer.
//   In this case, 0 is the left edge and 79 is the right edge. The last inclusive index of these points in the buffer.
// Arguments:
// - psrEdges - Pointer to rectangle to hold the edge data
// Return Value:
// - <none>
void SCREEN_INFORMATION::GetScreenEdges(_Out_ SMALL_RECT* const psrEdges) const
{
    const COORD coordScreenBufferSize = GetScreenBufferSize();
    psrEdges->Left = 0;
    psrEdges->Right = coordScreenBufferSize.X - 1;
    psrEdges->Top = 0;
    psrEdges->Bottom = coordScreenBufferSize.Y - 1;
}

void SCREEN_INFORMATION::MakeCurrentCursorVisible()
{
    MakeCursorVisible(_textBuffer->GetCursor().GetPosition());
}

// Routine Description:
// - This routine sets the cursor size and visibility both in the data
//      structures and on the screen. Also updates the cursor information of
//      this buffer's main buffer, if this buffer is an alt buffer.
// TODO: MSFT:15954781 - split this into one method for each param.
// Arguments:
// - ScreenInfo - pointer to screen info structure.
// - Size - cursor size
// - Visible - cursor visibility
// Return Value:
// - None
void SCREEN_INFORMATION::SetCursorInformation(const ULONG Size,
                                              const bool Visible,
                                              _In_ unsigned int const Color,
                                              const CursorType Type)
{
    Cursor& cursor = _textBuffer->GetCursor();

    cursor.SetSize(Size);
    cursor.SetIsVisible(Visible);

    cursor.SetColor(Color);
    cursor.SetType(Type);

    // If we're an alt buffer, also update our main buffer.
    if (_psiMainBuffer)
    {
        _psiMainBuffer->SetCursorInformation(Size, Visible, Color, Type);
    }
}

// Routine Description:
// - This routine sets a flag saying whether the cursor should be displayed
//   with it's default size or it should be modified to indicate the
//   insert/overtype mode has changed.
// Arguments:
// - ScreenInfo - pointer to screen info structure.
// - DoubleCursor - should we indicated non-normal mode
// Return Value:
// - None
void  SCREEN_INFORMATION::SetCursorDBMode(const bool DoubleCursor)
{
    Cursor& cursor = _textBuffer->GetCursor();

    if ((cursor.IsDouble() != DoubleCursor))
    {
        cursor.SetIsDouble(DoubleCursor);
    }

    // If we're an alt buffer, also update our main buffer.
    if (_psiMainBuffer)
    {
        _psiMainBuffer->SetCursorDBMode(DoubleCursor);
    }
}

// Routine Description:
// - This routine sets the cursor position in the data structures and on the screen.
// Arguments:
// - ScreenInfo - pointer to screen info structure.
// - Position - new position of cursor
// - TurnOn - true if cursor should be left on, false if should be left off
// Return Value:
// - Status
[[nodiscard]]
NTSTATUS SCREEN_INFORMATION::SetCursorPosition(const COORD Position, const bool TurnOn)
{
    const CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    Cursor& cursor = _textBuffer->GetCursor();

    //
    // Ensure that the cursor position is within the constraints of the screen
    // buffer.
    //
    const COORD coordScreenBufferSize = GetScreenBufferSize();
    if (Position.X >= coordScreenBufferSize.X || Position.Y >= coordScreenBufferSize.Y || Position.X < 0 || Position.Y < 0)
    {
        return STATUS_INVALID_PARAMETER;
    }

    cursor.SetPosition(Position);

    // if we have the focus, adjust the cursor state
    if (gci.Flags & CONSOLE_HAS_FOCUS)
    {
        if (TurnOn)
        {
            cursor.SetDelay(false);
            cursor.SetIsOn(true);
        }
        else
        {
            cursor.SetDelay(true);
        }
        cursor.SetHasMoved(true);
    }

    return STATUS_SUCCESS;
}

void SCREEN_INFORMATION::MakeCursorVisible(const COORD CursorPosition)
{
    COORD WindowOrigin;

    if (CursorPosition.X > _viewport.RightInclusive())
    {
        WindowOrigin.X = CursorPosition.X - _viewport.RightInclusive();
    }
    else if (CursorPosition.X < _viewport.Left())
    {
        WindowOrigin.X = CursorPosition.X - _viewport.Left();
    }
    else
    {
        WindowOrigin.X = 0;
    }

    if (CursorPosition.Y > _viewport.BottomInclusive())
    {
        WindowOrigin.Y = CursorPosition.Y - _viewport.BottomInclusive();
    }
    else if (CursorPosition.Y < _viewport.Top())
    {
        WindowOrigin.Y = CursorPosition.Y - _viewport.Top();
    }
    else
    {
        WindowOrigin.Y = 0;
    }

    if (WindowOrigin.X != 0 || WindowOrigin.Y != 0)
    {
        LOG_IF_FAILED(SetViewportOrigin(FALSE, WindowOrigin));
    }
}

void SCREEN_INFORMATION::SetScrollMargins(const SMALL_RECT* const psrMargins)
{
    _srScrollMargins = *psrMargins;
}

SMALL_RECT SCREEN_INFORMATION::GetScrollMargins() const
{
    return _srScrollMargins;
}

// Routine Description:
// - Retrieves the active buffer of this buffer. If this buffer has an
//     alternate buffer, this is the alternate buffer. Otherwise, it is this buffer.
// Parameters:
// - None
// Return value:
// - a reference to this buffer's active buffer.
SCREEN_INFORMATION& SCREEN_INFORMATION::GetActiveBuffer()
{
    return const_cast<SCREEN_INFORMATION&>(static_cast<const SCREEN_INFORMATION* const>(this)->GetActiveBuffer());
}

const SCREEN_INFORMATION& SCREEN_INFORMATION::GetActiveBuffer() const
{
    if (_psiAlternateBuffer != nullptr)
    {
        return *_psiAlternateBuffer;
    }
    return *this;
}

// Routine Description:
// - Retrieves the main buffer of this buffer. If this buffer has an
//     alternate buffer, this is the main buffer. Otherwise, it is this buffer's main buffer.
//     The main buffer is not necessarily the active buffer.
// Parameters:
// - None
// Return value:
// - a reference to this buffer's main buffer.
SCREEN_INFORMATION& SCREEN_INFORMATION::GetMainBuffer()
{
    return const_cast<SCREEN_INFORMATION&>(static_cast<const SCREEN_INFORMATION* const>(this)->GetMainBuffer());
}

const SCREEN_INFORMATION& SCREEN_INFORMATION::GetMainBuffer() const
{
    if (_psiMainBuffer != nullptr)
    {
        return *_psiMainBuffer;
    }
    return *this;
}

// Routine Description:
// - Instantiates a new buffer to be used as an alternate buffer. This buffer
//     does not have a driver handle associated with it and shares a state
//     machine with the main buffer it belongs to.
// Parameters:
// - ppsiNewScreenBuffer - a pointer to recieve the newly created buffer.
// Return value:
// - STATUS_SUCCESS if handled successfully. Otherwise, an approriate status code indicating the error.
[[nodiscard]]
NTSTATUS SCREEN_INFORMATION::_CreateAltBuffer(_Out_ SCREEN_INFORMATION** const ppsiNewScreenBuffer)
{
    // Create new screen buffer.
    CHAR_INFO Fill;
    Fill.Char.UnicodeChar = UNICODE_SPACE;
    Fill.Attributes = _Attributes.GetLegacyAttributes();

    COORD WindowSize = _viewport.Dimensions();

    const FontInfo& existingFont = _textBuffer->GetCurrentFont();

    NTSTATUS Status = SCREEN_INFORMATION::CreateInstance(WindowSize,
                                                         existingFont,
                                                         WindowSize,
                                                         Fill,
                                                         Fill,
                                                         CURSOR_SMALL_SIZE,
                                                         ppsiNewScreenBuffer);
    if (NT_SUCCESS(Status))
    {
        s_InsertScreenBuffer(*ppsiNewScreenBuffer);

        // delete the alt buffer's state machine. We don't want it.
        (*ppsiNewScreenBuffer)->_FreeOutputStateMachine(); // this has to be done before we give it a main buffer
        // we'll attach the GetSet, etc once we successfully make this buffer the active buffer.

        // Set up the new buffers references to our current state machine, dispatcher, getset, etc.
        (*ppsiNewScreenBuffer)->_pStateMachine = _pStateMachine;
        (*ppsiNewScreenBuffer)->_pAdapter = _pAdapter;
        (*ppsiNewScreenBuffer)->_pBufferWriter = _pBufferWriter;
        (*ppsiNewScreenBuffer)->_pConApi = _pConApi;
    }
    return Status;
}

// Routine Description:
// - Creates an "alternate" screen buffer for this buffer. In virtual terminals, there exists both a "main"
//     screen buffer and an alternate. ASBSET creates a new alternate, and switches to it. If there is an already
//     existing alternate, it is discarded. This allows applications to retain one HANDLE, and switch which buffer it points to seamlessly.
// Parameters:
// - None
// Return value:
// - STATUS_SUCCESS if handled successfully. Otherwise, an approriate status code indicating the error.
[[nodiscard]]
NTSTATUS SCREEN_INFORMATION::UseAlternateScreenBuffer()
{
    CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    SCREEN_INFORMATION& siMain = GetMainBuffer();
    // If we're in an alt that resized, resize the main before making the new alt
    if (siMain._fAltWindowChanged)
    {
        siMain.ProcessResizeWindow(&(siMain._rcAltSavedClientNew), &(siMain._rcAltSavedClientOld));
        siMain._fAltWindowChanged = false;
    }

    SCREEN_INFORMATION* psiNewAltBuffer;
    NTSTATUS Status = _CreateAltBuffer(&psiNewAltBuffer);
    if (NT_SUCCESS(Status))
    {
        // if this is already an alternate buffer, we want to make the new
        // buffer the alt on our main buffer, not on ourself, because there
        // can only ever be one main and one alternate.
        SCREEN_INFORMATION* const psiOldAltBuffer = siMain._psiAlternateBuffer;

        psiNewAltBuffer->_psiMainBuffer = &siMain;
        siMain._psiAlternateBuffer = psiNewAltBuffer;

        if (psiOldAltBuffer != nullptr)
        {
            s_RemoveScreenBuffer(psiOldAltBuffer); // this will also delete the old alt buffer
        }

        ::SetActiveScreenBuffer(*psiNewAltBuffer);

        // Kind of a hack until we have proper signal channels: If the client app wants window size events, send one for
        // the new alt buffer's size (this is so WSL can update the TTY size when the MainSB.viewportWidth <
        // MainSB.bufferWidth (which can happen with wrap text disabled))
        ScreenBufferSizeChange(psiNewAltBuffer->GetScreenBufferSize());

        // Tell the VT MouseInput handler that we're in the Alt buffer now
        gci.terminalMouseInput.UseAlternateScreenBuffer();

    }
    return Status;
}

// Routine Description:
// - Restores the active buffer to be this buffer's main buffer. If this is the main buffer, then nothing happens.
// Parameters:
// - None
// Return value:
// - STATUS_SUCCESS if handled successfully. Otherwise, an approriate status code indicating the error.
void SCREEN_INFORMATION::UseMainScreenBuffer()
{
    CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    SCREEN_INFORMATION* psiMain = _psiMainBuffer;
    if (psiMain != nullptr)
    {
        if (psiMain->_fAltWindowChanged)
        {
            psiMain->ProcessResizeWindow(&(psiMain->_rcAltSavedClientNew), &(psiMain->_rcAltSavedClientOld));
            psiMain->_fAltWindowChanged = false;
        }
        ::SetActiveScreenBuffer(*psiMain);
        psiMain->UpdateScrollBars(); // The alt had disabled scrollbars, re-enable them

        // send a _coordScreenBufferSizeChangeEvent for the new Sb viewport
        ScreenBufferSizeChange(psiMain->GetScreenBufferSize());

        SCREEN_INFORMATION* psiAlt = psiMain->_psiAlternateBuffer;
        psiMain->_psiAlternateBuffer = nullptr;
        s_RemoveScreenBuffer(psiAlt); // this will also delete the alt buffer
        // deleting the alt buffer will give the GetSet back to it's main

        // Tell the VT MouseInput handler that we're in the main buffer now
        gci.terminalMouseInput.UseMainScreenBuffer();
    }
}

// Routine Description:
// - Helper indicating if the buffer has a main buffer, meaning that this is an alternate buffer.
// Parameters:
// - None
// Return value:
// - true iff this buffer has a main buffer.
bool SCREEN_INFORMATION::_IsAltBuffer() const
{
    return _psiMainBuffer != nullptr;
}

// Routine Description:
// - Helper indicating if the buffer is acting as a pty - with the screenbuffer
//      clamped to the viewport size. This can be the case either when we're in
//      VT I/O mode, or when this buffer is an alt buffer.
// Parameters:
// - None
// Return value:
// - true iff this buffer has a main buffer.
bool SCREEN_INFORMATION::_IsInPtyMode() const
{
    const CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    return _IsAltBuffer() || gci.IsInVtIoMode();
}

// Routine Description:
// - Sets a VT tab stop in the column sColumn. If there is already a tab there, it does nothing.
// Parameters:
// - sColumn: the column to add a tab stop to.
// Return value:
// - STATUS_SUCCESS if handled successfully. Otherwise, an approriate status code indicating the error.
//      (This is most likely an allocation failure on the instantiation of the new tab stop.)
// Note:
//  This screen buffer is responsible for the lifetime of any tab stops added to it. They can all be freed with ClearTabStops()
[[nodiscard]]
NTSTATUS SCREEN_INFORMATION::AddTabStop(const SHORT sColumn)
{
    NTSTATUS Status = STATUS_NO_MEMORY;

    TabStop* prev = _ptsTabs;
    if (prev == nullptr || prev->sColumn > sColumn) //if there is no head, or we should insert at the head
    {
        _ptsTabs = new(std::nothrow) TabStop();
        Status = NT_TESTNULL(_ptsTabs);
        if (NT_SUCCESS(Status))
        {
            _ptsTabs->sColumn = sColumn;
            _ptsTabs->ptsNext = prev;
        }
    }
    else
    {
        bool fSearching = true;
        while (fSearching)
        {
            // if there's already a tabstop here, don't add another
            if (prev->sColumn == sColumn)
            {
                break;
            }
            // if we're at the end, or we should insert after prev
            if (prev->ptsNext == nullptr || prev->ptsNext->sColumn > sColumn)
            {
                fSearching = false;
                break;
            }
            else
            {
                prev = prev->ptsNext;
            }
        }
        if (!fSearching) //we broke out by finding the right spot to insert,
        {               // NOT by finding an existing tabstop here.
            TabStop* ptsNewTabStop = new(std::nothrow) TabStop();
            Status = NT_TESTNULL(ptsNewTabStop);
            if (NT_SUCCESS(Status))
            {
                ptsNewTabStop->sColumn = sColumn;
                ptsNewTabStop->ptsNext = prev->ptsNext;
                prev->ptsNext = ptsNewTabStop;
            }
        }

    }
    return Status;

}

// Routine Description:
// - Clears all of the VT tabs that have been set. This also deletes them.
// Parameters:
// <none>
// Return value:
// <none>
void SCREEN_INFORMATION::ClearTabStops()
{
    TabStop* curr = _ptsTabs;
    while (curr != nullptr)
    {
        TabStop* prev = curr;
        curr = curr->ptsNext;
        delete prev;
    }
    _ptsTabs = nullptr;
}

// Routine Description:
// - Clears the VT tab in the column sColumn (if one has been set). Also deletes it from the heap.
// Parameters:
// - sColumn - The column to clear the tab stop for.
// Return value:
// <none>
void SCREEN_INFORMATION::ClearTabStop(const SHORT sColumn)
{
    if (AreTabsSet())
    {
        // When we start, there is no previous item.
        TabStop* prev = nullptr;
        // Take the starting "current" item from the class storage.
        TabStop* curr = _ptsTabs;

        // Dig through every element in the list
        while (curr != nullptr)
        {
            // Hold pointer to next item only after we're sure that the current is valid.
            TabStop* next = curr->ptsNext; // Will be nullptr if it's the end of the list. This is OK.

            // If the current one matches the column, take it out of the linked list and join the prev and curr items together.
            if (curr->sColumn == sColumn)
            {
                if (prev == nullptr)
                {
                    // If we had no previous, we're at the list head and need to update the stored list head pointer to the following item.
                    _ptsTabs = next;
                }
                else
                {
                    // If we had a previous, then just connect the next to the "next" pointer of the previous.
                    prev->ptsNext = next;
                }

                // And delete the allocation for the current now that nothing is pointing to it anymore.
                delete curr;

                // Walk forward to the next item.
                // Prev remains the same as we deleted what would have moved there.
                curr = next;
            }
            else
            {
                // Walk forward to the next item.
                prev = curr;
                curr = next;
            }
        }
    }
}

// Routine Description:
// - Places the location that a forwards tab would take cCurrCursorPos to into pcNewCursorPos
// Parameters:
// - cCurrCursorPos - The initial cursor location
// - pcNewCursorPos - The cursor location after a forwards tab.
// Return value:
// - <none>
COORD SCREEN_INFORMATION::GetForwardTab(const COORD cCurrCursorPos)
{
    COORD cNewCursorPos = cCurrCursorPos;
    SHORT sWidth = GetScreenBufferSize().X - 1;
    if (cCurrCursorPos.X == sWidth)
    {
        cNewCursorPos.X = 0;
        cNewCursorPos.Y += 1;
    }
    else
    {
        TabStop* ptsNext = _ptsTabs;
        while (ptsNext != nullptr)
        {
            if (cCurrCursorPos.X >= ptsNext->sColumn)
            {
                ptsNext = ptsNext->ptsNext;
            }
            else
            {
                break;
            }
        }
        if (ptsNext == nullptr)
        {
            cNewCursorPos.X = sWidth;
        }
        else
        {
            cNewCursorPos.X = ptsNext->sColumn;
        }
    }
    return cNewCursorPos;
}

// Routine Description:
// - Places the location that a backwards tab would take cCurrCursorPos to into pcNewCursorPos
// Parameters:
// - cCurrCursorPos - The initial cursor location
// - pcNewCursorPos - The cursor location after a reverse tab.
// Return value:
// - <none>
COORD SCREEN_INFORMATION::GetReverseTab(const COORD cCurrCursorPos)
{
    COORD cNewCursorPos = cCurrCursorPos;
    // if we're at 0, or there are NO tabs, or the first tab is farther than where we are
    if (cCurrCursorPos.X == 0 || (_ptsTabs == nullptr) || (_ptsTabs->sColumn >= cCurrCursorPos.X))
    {
        cNewCursorPos.X = 0;
    }
    else // _ptsTabs != null, and we're past the first tab stop
    {
        TabStop* ptsCurr;
        // while we still have at least one to iterate over, and we are still farther than the current tabstop
        for (ptsCurr = _ptsTabs;
             ptsCurr->ptsNext != nullptr && cCurrCursorPos.X > ptsCurr->ptsNext->sColumn;
             ptsCurr = ptsCurr->ptsNext)
        {
            ; //just iterate through till the loop is done.
        }

        cNewCursorPos.X = ptsCurr->sColumn;
    }
    return cNewCursorPos;
}

// Routine Description:
// - Returns true if any VT-style tab stops have been set (with AddTabStop)
// Parameters:
// <none>
// Return value:
// - true if any VT-style tab stops have been set
bool SCREEN_INFORMATION::AreTabsSet()
{
    return _ptsTabs != nullptr;
}

// Routine Description:
// - Returns the value of the attributes
// Parameters:
// <none>
// Return value:
// - This screen buffer's attributes
TextAttribute SCREEN_INFORMATION::GetAttributes() const
{
    return _Attributes;
}

// Routine Description:
// - Returns the value of the popup attributes
// Parameters:
// <none>
// Return value:
// - This screen buffer's popup attributes
const TextAttribute* const SCREEN_INFORMATION::GetPopupAttributes() const
{
    return &_PopupAttributes;
}

// Routine Description:
// - Sets the value of the attributes on this screen buffer. Also propagates
//     the change down to the fill of the text buffer attached to this screen buffer.
// Parameters:
// - attributes - The new value of the attributes to use.
// Return value:
// <none>
void SCREEN_INFORMATION::SetAttributes(const TextAttribute& attributes)
{
    _Attributes = attributes;

    CHAR_INFO ciFill = _textBuffer->GetFill();
    ciFill.Attributes = _Attributes.GetLegacyAttributes();
    _textBuffer->SetFill(ciFill);

    // If we're an alt buffer, also update our main buffer.
    if (_psiMainBuffer)
    {
        _psiMainBuffer->SetAttributes(attributes);
    }
}

// Method Description:
// - Sets the value of the popup attributes on this screen buffer.
// Parameters:
// - popupAttributes - The new value of the popup attributes to use.
// Return value:
// <none>
void SCREEN_INFORMATION::SetPopupAttributes(const TextAttribute& popupAttributes)
{
    _PopupAttributes = popupAttributes;
    // If we're an alt buffer, also update our main buffer.
    if (_psiMainBuffer)
    {
        _psiMainBuffer->SetPopupAttributes(popupAttributes);
    }
}

// Method Description:
// - Sets the value of the attributes on this screen buffer. Also propagates
//     the change down to the fill of the attached text buffer.
// Parameters:
// - attributes - The new value of the attributes to use.
// - popupAttributes - The new value of the popup attributes to use.
// Return value:
// <none>
void SCREEN_INFORMATION::SetDefaultAttributes(const TextAttribute& attributes,
                                              const TextAttribute& popupAttributes)
{
    SetAttributes(attributes);
    SetPopupAttributes(popupAttributes);
    GetAdapterDispatch()->UpdateDefaultColor(attributes.GetLegacyAttributes());
}

// Method Description:
// - Replaces the given oldAttributes and oldPopupAttributes with the
//      newAttributes thoughout the entirety of our buffer.
//   This is called when the default screen attributes change, (eg through the
//      propsheet or the API) and we want to replace all of the attributes of
//      characters that had the old default attributes with the new setting.
// NOTE: Only replaces legacy style attributes. If a character had RGB
//      attributes, then we know that it wasn't using the default attributes.
// Parameters:
// - oldAttributes - The old attributes containing legacy attributes to replace.
// - oldPopupAttributes - The old popoup attributes to replace.
// - newAttributes - The new value of the attributes to use.
// - newPopupAttributes - The new value of the popup attributes to use.
// Return value:
// <none>
void SCREEN_INFORMATION::ReplaceDefaultAttributes(const TextAttribute& oldAttributes,
                                                  const TextAttribute& oldPopupAttributes,
                                                  const TextAttribute& newAttributes,
                                                  const TextAttribute& newPopupAttributes)
{
    const WORD oldLegacyAttributes = oldAttributes.GetLegacyAttributes();
    const WORD oldLegacyPopupAttributes = oldPopupAttributes.GetLegacyAttributes();
    const WORD newLegacyAttributes = newAttributes.GetLegacyAttributes();
    const WORD newLegacyPopupAttributes = newPopupAttributes.GetLegacyAttributes();

    // TODO: MSFT 9354902: Fix this up to be clearer with less magic bit shifting and less magic numbers. http://osgvsowi/9354902
    const WORD InvertedOldPopupAttributes = (WORD)(((oldLegacyPopupAttributes << 4) & 0xf0) | ((oldLegacyPopupAttributes >> 4) & 0x0f));
    const WORD InvertedNewPopupAttributes = (WORD)(((newLegacyPopupAttributes << 4) & 0xf0) | ((newLegacyPopupAttributes >> 4) & 0x0f));

    // change all chars with default color
    const SHORT sScreenBufferSizeY = GetScreenBufferSize().Y;
    for (SHORT i = 0; i < sScreenBufferSizeY; i++)
    {
        ROW& Row = _textBuffer->GetRowByOffset(i);
        ATTR_ROW& attrRow = Row.GetAttrRow();
        attrRow.ReplaceLegacyAttrs(oldLegacyAttributes, newLegacyAttributes);
        attrRow.ReplaceLegacyAttrs(oldLegacyPopupAttributes, newLegacyPopupAttributes);
        attrRow.ReplaceLegacyAttrs(InvertedOldPopupAttributes, InvertedNewPopupAttributes);
    }

    if (_psiMainBuffer)
    {
        _psiMainBuffer->ReplaceDefaultAttributes(oldAttributes, oldPopupAttributes, newAttributes, newPopupAttributes);
    }
}

// Method Description:
// - Returns an inclusive rectangle that describes the bounds of the buffer viewport.
// Arguments:
// - <none>
// Return Value:
// - the viewport bounds as an inclusive rect.
SMALL_RECT SCREEN_INFORMATION::GetBufferViewport() const
{
    return _viewport.ToInclusive();
}

void SCREEN_INFORMATION::SetBufferViewport(const Viewport newViewport)
{
    _viewport = Viewport(newViewport);
}

// Method Description:
// - Performs a VT Erase All operation. In most terminals, this is done by
//      moving the viewport into the scrollback, clearing out the current screen.
//      For them, there can never be any characters beneath the viewport, as the
//      viewport is always at the bottom. So, we can accomplish the same behavior
//      by using the LastNonspaceCharacter as the "bottom", and placing the new
//      viewport underneath that character.
// Parameters:
//  <none>
// Return value:
// - S_OK if we succeeded, or another status if there was a failure.
[[nodiscard]]
HRESULT SCREEN_INFORMATION::VtEraseAll()
{
    const COORD coordLastChar = _textBuffer->GetLastNonSpaceCharacter();
    short sNewTop = coordLastChar.Y + 1;
    const Viewport oldViewport = _viewport;
    // Stash away the current position of the cursor within the viewport.
    // We'll need to restore the cursor to that same relative position, after
    //      we move the viewport.
    const COORD oldCursorPos = _textBuffer->GetCursor().GetPosition();
    COORD relativeCursor = oldCursorPos;
    oldViewport.ConvertToOrigin(&relativeCursor);

    short delta = (sNewTop + GetScreenWindowSizeY()) - (GetScreenBufferSize().Y);
    for (auto i = 0; i < delta; i++)
    {
        _textBuffer->IncrementCircularBuffer();
        sNewTop--;
    }

    const COORD coordNewOrigin = {0, sNewTop};
    RETURN_IF_FAILED(SetViewportOrigin(TRUE, coordNewOrigin));
    // Restore the relative cursor position
    _viewport.ConvertFromOrigin(&relativeCursor);
    RETURN_IF_FAILED(SetCursorPosition(relativeCursor, false));

    // Update all the rows in the current viewport with the currently active attributes.
    ROW* pRow = &_textBuffer->GetRowByOffset(sNewTop);
    auto height = _viewport.Height();

    for (int i = 0; i < height; i++)
    {
        pRow->GetAttrRow().SetAttrToEnd(0, _Attributes);
        pRow = &_textBuffer->GetNextRow(*pRow);
    }

    // Invalidate the entire viewport
    if (ServiceLocator::LocateGlobals().pRender != nullptr)
    {
        ServiceLocator::LocateGlobals().pRender->TriggerRedrawAll();
    }

    return S_OK;
}

// Method Description:
// - Initialize the dimensions of the screenbuffer. If we're in VT I/O mode,
//      then use the screen buffer dimensions as the size of the viewport.
// Arguments:
// - coordScreenBufferSize: The initial dimensions of the screen buffer, in characters.
// - coordViewportSize: The initial dimensions of the viewport, in characters.
// Return Value:
// - <none>
void SCREEN_INFORMATION::_InitializeBufferDimensions(const COORD coordScreenBufferSize,
                                                     const COORD coordViewportSize)
{
    _viewport = Viewport::FromDimensions({0, 0},
                                         _IsInPtyMode() ? coordScreenBufferSize : coordViewportSize);

    SetScreenBufferSize(coordScreenBufferSize);
}

// Method Description:
// - Sets up the Output state machine to be in pty mode. Sequences it doesn't
//      understand will be written to tthe pTtyConnection passed in here.
// Arguments:
// - pTtyConnection: This is a TerminaOutputConnection that we can write the
//      sequence we didn't understand to.
// Return Value:
// - <none>
void SCREEN_INFORMATION::SetTerminalConnection(_In_ ITerminalOutputConnection* const pTtyConnection)
{
    _pEngine->SetTerminalConnection(pTtyConnection,
                                    std::bind(&StateMachine::FlushToTerminal, _pStateMachine));
}

// Routine Description:
// - Reads text from the output buffer. will contain text as it would look at the terminal (no duplicate wide chars)
// Arguments:
// - rowIndex - the index of the row to read the text for
// Return Value:
// - wstring containing row's text
std::wstring SCREEN_INFORMATION::ReadText(const size_t rowIndex) const
{
    const ROW& row = _textBuffer->GetRowByOffset(rowIndex);
    return row.GetText();
}

// Routine Description:
// - Reads cells from the output buffer.
// Arguments:
// - rowIndex - the index of the row to read the text for
// Return Value:
// - vector of cells containing row's data.
std::vector<OutputCell> SCREEN_INFORMATION::ReadLine(const size_t rowIndex) const
{
    const ROW& row = _textBuffer->GetRowByOffset(rowIndex);
    return row.AsCells();
}

// Routine Description:
// - Reads cells from the output buffer.
// Arguments:
// - rowIndex - the index of the row to read the text for
// - startIndex - column in the row to start reading cells from
// Return Value:
// - vector of cells containing row's data.
std::vector<OutputCell> SCREEN_INFORMATION::ReadLine(const size_t rowIndex,
                                                     const size_t startIndex) const
{
    const ROW& row = _textBuffer->GetRowByOffset(rowIndex);
    return row.AsCells(startIndex);
}

// Routine Description:
// - Reads cells from the output buffer.
// Arguments:
// - rowIndex - the index of the row to read the text for
// - startIndex - column in the row to start reading cells from
// - count - the number of cells to read
// Return Value:
// - vector of cells containing row's data.
std::vector<OutputCell> SCREEN_INFORMATION::ReadLine(const size_t rowIndex,
                                                     const size_t startIndex,
                                                     const size_t count) const
{
    const ROW& row = _textBuffer->GetRowByOffset(rowIndex);
    return row.AsCells(startIndex, count);
}

// Routine Description:
// - writes cells to the output buffer.
// Arguments:
// - cells - the cells to write to the output buffer
// - rowIndex - the index of the row to write the text to
// - startIndex - column in the row to start writing cells to
// Return Value:
// - the number of cells written
size_t SCREEN_INFORMATION::WriteLine(const std::vector<OutputCell>& cells,
                                     const size_t rowIndex,
                                     const size_t startIndex)
{
    return _WriteLine(cells, rowIndex, startIndex, true);
}

// Routine Description:
// - writes cells to the output buffer. will stop at the end of the screen buffer
// Arguments:
// - cells - the cells to write to the output buffer
// - rowIndex - the index of the row to write the text to
// - startIndex - column in the row to start writing cells to
// Return Value:
// - the number of cells written
size_t SCREEN_INFORMATION::WriteLineNoWrap(const std::vector<OutputCell>& cells,
                                           const size_t rowIndex,
                                           const size_t startIndex)
{
    return _WriteLine(cells, rowIndex, startIndex, false);
}

// Routine Description:
// - writes cells to the output buffer.
// Arguments:
// - cells - the cells to write to the output buffer
// - rowIndex - the index of the row to write the text to
// - startIndex - column in the row to start writing cells to
// - shouldWrap - whether text writing should wrap around the text buffer or not
// Return Value:
// - the number of cells written
size_t SCREEN_INFORMATION::_WriteLine(const std::vector<OutputCell>& cells,
                                      const size_t rowIndex,
                                      const size_t startIndex,
                                      const bool shouldWrap)
{
    auto it = cells.begin();
    auto currentColumn = startIndex;
    size_t amountWritten = 0;
    auto currentRowIndex = rowIndex;
    while (it != cells.end())
    {
        if (!shouldWrap && currentRowIndex >= static_cast<size_t>(_textBuffer->TotalRowCount()))
        {
            break;
        }

        ROW& row = _textBuffer->GetRowByOffset(currentRowIndex);
        const auto newIt = row.WriteCells(it, cells.end(), currentColumn);
        amountWritten += newIt - it;
        it = newIt;
        row.GetCharRow().SetWrapForced(it != cells.end());
        currentColumn = 0;
        ++currentRowIndex;
    }
    return amountWritten;
}

// Routine Description:
// - fills a text attribute a number of times.
// Arguments:
// - attr - the text attribute to fill
// - target - the starting location of the fill
// - amountToWrite - the number of cells to fill
// Return Value:
// - the number of cells filled
size_t SCREEN_INFORMATION::FillTextAttribute(const TextAttribute attr,
                                             const COORD target,
                                             const size_t amountToWrite)
{
    const COORD coordScreenBufferSize = GetScreenBufferSize();
    std::vector<TextAttributeRun> attrRun{ {} };
    // Here we're being a little clever -
    // Because RGB color can't roundtrip the API, certain VT sequences will forget the RGB color
    // because their first call to GetScreenBufferInfo returned a legacy attr.
    // If they're calling this with the default attrs, they likely wanted to use the RGB default attrs.
    // This could create a scenario where someone emitted RGB with VT,
    // THEN used the API to FillConsoleOutput with the default attrs, and DIDN'T want the RGB color
    // they had set.
    if (InVTMode() && GetAttributes().GetLegacyAttributes() == attr.GetLegacyAttributes())
    {
        attrRun.front().SetAttributes(GetAttributes());
    }
    else
    {
        WORD actualAttr = attr.GetLegacyAttributes();
        ClearAllFlags(actualAttr, COMMON_LVB_SBCSDBCS);
        attrRun.front().SetAttributesFromLegacy(actualAttr);
    }

    COORD currentLocation = target;
    size_t amountWritten = 0;
    do
    {
        const size_t rowIndex = gsl::narrow<size_t>(currentLocation.Y);
        const size_t writesLeft = amountToWrite - amountWritten;
        const size_t columnsToFill = std::min(writesLeft, gsl::narrow<size_t>(coordScreenBufferSize.X - currentLocation.X));
        ROW& row = _textBuffer->GetRowByOffset(currentLocation.Y);
        ATTR_ROW& attrRow = row.GetAttrRow();

        attrRun.front().SetLength(columnsToFill);
        THROW_IF_FAILED(attrRow.InsertAttrRuns(attrRun,
                                               currentLocation.X,
                                               currentLocation.X + columnsToFill - 1,
                                               coordScreenBufferSize.X));
        amountWritten += columnsToFill;
        currentLocation.X = 0;
        currentLocation.Y++;
    }
    while (amountWritten < amountToWrite && IsCoordInBounds(currentLocation, coordScreenBufferSize));

    NotifyAccessibilityEventing(target.X, target.Y, currentLocation.X, currentLocation.Y);
    UpdateScreen(target, currentLocation);
    return amountWritten;
}

size_t SCREEN_INFORMATION::FillTextGlyph(const std::vector<wchar_t>& glyph,
                                         const COORD target,
                                         const size_t amountToWrite)
{
    const COORD coordScreenBufferSize = GetScreenBufferSize();

    const OutputCell cell{ glyph, {}, OutputCell::TextAttributeBehavior::Current };
    std::vector<OutputCell> cellsToWrite{ cell };
    if (IsGlyphFullWidth(glyph))
    {
        cellsToWrite.push_back(cell);
        cellsToWrite.at(0).DbcsAttr().SetLeading();
        cellsToWrite.at(1).DbcsAttr().SetTrailing();
    }

    auto currentLocation = target;
    CleanupDbcsEdgesForWrite(amountToWrite, currentLocation, *this);
    size_t amountWritten = 0;


    do
    {
        const size_t writesLeft = amountToWrite - amountWritten;
        const size_t columnsToFill = std::min(writesLeft, gsl::narrow<size_t>(coordScreenBufferSize.X - currentLocation.X));
        auto& row = _textBuffer->GetRowByOffset(currentLocation.Y);
        const RepeatingIterator<OutputCell, decltype(cellsToWrite)::const_iterator>  startingIt{ cellsToWrite.begin(),
                                                                                                 cellsToWrite.end() };
        const auto it = row.WriteCells(startingIt, startingIt + columnsToFill, currentLocation.X);
        const short distance = gsl::narrow<short>(std::distance( startingIt, it));
        currentLocation.X += distance;
        amountWritten += distance;

        // invalidate row wrap status for any bulk fill of text characters
        row.GetCharRow().SetWrapForced(false);

        if (!IsCoordInBounds(currentLocation, coordScreenBufferSize))
        {
            currentLocation.Y++;
            currentLocation.X = 0;
        }
    }
    while (amountWritten < amountToWrite && IsCoordInBounds(currentLocation, coordScreenBufferSize));

    NotifyAccessibilityEventing(target.X, target.Y, currentLocation.X, currentLocation.Y);
    UpdateScreen(target, currentLocation);
    return amountWritten;
}

void SCREEN_INFORMATION::UpdateScreen(const COORD start, const COORD end)
{
    const COORD coordScreenBufferSize = GetScreenBufferSize();

    SMALL_RECT rect;
    if (ConvScreenInfo)
    {
        auto& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
        const auto viewport = GetBufferViewport();
        const auto convCoord = ConvScreenInfo->GetAreaBufferInfo().coordConView;

        rect.Top = start.Y + viewport.Left + convCoord.Y;
        rect.Bottom = end.Y + viewport.Left + convCoord.Y;
        if (start.Y == end.Y)
        {
            rect.Left = start.X + viewport.Top + convCoord.X;
            rect.Right = end.X + viewport.Top + convCoord.X;
        }
        else
        {
            rect.Left = 0;
            rect.Right = gci.GetActiveOutputBuffer().GetScreenBufferSize().X - 1;
        }
        WriteConvRegionToScreen(gci.GetActiveOutputBuffer(), rect);
    }
    else
    {
        rect.Top = start.Y;
        rect.Bottom = end.Y;
        if (start.Y == end.Y)
        {
            rect.Left = start.X;
            rect.Right = end.X - 1;
        }
        else
        {
            rect.Left = 0;
            rect.Right = coordScreenBufferSize.X - 1;
        }
        WriteToScreen(*this, rect);
    }
}

// Routine Description:
// - Clears out the entire text buffer with the default character and
//   the current default attribute applied to this screen.
void SCREEN_INFORMATION::ClearTextData()
{
    // Get attribute to clear with
    const auto attribute = GetAttributes();

    // Clear the text buffer.
    _textBuffer->Reset(attribute);
}

// Routine Description:
// - finds the boundaries of the word at the given position on the screen
// Arguments:
// - position - location on the screen to get the word boundary for
// Return Value:
// - word boundary positions
std::pair<COORD, COORD> SCREEN_INFORMATION::GetWordBoundary(const COORD position) const
{
    const ROW& row = _textBuffer->GetRowByOffset(position.Y);
    const COORD screenBufferSize = GetScreenBufferSize();
    COORD start{ position };
    COORD end{ position };

    // find the start of the word
    while (start.X > 0)
    {
        if (IsWordDelim(row.at(start.X - 1).Chars()))
        {
            break;
        }
        start.X--;
    }

    // find the end of the word
    while (end.X < screenBufferSize.X)
    {
        if (IsWordDelim(row.at(end.X).Chars()))
        {
            break;
        }
        end.X++;
    }

    // trim leading zeros if we need to
    const auto& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    if (gci.GetTrimLeadingZeros())
    {
        // Trim the leading zeros: 000fe12 -> fe12, except 0x and 0n.
        // Useful for debugging
        const auto& glyph = row.at(start.X + 1).Chars();
        if (glyph.size() == 1)
        {
            const wchar_t wch = glyph.front();
            if (end.X > start.X + 2 &&
                wch != L'x' &&
                wch != L'X' &&
                wch != L'n')
            {
                // Don't touch the selection begins with 0x
                while (row.at(start.X).Chars().size() == 1 &&
                       row.at(start.X).Chars().front() == L'0' &&
                       start.X < end.X - 1)
                {
                    start.X++;
                }
            }
        }
    }
    return { start, end };
}

TextBuffer& SCREEN_INFORMATION::GetTextBuffer() noexcept
{
    return *_textBuffer;
}

const TextBuffer& SCREEN_INFORMATION::GetTextBuffer() const noexcept
{
    return *_textBuffer;
}

ScreenInfoTextIterator SCREEN_INFORMATION::GetTextDataAt(const COORD at) const
{
    return ScreenInfoTextIterator(this, at);
}
