/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "_output.h"
#include "stream.h"
#include "scrolling.hpp"
#include "Ucs2CharRow.hpp"

#include "..\interactivity\inc\ServiceLocator.hpp"

Selection::Selection() :
    _fSelectionVisible(false),
    _ulSavedCursorSize(0),
    _fSavedCursorVisible(false),
    _dwSelectionFlags(0)
{
    ZeroMemory((void*)&_srSelectionRect, sizeof(_srSelectionRect));
    ZeroMemory((void*)&_coordSelectionAnchor, sizeof(_coordSelectionAnchor));
    ZeroMemory((void*)&_coordSavedCursorPosition, sizeof(_coordSavedCursorPosition));
}

Selection::~Selection()
{

}

Selection& Selection::Instance()
{
    static Selection s;
    return s;
}

// Routine Description:
// - Detemines the line-by-line selection rectangles based on global selection state.
// Arguments:
// - <input read from globals>
// - Writes pointer to array of small rectangles representing selection region of each row
//   CALLER MUST FREE THIS ARRAY.
// - Writes pointer to count of rectangles in array
// Return Value:
// - Success if success. Invalid parameter if global state is incorrect. No memory if out of memory.
[[nodiscard]]
NTSTATUS Selection::GetSelectionRects(_Outptr_result_buffer_all_(*pcRectangles) SMALL_RECT** const prgsrSelection,
                                      _Out_ UINT* const pcRectangles) const
{
    const CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    NTSTATUS status = STATUS_SUCCESS;
    SMALL_RECT* rgsrSelection = nullptr;
    *prgsrSelection = nullptr;
    *pcRectangles = 0;

    if (!_fSelectionVisible)
    {
        return status;
    }

    if (NT_SUCCESS(status))
    {
        const PSCREEN_INFORMATION pScreenInfo = gci.CurrentScreenBuffer;

        const SMALL_RECT * const pSelectionRect = &_srSelectionRect;
        const UINT cRectangles = pSelectionRect->Bottom - pSelectionRect->Top + 1;

        if (NT_SUCCESS(status))
        {
            rgsrSelection = new SMALL_RECT[cRectangles];

            status = NT_TESTNULL(rgsrSelection);

            if (NT_SUCCESS(status))
            {
                bool fLineSelection = IsLineSelection();

                // if the anchor (start of select) was in the top right or bottom left of the box,
                // we need to remove rectangular overlap in the middle.
                // e.g.
                // For selections with the anchor in the top left (A) or bottom right (B),
                // it is valid to maintain the inner rectangle (+) as part of the selection
                //               A+++++++================
                // ==============++++++++B
                // + and = are valid highlights in this scenario.
                // For selections with the anchor in in the top right (A) or bottom left (B),
                // we must remove a portion of the first/last line that lies within the rectangle (+)
                //               +++++++A=================
                // ==============B+++++++
                // Only = is valid for highlight in this scenario.
                // This is only needed for line selection. Box selection doesn't need to account for this.

                bool fRemoveRectPortion = false;

                if (fLineSelection)
                {
                    const COORD coordSelectStart = _coordSelectionAnchor;

                    // only if top and bottom aren't the same line... we need the whole rectangle if we're on the same line.
                    // e.g.         A++++++++++++++B
                    // All the + are valid select points.
                    if (pSelectionRect->Top != pSelectionRect->Bottom)
                    {
                        if ((coordSelectStart.X == pSelectionRect->Right && coordSelectStart.Y == pSelectionRect->Top) ||
                            (coordSelectStart.X == pSelectionRect->Left && coordSelectStart.Y == pSelectionRect->Bottom))
                        {
                            fRemoveRectPortion = true;
                        }
                    }
                }

                UINT iFinal = 0; // index for storing into final array

                // for each row within the selection rectangle
                for (short iRow = pSelectionRect->Top; iRow <= pSelectionRect->Bottom; iRow++)
                {
                    // create a rectangle representing the highlight on one row
                    SMALL_RECT srHighlightRow;

                    srHighlightRow.Top = iRow;
                    srHighlightRow.Bottom = iRow;
                    srHighlightRow.Left = pSelectionRect->Left;
                    srHighlightRow.Right = pSelectionRect->Right;

                    // compensate for line selection by extending one or both ends of the rectangle to the edge
                    if (fLineSelection)
                    {
                        // if not the first row, pad the left selection to the buffer edge
                        if (iRow != pSelectionRect->Top)
                        {
                            srHighlightRow.Left = 0;
                        }

                        // if not the last row, pad the right selection to the buffer edge
                        if (iRow != pSelectionRect->Bottom)
                        {
                            srHighlightRow.Right = pScreenInfo->TextInfo->GetCoordBufferSize().X - 1;
                        }

                        // if we've determined we're in a scenario where we must remove the inner rectangle from the lines...
                        if (fRemoveRectPortion)
                        {
                            if (iRow == pSelectionRect->Top)
                            {
                                // from the top row, move the left edge of the highlight line to the right edge of the rectangle
                                srHighlightRow.Left = pSelectionRect->Right;
                            }
                            else if (iRow == pSelectionRect->Bottom)
                            {
                                // from the bottom row, move the right edge of the highlight line to the left edge of the rectangle
                                srHighlightRow.Right = pSelectionRect->Left;
                            }
                        }
                    }

                    // compensate for double width characters by calling double-width measuring/limiting function
                    COORD coordTargetPoint;
                    coordTargetPoint.X = srHighlightRow.Left;
                    coordTargetPoint.Y = srHighlightRow.Top;
                    SHORT sStringLength = srHighlightRow.Right - srHighlightRow.Left + 1;
                    s_BisectSelection(sStringLength, coordTargetPoint, pScreenInfo, &srHighlightRow);

                    rgsrSelection[iFinal].Left = srHighlightRow.Left;
                    rgsrSelection[iFinal].Right = srHighlightRow.Right;
                    rgsrSelection[iFinal].Top = srHighlightRow.Top;
                    rgsrSelection[iFinal].Bottom = srHighlightRow.Bottom;

                    iFinal++;
                }

                // validate that we wrote the number of rows we expected to write
                if (iFinal != cRectangles)
                {
                    status = STATUS_UNSUCCESSFUL;
                }
            }
        }

        if (NT_SUCCESS(status))
        {
            *prgsrSelection = rgsrSelection;
            *pcRectangles = cRectangles;
        }
        else
        {
            if (rgsrSelection != nullptr)
            {
                delete[] rgsrSelection;
            }
        }
    }

    return status;
}

