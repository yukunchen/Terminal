/*++
Copyright (c) Microsoft Corporation

Module Name:
- selection.hpp

Abstract:
- This module is used for managing the selection region

Author(s):
- Michael Niksa (MiNiksa) 4-Jun-2014
- Paul Campbell (PaulCam) 4-Jun-2014

Revision History:
- From components of clipbrd.h/.c and input.h/.c of v1 console.
--*/

#pragma once

#include "input.h"

#include "..\interactivity\inc\IAccessibilityNotifier.hpp"
#include "..\interactivity\inc\IConsoleWindow.hpp"

using namespace Microsoft::Console::Interactivity;

class Selection
{
public:
    ~Selection();

    _Check_return_
    NTSTATUS GetSelectionRects(_Outptr_result_buffer_all_(*pcRectangles) SMALL_RECT** const prgsrSelection,
                               _Out_ UINT* const pcRectangles) const;

    void ShowSelection();
    void HideSelection();

    static Selection& Instance();

    // Key selection generally refers to "mark mode" selection where the cursor is present and used to navigate 100% with the keyboard
    // Mouse selection means either the block or line mode selection usually initiated by the mouse.
    // However, Mouse mode can also mean initiated with our shift+directional commands as no block cursor is required for navigation

    void InitializeMarkSelection();
    void InitializeMouseSelection(_In_ const COORD coordBufferPos);

    void SelectNewRegion(_In_ COORD const coordStart, _In_ COORD const coordEnd);
    void SelectAll();

    void ExtendSelection(_In_ COORD coordBufferPos);
    void AdjustSelection(_In_ COORD const coordSelectionStart, _In_ COORD const coordSelectionEnd);

    void ClearSelection();
    void ClearSelection(_In_ bool const fStartingNewSelection);
    void ColorSelection(_In_ SMALL_RECT* const psrRect, _In_ ULONG const ulAttr);

    // delete these or we can accidentally get copies of the singleton
    Selection(Selection const&) = delete;
    void operator=(Selection const&) = delete;

protected:
    Selection();

private:
    void _SetSelectionVisibility(_In_ bool const fMakeVisible);

    void _PaintSelection() const;

    static void s_BisectSelection(_In_ short const sStringLength,
                                  _In_ COORD const coordTargetPoint,
                                  _In_ const SCREEN_INFORMATION* const pScreenInfo,
                                  _Inout_ SMALL_RECT* const pSmallRect);

    void _CancelMarkSelection();
    void _CancelMouseSelection();

// -------------------------------------------------------------------------------------------------------
// input handling (selectionInput.cpp)
public:

    // key handling

    // N.B.: This enumeration helps push up calling clipboard functions into
    //       the caller. This way, all of the selection code is independent of
    //       the clipboard and thus more easily shareable with Windows editions
    //       that do not have a clipboard (i.e. OneCore).
    enum class KeySelectionEventResult
    {
        EventHandled,
        EventNotHandled,
        CopyToClipboard
    };

    KeySelectionEventResult HandleKeySelectionEvent(_In_ const INPUT_KEY_INFO* const pInputKeyInfo);
    static bool s_IsValidKeyboardLineSelection(_In_ const INPUT_KEY_INFO* const pInputKeyInfo);
    bool HandleKeyboardLineSelectionEvent(_In_ const INPUT_KEY_INFO* const pInputKeyInfo);

    void CheckAndSetAlternateSelection();

    // calculation functions
    _Check_return_ _Success_(return)
    static bool s_GetInputLineBoundaries(_Out_opt_ COORD* const pcoordInputStart, _Out_opt_ COORD* const pcoordInputEnd);
    void GetValidAreaBoundaries(_Out_opt_ COORD* const pcoordValidStart, _Out_opt_ COORD* const pcoordValidEnd) const;
    static bool s_IsWithinBoundaries(_In_ const COORD coordPosition, _In_ const COORD coordStart, _In_ const COORD coordEnd);

private:
    // key handling
    bool _HandleColorSelection(_In_ const INPUT_KEY_INFO* const pInputKeyInfo);
    bool _HandleMarkModeSelectionNav(_In_ const INPUT_KEY_INFO* const pInputKeyInfo);
    void WordByWordSelection(_In_ const bool fPrevious, _In_ const SMALL_RECT srectEdges, _In_ const COORD coordAnchor, _Inout_ COORD *pcoordSelPoint) const;


// -------------------------------------------------------------------------------------------------------
// selection state (selectionState.cpp)
public:
    bool IsKeyboardMarkSelection() const;
    bool IsMouseInitiatedSelection() const;

    bool IsLineSelection() const;

    bool IsInSelectingState() const;
    bool IsInQuickEditMode() const;

    bool IsAreaSelected() const;
    bool IsMouseButtonDown() const;

    void GetPublicSelectionFlags(_Out_ DWORD* const pdwFlags) const;
    void GetSelectionAnchor(_Out_ COORD* const pcoordSelectionAnchor) const;
    void GetSelectionRectangle(_Out_ SMALL_RECT* const psrSelectionRect) const;

    void SetLineSelection(_In_ const bool fLineSelectionOn);

    // TODO: these states likely belong somewhere else
    void MouseDown();
    void MouseUp();

private:
    void _SaveCursorData(_In_ const TEXT_BUFFER_INFO* const pTextInfo);
    void _RestoreCursorData(_In_ SCREEN_INFORMATION* const pScreenInfo);

    void _AlignAlternateSelection(_In_ const bool fAlignToLineSelect);

    void _SetSelectingState(_In_ bool const fSelectingOn);

    // TODO: extended edit key should probably be in here (remaining code is in cmdline.cpp)
    // TODO: trim leading zeros should probably be in here (pending move of reactive code from input.cpp to selectionInput.cpp)
    // TODO: enable color selection should be in here
    // TODO: quick edit mode should be in here
    // TODO: console selection mode should be in here
    // TODO: consider putting word delims in here

    // -- State/Flags --
    // This replaces/deprecates CONSOLE_SELECTION_INVERTED on ServiceLocator::LocateGlobals()->getConsoleInformation()->SelectionFlags
    bool _fSelectionVisible;

    bool _fLineSelection; // whether to use line selection or block selection
    bool _fUseAlternateSelection; // whether the user has triggered the alternate selection method

    // Flags for this DWORD are defined in wincon.h. Search for def:CONSOLE_SELECTION_IN_PROGRESS, etc.
    DWORD _dwSelectionFlags;

    // -- Current Selection Data --
    // Anchor is the point the selection was started from (and will be one of the corners of the rectangle).
    COORD _coordSelectionAnchor;
    // Rectangle is the area inscribing the selection. It is extended to screen edges in a particular way for line selection.
    SMALL_RECT _srSelectionRect;

    // -- Saved Cursor Data --
    // Saved when a selection is started for restoration later. Position is in character coordinates, not pixels.
    COORD _coordSavedCursorPosition;
    ULONG _ulSavedCursorSize;
    bool _fSavedCursorVisible;

#ifdef UNIT_TESTING
    friend class SelectionTests;
    friend class SelectionInputTests;
    friend class ClipboardTests;
#endif
};
