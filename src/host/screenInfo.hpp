/*++
Copyright (c) Microsoft Corporation

Module Name:
- screenInfo.hpp

Abstract:
- This module represents the structures and functions required
  for rendering one screen of the console host window.

Author(s):
- Michael Niksa (MiNiksa) 10-Apr-2014
- Paul Campbell (PaulCam) 10-Apr-2014

Revision History:
- From components of output.h/.c and resize.c by Therese Stowell (ThereseS) 1990-1991
--*/

#pragma once

#include "conapi.h"
#include "textBuffer.hpp"
#include "settings.hpp"
#include "TextAttribute.hpp"

#include "outputStream.hpp"
#include "..\terminal\adapter\adaptDispatch.hpp"
#include "..\terminal\parser\stateMachine.hpp"
#include "..\server\ObjectHeader.h"

#include "..\interactivity\inc\IAccessibilityNotifier.hpp"
#include "..\interactivity\inc\IConsoleWindow.hpp"
#include "..\interactivity\inc\IWindowMetrics.hpp"

using namespace Microsoft::Console::Interactivity;
using namespace Microsoft::Console::VirtualTerminal;

class ConversionAreaInfo; // forward decl window. circular reference

class SCREEN_INFORMATION
{
public:
    [[nodiscard]]
    static NTSTATUS CreateInstance(_In_ COORD coordWindowSize,
                                   _In_ const FontInfo* const pfiFont,
                                   _In_ COORD coordScreenBufferSize,
                                   _In_ CHAR_INFO const ciFill,
                                   _In_ CHAR_INFO const ciPopupFill,
                                   _In_ UINT const uiCursorSize,
                                   _Outptr_ SCREEN_INFORMATION ** const ppScreen);

    ~SCREEN_INFORMATION();

    void GetScreenBufferInformation(_Out_ PCOORD pcoordSize,
                                    _Out_ PCOORD pcoordCursorPosition,
                                    _Out_ PSMALL_RECT psrWindow,
                                    _Out_ PWORD pwAttributes,
                                    _Out_ PCOORD pcoordMaximumWindowSize,
                                    _Out_ PWORD pwPopupAttributes,
                                    _Out_writes_(COLOR_TABLE_SIZE) LPCOLORREF lpColorTable) const;

    void GetRequiredConsoleSizeInPixels(_Out_ PSIZE const pRequiredSize) const;

    void MakeCurrentCursorVisible();

    void ClipToScreenBuffer(_Inout_ SMALL_RECT* const psrClip) const;
    void ClipToScreenBuffer(_Inout_ COORD* const pcoordClip) const;

    void GetScreenEdges(_Out_ SMALL_RECT* const psrEdges) const;

    COORD GetMinWindowSizeInCharacters(_In_ COORD const coordFontSize = { 1, 1 }) const;
    COORD GetMaxWindowSizeInCharacters(_In_ COORD const coordFontSize = { 1, 1 }) const;
    COORD GetLargestWindowSizeInCharacters(_In_ COORD const coordFontSize = { 1, 1 }) const;
    COORD GetScrollBarSizesInCharacters() const;

    void ProcessResizeWindow(_In_ const RECT* const prcClientNew, _In_ const RECT* const prcClientOld);
    void SetViewportSize(_In_ const COORD* const pcoordSize);

    COORD GetScreenBufferSize() const;
    void SetScreenBufferSize(_In_ const COORD coordNewBufferSize);

    COORD GetScreenFontSize() const;
    void UpdateFont(_In_ const FontInfo* const pfiNewFont);
    void RefreshFontWithRenderer();

    [[nodiscard]]
    NTSTATUS ResizeScreenBuffer(_In_ const COORD coordNewScreenSize, _In_ const bool fDoScrollBarUpdate);

    void ResetTextFlags(_In_ short const sStartX, _In_ short const sStartY, _In_ short const sEndX, _In_ short const sEndY);

    void UpdateScrollBars();
    void InternalUpdateScrollBars();

    bool IsMaximizedBoth() const;
    bool IsMaximizedX() const;
    bool IsMaximizedY() const;