// Routine Description:
// - This routine checks to ensure that clipboard selection isn't trying to cut a double byte character in half.
//   It will adjust the SmallRect rectangle size to ensure this.
// Arguments:
// - sStringLength - The length of the string we're attempting to clip.
// - coordTargetPoint - The row/column position within the text buffer that we're about to try to clip.
// - pScreenInfo - Screen information structure containing relevant text and dimension information.
// - pSmallRect - The region of the text that we want to clip, and then adjusted to the region that should be clipped without splicing double-width characters.
// Return Value:
//  <none>
void Selection::s_BisectSelection(_In_ short const sStringLength,
                                  _In_ COORD const coordTargetPoint,
                                  _In_ const SCREEN_INFORMATION* const pScreenInfo,
                                  _Inout_ SMALL_RECT* const pSmallRect)
{
    const TEXT_BUFFER_INFO* const pTextInfo = pScreenInfo->TextInfo;
    const ROW& Row = pTextInfo->GetRowByOffset(coordTargetPoint.Y);

    try
    {
        const ICharRow& iCharRow = Row.GetCharRow();
        // we only support ucs2 encoded char rows
        FAIL_FAST_IF_MSG(iCharRow.GetSupportedEncoding() != ICharRow::SupportedEncoding::Ucs2,
                        "only support UCS2 char rows currently");

        const Ucs2CharRow& charRow = static_cast<const Ucs2CharRow&>(iCharRow);
        // Check start position of strings
        if (charRow.GetAttribute(coordTargetPoint.X).IsTrailing())
        {
            if (coordTargetPoint.X == 0)
            {
                pSmallRect->Left++;
            }
            else
            {
                pSmallRect->Left--;
            }
        }

        // Check end position of strings
        if (coordTargetPoint.X + sStringLength < pScreenInfo->GetScreenBufferSize().X)
        {
            if (charRow.GetAttribute(coordTargetPoint.X + sStringLength).IsTrailing())
            {
                pSmallRect->Right++;
            }
        }
        else
        {
            const ROW& RowNext = pTextInfo->GetNextRowNoWrap(Row);
            const ICharRow& iCharRowNext = RowNext.GetCharRow();
            // we only support ucs2 encoded char rows
            FAIL_FAST_IF_MSG(iCharRow.GetSupportedEncoding() != ICharRow::SupportedEncoding::Ucs2,
                            "only support UCS2 char rows currently");

            const Ucs2CharRow& charRowNext = static_cast<const Ucs2CharRow&>(iCharRowNext);
            if (charRowNext.GetAttribute(0).IsTrailing())
            {
                pSmallRect->Right--;
            }
        }
    }
    catch (...)
    {
        LOG_HR(wil::ResultFromCaughtException());
    }
}


