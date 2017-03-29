/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"
#include "selection.hpp"

#include "globals.h"

#include "cursor.h"

// Routine Description:
// - Determines whether the console is in a selecting state
// Arguments:
// - <none> (gets global state)
// Return Value:
// - True if the console is in a selecting state. False otherwise.
bool Selection::IsInSelectingState() const
{
    return (g_ciConsoleInformation.Flags & CONSOLE_SELECTING) != 0;
}

// Routine Description:
// - Helps set the global selecting state.
// Arguments:
// - fSelectingOn - Set true to set the global flag on. False to turn the global flag off.
// Return Value:
// - <none>
void Selection::_SetSelectingState(_In_ const bool fSelectingOn)
{
    if (fSelectingOn)
    {
        g_ciConsoleInformation.Flags |= CONSOLE_SELECTING;
    }
    else
    {
        g_ciConsoleInformation.Flags &= ~CONSOLE_SELECTING;
    }
}

// Routine Description:
// - Determines whether the console should do selections with the mouse
//   a.k.a. "Quick Edit" mode
// Arguments:
// - <none> (gets global state)
// Return Value:
// - True if quick edit mode is enabled. False otherwise.
bool Selection::IsInQuickEditMode() const
{
    return (g_ciConsoleInformation.Flags & CONSOLE_QUICK_EDIT_MODE) != 0;
}

// Routine Description:
// - Determines whether we are performing a line selection right now
// Arguments:
// - <none>
// Return Value:
// - True if the selection is to be treated line by line. False if it is to be a block.
bool Selection::IsLineSelection() const
{
    // if line selection is on and alternate is off -OR-
    // if line selection is off and alternate is on...

    return ((_fLineSelection && !_fUseAlternateSelection) ||
        (!_fLineSelection && _fUseAlternateSelection));
}

// Routine Description:
// - Assures that the alternate selection flag is flipped in line with the requested format.
//   If true, we'll align to ensure line selection is used. If false, we'll make sure box selection is used.
// Arguments:
// - fAlignToLineSelect - whether or not to use line selection
// Return Value:
// - <none>
void Selection::_AlignAlternateSelection(_In_ const bool fAlignToLineSelect)
{
    if (fAlignToLineSelect)
    {
        // states are opposite when in line selection.
        // e.g. Line = true, Alternate = false.
        // and  Line = false, Alternate = true.
        _fUseAlternateSelection = !_fLineSelection;
    }
    else
    {
        // states are the same when in box selection.
        // e.g. Line = true, Alternate = true.
        // and  Line = false, Alternate = false.
        _fUseAlternateSelection = _fLineSelection;
    }
}

// Routine Description:
// - Determines whether the selection area is empty.
// Arguments:
// - <none>
// Return Value:
// - True if the selection variables contain valid selection data. False otherwise.
bool Selection::IsAreaSelected() const
{
    return (_dwSelectionFlags & CONSOLE_SELECTION_NOT_EMPTY) != 0;
}

// Routine Description:
// - Determines whether mark mode specifically started this selction.
// Arguments:
// - <none>
// Return Value:
// - True if the selection was started as mark mode. False otherwise.
bool Selection::IsKeyboardMarkSelection() const
{
    // if we're not selecting, we shouldn't be asking what type of selection it is
    ASSERT(IsInSelectingState());

    return (_dwSelectionFlags & CONSOLE_MOUSE_SELECTION) == 0;
}

// Routine Description:
// - Determines whether a mouse event was responsible for initiating this selection.
// - This primarily refers to mouse drag in QuickEdit mode.
// - However, it refers to any non-mark-mode selection, whether the mouse actually started it or not.
// Arguments:
// - <none>
// Return Value:
// - True if the selection is mouse-initiated. False otherwise.
bool Selection::IsMouseInitiatedSelection() const
{
    // if we're not selecting, we shouldn't be asking what type of selection it is
    ASSERT(IsInSelectingState());

    return (_dwSelectionFlags & CONSOLE_MOUSE_SELECTION) != 0;
}

// Routine Description:
// - Determines whether the mouse button is currently being held down
//   to extend or otherwise manipulate the selection area.
// Arguments:
// - <none>
// Return Value:
// - True if the mouse button is currently down. False otherwise.
bool Selection::IsMouseButtonDown() const
{
    return (_dwSelectionFlags & CONSOLE_MOUSE_DOWN) != 0;
}

void Selection::MouseDown()
{
    _dwSelectionFlags |= CONSOLE_MOUSE_DOWN;

    // We must capture the mouse on button down to ensure we receive messages if it comes back up outside the window.
    SetCapture(g_ciConsoleInformation.hWnd);
}

void Selection::MouseUp()
{
    _dwSelectionFlags &= ~CONSOLE_MOUSE_DOWN;

    ReleaseCapture();
}

// Routine Description:
// - Saves the current cursor position data so it can be manipulated during selection.
// Arguments:
// - <none> (Captures global state)
// Return Value:
// - <none>
void Selection::_SaveCursorData(_In_ const TEXT_BUFFER_INFO* const pTextInfo)
{
    Cursor* const pCursor = pTextInfo->GetCursor();
    _coordSavedCursorPosition = pCursor->GetPosition();
    _ulSavedCursorSize = pCursor->GetSize();
    _fSavedCursorVisible = !!pCursor->IsVisible();
}

// Routine Description:
// - Restores the cursor position data that was captured when selection was started.
// Arguments:
// - <none> (Restores global state)
// Return Value:
// - <none>
void Selection::_RestoreCursorData(_In_ SCREEN_INFORMATION* const pScreenInfo)
{
    pScreenInfo->SetCursorInformation(_ulSavedCursorSize, _fSavedCursorVisible);
    pScreenInfo->SetCursorPosition(_coordSavedCursorPosition, TRUE /* TurnOn */);
}

// Routine Description:
// - Gets the current selection anchor position
// Arguments:
// - pcoordSelectionAnchor - The coordinate to fill with the current selection anchor values
// Return Value:
// - <none>
void Selection::GetSelectionAnchor(_Out_ COORD* const pcoordSelectionAnchor) const
{
    (*pcoordSelectionAnchor) = _coordSelectionAnchor;
}

// Routine Description:
// - Gets the current selection rectangle
// Arguments:
// - psrSelectionRect - The rectangle to fill with selection data.
// Return Value:
// - <none>
void Selection::GetSelectionRectangle(_Out_ SMALL_RECT* const psrSelectionRect) const
{
    (*psrSelectionRect) = _srSelectionRect;
}

// Routine Description:
// - Gets the publically facing set of selection flags.
//   Strips out any internal flags in use.
// Arguments:
// - pdwFlags - DWORD to fill with flags data.
// Return Value:
// - <none>
void Selection::GetPublicSelectionFlags(_Out_ DWORD* const pdwFlags) const
{
    // CONSOLE_SELECTION_VALID is the union (binary OR) of all externally valid flags in wincon.h

    *pdwFlags = _dwSelectionFlags & CONSOLE_SELECTION_VALID;
}

// Routine Description:
// - Sets the line selection status.
//   If true, we'll use line selection. If false, we'll use traditional box selection.
// Arguments:
// - fLineSelectionOn - whether or not to use line selection
// Return Value:
// - <none>
void Selection::SetLineSelection(_In_ const bool fLineSelectionOn)
{
    if (_fLineSelection != fLineSelectionOn)
    {
        // Ensure any existing selections are cleared so the draw state is updated appropriately.
        ClearSelection();

        _fLineSelection = fLineSelectionOn;
    }
}