    SMALL_RECT GetBufferViewport() const;
    void SetBufferViewport(SMALL_RECT srBufferViewport);
    // Forwarders to Window if we're the active buffer.
    [[nodiscard]]
    NTSTATUS SetViewportOrigin(_In_ const BOOL fAbsolute, _In_ const COORD coordWindowOrigin);
    void SetViewportRect(_In_ SMALL_RECT* const prcNewViewport);
    BOOL SendNotifyBeep() const;
    BOOL PostUpdateWindowSize() const;

    bool InVTMode() const;

    // TODO: MSFT 9358743 - http://osgvsowi/9358743
    // TODO: WARNING. This currently relies on the ConsoleObjectHeader being the FIRST portion of the console object
    //       structure or class. If it is not the first item, the cast back to the object won't work anymore.
    ConsoleObjectHeader Header;

    // TODO: MSFT 9355062 these methods should probably be a part of construction/destruction. http://osgvsowi/9355062
    static void s_InsertScreenBuffer(_In_ SCREEN_INFORMATION* const pScreenInfo);
    static void s_RemoveScreenBuffer(_In_ SCREEN_INFORMATION* const pScreenInfo);

    DWORD OutputMode;
    WORD ResizingWindow;    // > 0 if we should ignore WM_SIZE messages

    short WheelDelta;
    short HWheelDelta;
    TEXT_BUFFER_INFO *TextInfo;
    SCREEN_INFORMATION *Next;
    BYTE WriteConsoleDbcsLeadByte[2];
    BYTE FillOutDbcsLeadChar;
    WCHAR LineChar[6];
#define UPPER_LEFT_CORNER   0
#define UPPER_RIGHT_CORNER  1
#define HORIZONTAL_LINE     2
#define VERTICAL_LINE       3
#define BOTTOM_LEFT_CORNER  4
#define BOTTOM_RIGHT_CORNER 5

    // non ownership pointer
    ConversionAreaInfo* ConvScreenInfo;

    UINT ScrollScale;

    BOOL IsActiveScreenBuffer() const;

    SHORT GetScreenWindowSizeX() const;
    SHORT GetScreenWindowSizeY() const;

    WriteBuffer* GetBufferWriter() const;
    AdaptDispatch* GetAdapterDispatch() const;
    StateMachine* GetStateMachine() const;

    void SetCursorInformation(_In_ ULONG const Size,
                              _In_ BOOLEAN const Visible,
                              _In_ unsigned int const Color,
                              _In_ CursorType const Type);

    void SetCursorDBMode(_In_ const BOOLEAN DoubleCursor);
    [[nodiscard]]
    NTSTATUS SetCursorPosition(_In_ COORD const Position, _In_ BOOL const TurnOn);

    void MakeCursorVisible(_In_ const COORD CursorPosition);

    SMALL_RECT GetScrollMargins() const;
    void SetScrollMargins(_In_ const SMALL_RECT* const psrMargins);

    [[nodiscard]]
    NTSTATUS UseAlternateScreenBuffer();
    [[nodiscard]]
    NTSTATUS UseMainScreenBuffer();
    SCREEN_INFORMATION* const GetActiveBuffer();
    SCREEN_INFORMATION* const GetMainBuffer();

    typedef struct _TabStop
    {
        SHORT sColumn;
        struct _TabStop* ptsNext = nullptr;
    } TabStop;

    [[nodiscard]]
    NTSTATUS AddTabStop(_In_ const SHORT sColumn);
    void ClearTabStops();
    void ClearTabStop(_In_ const SHORT sColumn);
    COORD GetForwardTab(_In_ const COORD cCurrCursorPos);
    COORD GetReverseTab(_In_ const COORD cCurrCursorPos);
    bool AreTabsSet();

    TextAttribute GetAttributes() const;
    const TextAttribute* const GetPopupAttributes() const;