// Routine Description:
// - Shows the selection area in the window if one is available and not already showing.
// Arguments:
//  <none>
// Return Value:
//  <none>
void Selection::ShowSelection()
{
    _SetSelectionVisibility(true);
}

// Routine Description:
// - Hides the selection area in the window if one is available and already showing.
// Arguments:
//  <none>
// Return Value:
//  <none>
void Selection::HideSelection()
{
    _SetSelectionVisibility(false);
}

// Routine Description:
// - Changes the visibility of the selection area on the screen.
// - Used to turn the selection area on or off.
// Arguments:
// - fMakeVisible - If TRUE, we're turning the selection ON.
//                - If FALSE, we're turning the selection OFF.
// Return Value:
//   <none>
void Selection::_SetSelectionVisibility(_In_ bool const fMakeVisible)
{
    if (IsInSelectingState() && IsAreaSelected())
    {
        if (fMakeVisible == _fSelectionVisible)
        {
            return;
        }

        _fSelectionVisible = fMakeVisible;

        _PaintSelection();
    }
}

// Routine Description:
//  - Inverts the selected region on the current screen buffer.
//  - Reads the selected area, selection mode, and active screen buffer
//    from the global properties and dispatches a GDI invert on the selected text area.
// Arguments:
//  - <none>
// Return Value:
//  - <none>
void Selection::_PaintSelection() const
{
    if (ServiceLocator::LocateGlobals().pRender != nullptr)
    {
        ServiceLocator::LocateGlobals().pRender->TriggerSelection();
    }
}

// Routine Description:
// - Starts the selection with the given initial position
// Arguments:
// - coordBufferPos - Position in which user started a selection
// Return Value:
// - <none>
void Selection::InitializeMouseSelection(_In_ const COORD coordBufferPos)
{
    Scrolling::s_ClearScroll();

    // set flags
    _SetSelectingState(true);
    _dwSelectionFlags = CONSOLE_MOUSE_SELECTION | CONSOLE_SELECTION_NOT_EMPTY;

    // store anchor and rectangle of selection
    _coordSelectionAnchor = coordBufferPos;

    // since we've started with just a point, the rectangle is 1x1 on the point given
    SMALL_RECT* const psrSelRect = &_srSelectionRect;
    psrSelRect->Left = psrSelRect->Right = coordBufferPos.X;
    psrSelRect->Top = psrSelRect->Bottom = coordBufferPos.Y;

    // Check for ALT-Mouse Down "use alternate selection"
    // If in box mode, use line mode. If in line mode, use box mode.
    CheckAndSetAlternateSelection();

    // set window title to mouse selection mode
    { // Scope to control service locator lock/unlock lifetime
        auto pWindow = ServiceLocator::LocateConsoleWindow();
        if (pWindow.get() != nullptr)
        {
            pWindow->UpdateWindowText();
        }
    }

    // Fire off an event to let accessibility apps know the selection has changed.

    ServiceLocator::LocateAccessibilityNotifier()->NotifyConsoleCaretEvent(IAccessibilityNotifier::ConsoleCaretEventFlags::CaretSelection, PACKCOORD(coordBufferPos));
}

// Routine Description:
// - Modifies both ends of the current selection.
// - Intended for use with functions that help auto-complete a selection area (e.g. double clicking)
// Arguments:
// - coordSelectionStart - Replaces the selection anchor, a.k.a. where the selection started from originally.
// - coordSelectionEnd - The linear final position or opposite corner of the anchor to represent the complete selection area.
// Return Value:
// - <none>
void Selection::AdjustSelection(_In_ const COORD coordSelectionStart, _In_ const COORD coordSelectionEnd)
{
    // modify the anchor and then just use extend to adjust the other portion of the selection rectangle
    _coordSelectionAnchor = coordSelectionStart;
    ExtendSelection(coordSelectionEnd);
}

// Routine Description:
// - Extends the selection out to the given position from the initial anchor point.
//   This means that a coordinate farther away will make the rectangle larger and a closer one will shrink it.
// Arguments:
// - coordBufferPos - Position to extend/contract the current selection up to.
// Return Value:
// - <none>
void Selection::ExtendSelection(_In_ COORD coordBufferPos)
{
    const CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    SCREEN_INFORMATION* pScreenInfo = gci.CurrentScreenBuffer;

    // ensure position is within buffer bounds. Not less than 0 and not greater than the screen buffer size.
    pScreenInfo->ClipToScreenBuffer(&coordBufferPos);

    if (!IsAreaSelected())
    {
        // we should only be extending a selection that has no area yet if we're coming from mark mode
        ASSERT(!IsMouseInitiatedSelection());

        // scroll if necessary to make cursor visible.
        pScreenInfo->MakeCursorVisible(coordBufferPos);

        _dwSelectionFlags |= CONSOLE_SELECTION_NOT_EMPTY;
        _srSelectionRect.Left = _srSelectionRect.Right = _coordSelectionAnchor.X;
        _srSelectionRect.Top = _srSelectionRect.Bottom = _coordSelectionAnchor.Y;

        ShowSelection();
    }
    else
    {
        // scroll if necessary to make cursor visible.
        pScreenInfo->MakeCursorVisible(coordBufferPos);
    }

    // remember previous selection rect
    SMALL_RECT srNewSelection = _srSelectionRect;

    // update selection rect
    // this adjusts the rectangle dimensions based on which way the move was requested
    // in respect to the original selection position (the anchor)
    if (coordBufferPos.X <= _coordSelectionAnchor.X)
    {
        srNewSelection.Left = coordBufferPos.X;
        srNewSelection.Right = _coordSelectionAnchor.X;
    }
    else if (coordBufferPos.X > _coordSelectionAnchor.X)
    {
        srNewSelection.Right = coordBufferPos.X;
        srNewSelection.Left = _coordSelectionAnchor.X;
    }
    if (coordBufferPos.Y <= _coordSelectionAnchor.Y)
    {
        srNewSelection.Top = coordBufferPos.Y;
        srNewSelection.Bottom = _coordSelectionAnchor.Y;
    }
    else if (coordBufferPos.Y > _coordSelectionAnchor.Y)
    {
        srNewSelection.Bottom = coordBufferPos.Y;
        srNewSelection.Top = _coordSelectionAnchor.Y;
    }

    // call special update method to modify the displayed selection in-place
    // NOTE: Using HideSelection, editing the rectangle, then ShowSelection will cause flicker.
    //_PaintUpdateSelection(&srNewSelection);
    _srSelectionRect = srNewSelection;
    _PaintSelection();

    // Fire off an event to let accessibility apps know the selection has changed.
    ServiceLocator::LocateAccessibilityNotifier()->NotifyConsoleCaretEvent(IAccessibilityNotifier::ConsoleCaretEventFlags::CaretSelection, PACKCOORD(coordBufferPos));
}