    void SetAttributes(_In_ const TextAttribute& attributes);
    void SetPopupAttributes(_In_ const TextAttribute& popupAttributes);
    void SetDefaultAttributes(_In_ const TextAttribute& attributes,
                              _In_ const TextAttribute& popupAttributes);
    void ReplaceDefaultAttributes(_In_ const TextAttribute& oldAttributes,
                                  _In_ const TextAttribute& oldPopupAttributes,
                                  _In_ const TextAttribute& newAttributes,
                                  _In_ const TextAttribute& newPopupAttributes);

    [[nodiscard]]
    HRESULT VtEraseAll();

private:
    SCREEN_INFORMATION(_In_ IWindowMetrics *pMetrics,
                       _In_ IAccessibilityNotifier *pNotifier,
                       _In_ const CHAR_INFO ciFill,
                       _In_ const CHAR_INFO ciPopupFill);

    IWindowMetrics *_pConsoleWindowMetrics;
    IAccessibilityNotifier *_pAccessibilityNotifier;

    [[nodiscard]]
    HRESULT _AdjustScreenBufferHelper(_In_ const RECT* const prcClientNew,
                                      _In_ COORD const coordBufferOld,
                                      _Out_ COORD* const pcoordClientNewCharacters);
    [[nodiscard]]
    HRESULT _AdjustScreenBuffer(_In_ const RECT* const prcClientNew);
    void _CalculateViewportSize(_In_ const RECT* const prcClientArea, _Out_ COORD* const pcoordSize);
    void _AdjustViewportSize(_In_ const RECT* const prcClientNew, _In_ const RECT* const prcClientOld, _In_ const COORD* const pcoordSize);
    void _InternalSetViewportSize(_In_ const COORD* const pcoordSize, _In_ bool const fResizeFromTop, _In_ bool const fResizeFromLeft);

    static void s_CalculateScrollbarVisibility(_In_ const RECT* const prcClientArea,
                                               _In_ const COORD* const pcoordBufferSize,
                                               _In_ const COORD* const pcoordFontSize,
                                               _Out_ bool* const pfIsHorizontalVisible,
                                               _Out_ bool* const pfIsVerticalVisible);

    [[nodiscard]]
    NTSTATUS ResizeWithReflow(_In_ COORD const coordnewScreenSize);
    [[nodiscard]]
    NTSTATUS ResizeTraditional(_In_ COORD const coordNewScreenSize);

    [[nodiscard]]
    NTSTATUS _InitializeOutputStateMachine();
    void _FreeOutputStateMachine();

    [[nodiscard]]
    NTSTATUS _CreateAltBuffer(_Out_ SCREEN_INFORMATION** const ppsiNewScreenBuffer);

    bool _IsAltBuffer() const;
    bool _IsInPtyMode() const;

    void _InitializeBufferDimensions(_In_ const COORD coordScreenBufferSize,
                                     _In_ const COORD coordViewportSize);

    ConhostInternalGetSet* _pConApi;
    WriteBuffer* _pBufferWriter;
    AdaptDispatch* _pAdapter;
    StateMachine* _pStateMachine;

    COORD _coordScreenBufferSize; // dimensions of buffer

    SMALL_RECT _srScrollMargins; //The margins of the VT specified scroll region. Left and Right are currently unused, but could be in the future.

    // specifies which coordinates of the screen buffer are visible in the
    //      window client (the "viewport" into the buffer)
    // This is an Inclusive rectangle
    SMALL_RECT _srBufferViewport;

    SCREEN_INFORMATION* _psiAlternateBuffer = nullptr; // The VT "Alternate" screen buffer.
    SCREEN_INFORMATION* _psiMainBuffer = nullptr; // A pointer to the main buffer, if this is the alternate buffer.

    RECT _rcAltSavedClientNew = { 0 };
    RECT _rcAltSavedClientOld = { 0 };
    bool _fAltWindowChanged = false;

    TabStop* _ptsTabs = nullptr; // The head of the list of Tab Stops

    TextAttribute _Attributes;
    TextAttribute _PopupAttributes;

#ifdef UNIT_TESTING
    friend class ScreenBufferTests;
#endif
};

typedef SCREEN_INFORMATION *PSCREEN_INFORMATION;
typedef PSCREEN_INFORMATION *PPSCREEN_INFORMATION;