// Routine Description:
// - Cancels any mouse selection state to return to normal mode.
// Arguments:
// - <none> (Uses global state)
// Return Value:
// - <none>
void Selection::_CancelMouseSelection()
{
    const CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    SCREEN_INFORMATION* pScreenInfo = gci.CurrentScreenBuffer;

    // invert old select rect.  if we're selecting by mouse, we
    // always have a selection rect.
    HideSelection();

    // turn off selection flag
    _SetSelectingState(false);

    { // Scope to control service locator lock/unlock lifetime
        auto pWindow = ServiceLocator::LocateConsoleWindow();
        if (pWindow.get() != nullptr)
        {
            pWindow->UpdateWindowText();
        }
    }

    // Mark the cursor position as changed so we'll fire off a win event.
    pScreenInfo->TextInfo->GetCursor()->SetHasMoved(TRUE);
}

// Routine Description:
// - Cancels any mark mode selection state to return to normal mode.
// Arguments:
// - <none>
// Return Value:
// - <none>
void Selection::_CancelMarkSelection()
{
    const CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    SCREEN_INFORMATION* pScreenInfo = gci.CurrentScreenBuffer;

    // Hide existing selection, if we have one.
    if (IsAreaSelected())
    {
        HideSelection();
    }

    // Turn off selection flag.
    _SetSelectingState(false);

    { // Scope to control service locator lock/unlock lifetime
        auto pWindow = ServiceLocator::LocateConsoleWindow();
        if (pWindow.get() != nullptr)
        {
            pWindow->UpdateWindowText();
        }
    }

    // restore text cursor
    _RestoreCursorData(pScreenInfo);
}

// Routine Description:
// - If a selection exists, clears it and restores the state.
//   Will also unblock a blocked write if one exists.
// Arguments:
// - <none> (Uses global state)
// Return Value:
// - <none>
void Selection::ClearSelection()
{
    ClearSelection(false);
}

// Routine Description:
// - If a selection exists, clears it and restores the state.
// - Will only unblock a write if not starting a new selection.
// Arguments:
// - fStartingNewSelection - If we're going to start another selection right away, we'll keep the write blocked.
// Return Value:
// - <none>
void Selection::ClearSelection(_In_ bool const fStartingNewSelection)
{
    if (IsInSelectingState())
    {
        if (IsMouseInitiatedSelection())
        {
            _CancelMouseSelection();
        }
        else
        {
            _CancelMarkSelection();
        }

        _dwSelectionFlags = 0;

        // If we were using alternate selection, cancel it here before starting a new area.
        _fUseAlternateSelection = false;

        // Only unblock if we're not immediately starting a new selection. Otherwise stay blocked.
        if (!fStartingNewSelection)
        {
            UnblockWriteConsole(CONSOLE_SELECTING);
        }
    }
}

// Routine Description:
// - Colors all text in the given rectangle with the color attribute provided.
// - This does not validate whether there is a valid selection right now or not.
//   It is assumed to already be in a proper selecting state and the given rectangle should be highlighted with the given color unconditionally.
// Arguments:
// - psrRect - Rectangular area to fill with color
// - ulAttr - The color attributes to apply
// Return Value:
// - <none>
void Selection::ColorSelection(_In_ SMALL_RECT* const psrRect, _In_ ULONG const ulAttr)
{
    const CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    // TODO: psrRect should likely one day be replaced with an array of rectangles (in case we have a line selection we want colored)
    ASSERT(ulAttr <= 0xff);

    // Read selection rectangle, assumed already clipped to buffer.
    SCREEN_INFORMATION* pScreenInfo = gci.CurrentScreenBuffer;

    COORD coordTargetSize;
    coordTargetSize.X = CalcWindowSizeX(&_srSelectionRect);
    coordTargetSize.Y = CalcWindowSizeY(&_srSelectionRect);

    COORD coordTarget;
    coordTarget.X = psrRect->Left;
    coordTarget.Y = psrRect->Top;

    // Now color the selection a line at a time.
    for (; (coordTarget.Y < psrRect->Top + coordTargetSize.Y); ++coordTarget.Y)
    {
        DWORD cchWrite = coordTargetSize.X;

        LOG_IF_FAILED(FillOutput(pScreenInfo, (USHORT)ulAttr, coordTarget, CONSOLE_ATTRIBUTE, &cchWrite));
    }
}

// Routine Description:
// - Enters mark mode selection. Prepares the cursor to move around to select a region and sets up state variables.
// Arguments:
// - <none>
// Return Value:
// - <none>
void Selection::InitializeMarkSelection()
{
    const CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    // clear any existing selection.
    ClearSelection(true);

    Scrolling::s_ClearScroll();

    // set flags
    _SetSelectingState(true);
    _dwSelectionFlags = 0;

    // save old cursor position and make console cursor into selection cursor.
    SCREEN_INFORMATION* pScreenInfo = gci.CurrentScreenBuffer;
    _SaveCursorData(pScreenInfo->TextInfo);
    Cursor* const pCursor = pScreenInfo->TextInfo->GetCursor();
    pScreenInfo->SetCursorInformation(100, TRUE, pCursor->GetColor(), pCursor->GetType());

    const COORD coordPosition = pCursor->GetPosition();
    LOG_IF_FAILED(pScreenInfo->SetCursorPosition(coordPosition, TRUE));

    // set the cursor position as the anchor position
    // it will get updated as the cursor moves for mark mode,
    // but it serves to prepare us for the inevitable start of the selection with Shift+Arrow Key
    _coordSelectionAnchor = coordPosition;

    // set frame title text
    auto pWindow = ServiceLocator::LocateConsoleWindow();
    if (pWindow.get() != nullptr)
    {
        pWindow->UpdateWindowText();
    }
}

// Routine Description:
// - Resets the current selection and selects a new region from the start to end coordinates
// Arguments:
// - coordStart - Position to start selection area from
// - coordEnd - Position to select up to
// Return Value:
// - <none>
void Selection::SelectNewRegion(_In_ COORD const coordStart, _In_ COORD const coordEnd)
{
    // clear existing selection if applicable
    ClearSelection();

    // initialize selection
    InitializeMouseSelection(coordStart);

    ShowSelection();

    // extend selection
    ExtendSelection(coordEnd);
}

// Routine Description:
// - Creates a new selection region of "all" available text.
//   The meaning of "all" can vary. If we have input text, then "all" is just the input text.
//   If we have no input text, "all" is the entire valid screen buffer (output text and the prompt)
// Arguments:
// - <none> (Uses global state)
// Return Value:
// - <none>
void Selection::SelectAll()
{
    const CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    // save the old window position
    SCREEN_INFORMATION* pScreenInfo = gci.CurrentScreenBuffer;

    COORD coordWindowOrigin;
    coordWindowOrigin.X = pScreenInfo->GetBufferViewport().Left;
    coordWindowOrigin.Y = pScreenInfo->GetBufferViewport().Top;

    // Get existing selection rectangle parameters
    const bool fOldSelectionExisted = IsAreaSelected();
    const SMALL_RECT srOldSelection = _srSelectionRect;
    const COORD coordOldAnchor = _coordSelectionAnchor;

    // Attempt to get the boundaries of the current input line.
    COORD coordInputStart;
    COORD coordInputEnd;
    const bool fHasInputArea = s_GetInputLineBoundaries(&coordInputStart, &coordInputEnd);

    // These variables will be used to specify the new selection area when we're done
    COORD coordNewSelStart;
    COORD coordNewSelEnd;

    // Now evaluate conditions and attempt to assign a new selection area.
    if (!fHasInputArea)
    {
        // If there's no input area, just select the entire valid text region.
        GetValidAreaBoundaries(&coordNewSelStart, &coordNewSelEnd);
    }
    else
    {
        if (!fOldSelectionExisted)
        {
            // Temporary workaround until MSFT: 614579 is completed.
            SMALL_RECT srEdges;
            Utils::s_GetCurrentBufferEdges(&srEdges);
            COORD coordOneAfterEnd = coordInputEnd;
            Utils::s_DoIncrementScreenCoordinate(srEdges, &coordOneAfterEnd);

            if (s_IsWithinBoundaries(pScreenInfo->TextInfo->GetCursor()->GetPosition(), coordInputStart, coordInputEnd))
            {
                // If there was no previous selection and the cursor is within the input line, select the input line only
                coordNewSelStart = coordInputStart;
                coordNewSelEnd = coordInputEnd;
            }
            else if (s_IsWithinBoundaries(pScreenInfo->TextInfo->GetCursor()->GetPosition(), coordOneAfterEnd, coordOneAfterEnd))
            {
                // Temporary workaround until MSFT: 614579 is completed.
                // Select only the input line if the cursor is one after the final position of the input line.
                coordNewSelStart = coordInputStart;
                coordNewSelEnd = coordInputEnd;
            }
            else
            {
                // otherwise if the cursor is elsewhere, select everything
                GetValidAreaBoundaries(&coordNewSelStart, &coordNewSelEnd);
            }
        }
        else
        {
            // This is the complex case. We had an existing selection and we have an input area.

            // To figure this out, we need the anchor (the point where the selection starts) and its opposite corner
            COORD coordOldAnchorOpposite;
            Utils::s_GetOppositeCorner(srOldSelection, coordOldAnchor, &coordOldAnchorOpposite);

            // Check if both anchor and opposite corner fall within the input line
            const bool fIsOldSelWithinInput =
                s_IsWithinBoundaries(coordOldAnchor, coordInputStart, coordInputEnd) &&
                s_IsWithinBoundaries(coordOldAnchorOpposite, coordInputStart, coordInputEnd);

            // Check if both anchor and opposite corner are exactly the bounds of the input line
            const bool fAllInputSelected =
                ((Utils::s_CompareCoords(coordInputStart, coordOldAnchor) == 0 && Utils::s_CompareCoords(coordInputEnd, coordOldAnchorOpposite) == 0) ||
                 (Utils::s_CompareCoords(coordInputStart, coordOldAnchorOpposite) == 0 && Utils::s_CompareCoords(coordInputEnd, coordOldAnchor) == 0));

            if (fIsOldSelWithinInput && !fAllInputSelected)
            {
                // If it's within the input area and the whole input is not selected, then select just the input
                coordNewSelStart = coordInputStart;
                coordNewSelEnd = coordInputEnd;
            }
            else
            {
                // Otherwise just select the whole valid area
                GetValidAreaBoundaries(&coordNewSelStart, &coordNewSelEnd);
            }
        }
    }

    // If we're in box selection, adjust end coordinate to end of line and start coordinate to start of line
    // or it won't be selecting all the text.
    if (!IsLineSelection())
    {
        coordNewSelStart.X = 0;
        coordNewSelEnd.X = pScreenInfo->GetScreenBufferSize().X - 1;
    }

    SelectNewRegion(coordNewSelStart, coordNewSelEnd);

    // restore the old window position
    LOG_IF_FAILED(pScreenInfo->SetViewportOrigin(TRUE, coordWindowOrigin));
}
