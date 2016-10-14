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

#include "outputStream.hpp"
#include "..\terminal\adapter\adaptDispatch.hpp"
#include "..\terminal\parser\stateMachine.hpp"
using namespace Microsoft::Console::VirtualTerminal;

class Window; // forward decl window. circular reference
class ConversionAreaInfo;

class SCREEN_INFORMATION
{
public:
    static NTSTATUS CreateInstance(_In_ COORD coordWindowSize,
                                   _In_ const FontInfo* const pfiFont,
                                   _In_ COORD coordScreenBufferSize,
                                   _In_ CHAR_INFO const ciFill,
                                   _In_ CHAR_INFO const ciPopupFill,
                                   _In_ UINT const uiCursorSize,
                                   _Outptr_ SCREEN_INFORMATION ** const ppScreen);

    ~SCREEN_INFORMATION();

    NTSTATUS GetScreenBufferInformation(_Out_ PCOORD pcoordSize,
                                        _Out_ PCOORD pcoordCursorPosition,
                                        _Out_ PCOORD pcoordScrollPosition,
                                        _Out_ PWORD pwAttributes,
                                        _Out_ PCOORD pcoordCurrentWindowSize,
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
    COORD GetScrollBarSizesInCharacters(_In_ COORD const coordFontSize = { 1, 1 }) const;

    void ProcessResizeWindow(_In_ const RECT* const prcClientNew, _In_ const RECT* const prcClientOld);
    void SetViewportSize(_In_ const COORD* const pcoordSize);

    COORD GetScreenFontSize() const;
    void UpdateFont(_In_ const FontInfo* const pfiNewFont);
    void RefreshFontWithRenderer();

    NTSTATUS ResizeScreenBuffer(_In_ const COORD coordNewScreenSize, _In_ const bool fDoScrollBarUpdate);

    void ResetTextFlags(_In_ short const sStartX, _In_ short const sStartY, _In_ short const sEndX, _In_ short const sEndY);

    void UpdateScrollBars();
    void InternalUpdateScrollBars();

    bool IsMaximizedBoth() const;
    bool IsMaximizedX() const;
    bool IsMaximizedY() const;

    // Forwarders to Window if we're the active buffer.
    NTSTATUS SetViewportOrigin(_In_ const BOOL fAbsolute, _In_ const COORD coordWindowOrigin);
    NTSTATUS SetViewportRect(_In_ SMALL_RECT* const prcNewViewport);
    BOOL SendNotifyBeep() const;
    BOOL PostUpdateWindowSize() const;

    ConsoleObjectHeader Header;

    // TODO: MSFT 9355062 these methods should probably be a part of construction/destruction. http://osgvsowi/9355062
    static void s_InsertScreenBuffer(_In_ SCREEN_INFORMATION* const pScreenInfo);
    static void s_RemoveScreenBuffer(_In_ SCREEN_INFORMATION* const pScreenInfo);

    DWORD OutputMode;
    COORD ScreenBufferSize; // dimensions of buffer
    SMALL_RECT BufferViewport;  // specifies which coordinates of the screen buffer are visible in the window client (the "viewport" into the buffer)
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
    ConversionAreaInfo* ConvScreenInfo;
    UINT ScrollScale;

    BOOL IsActiveScreenBuffer() const;

    SHORT GetScreenWindowSizeX() const;
    SHORT GetScreenWindowSizeY() const;

    WriteBuffer* GetBufferWriter() const;
    AdaptDispatch* GetAdapterDispatch() const;
    StateMachine* GetStateMachine() const;

    NTSTATUS SetCursorInformation(_In_ ULONG const Size, _In_ BOOLEAN const Visible);
    NTSTATUS SetCursorDBMode(_In_ const BOOLEAN DoubleCursor);
    NTSTATUS SetCursorPosition(_In_ COORD const Position, _In_ BOOL const TurnOn);

    void MakeCursorVisible(_In_ const COORD CursorPosition);

    SMALL_RECT GetScrollMargins() const;
    void SetScrollMargins(_In_ const SMALL_RECT* const psrMargins);

    NTSTATUS UseAlternateScreenBuffer();
    NTSTATUS UseMainScreenBuffer();
    SCREEN_INFORMATION* const GetActiveBuffer();
    SCREEN_INFORMATION* const GetMainBuffer();

    typedef struct _TabStop
    {
        SHORT sColumn;
        struct _TabStop* ptsNext = nullptr;
    } TabStop;

    NTSTATUS AddTabStop(_In_ const SHORT sColumn);
    void ClearTabStops();
    void ClearTabStop(_In_ const SHORT sColumn);
    COORD GetForwardTab(_In_ const COORD cCurrCursorPos);
    COORD GetReverseTab(_In_ const COORD cCurrCursorPos);
    bool AreTabsSet();

    const TextAttribute* const GetAttributes() const;
    const TextAttribute* const GetPopupAttributes() const;

    void SetAttributes(_In_ const TextAttribute* const pAttributes);
    void SetPopupAttributes(_In_ const TextAttribute* const pPopupAttributes);

private:
    SCREEN_INFORMATION(_In_ const CHAR_INFO ciFill, _In_ const CHAR_INFO ciPopupFill);

    _Must_inspect_result_
    Window* _GetWindow() const;

    void _AdjustScreenBufferHelper(_In_ const RECT* const prcClientNew,
                                   _In_ COORD const coordBufferOld,
                                   _Out_ COORD* const pcoordClientNewCharacters);
    void _AdjustScreenBuffer(_In_ const RECT* const prcClientNew);
    void _CalculateViewportSize(_In_ const RECT* const prcClientArea, _Out_ COORD* const pcoordSize);
    void _AdjustViewportSize(_In_ const RECT* const prcClientNew, _In_ const RECT* const prcClientOld, _In_ const COORD* const pcoordSize);
    void _InternalSetViewportSize(_In_ const COORD* const pcoordSize, _In_ bool const fResizeFromTop, _In_ bool const fResizeFromLeft);

    static void s_CalculateScrollbarVisibility(_In_ const RECT* const prcClientArea,
                                               _In_ const COORD* const pcoordBufferSize,
                                               _In_ const COORD* const pcoordFontSize,
                                               _Out_ bool* const pfIsHorizontalVisible,
                                               _Out_ bool* const pfIsVerticalVisible);

    NTSTATUS ResizeWithReflow(_In_ COORD const coordnewScreenSize);
    NTSTATUS ResizeTraditional(_In_ COORD const coordNewScreenSize);

    NTSTATUS _InitializeOutputStateMachine();
    void _FreeOutputStateMachine();

    NTSTATUS _CreateAltBuffer(_Out_ SCREEN_INFORMATION** const ppsiNewScreenBuffer);

    bool _IsAltBuffer() const;

    ConhostInternalGetSet* _pConApi;
    WriteBuffer* _pBufferWriter;
    AdaptDispatch* _pAdapter;
    StateMachine* _pStateMachine;

    SMALL_RECT _srScrollMargins; //The margins of the VT specified scroll region. Left and Right are currently unused, but could be in the future. 

    SCREEN_INFORMATION* _psiAlternateBuffer = nullptr; // The VT "Alternate" screen buffer.
    SCREEN_INFORMATION* _psiMainBuffer = nullptr; // A pointer to the main buffer, if this is the alternate buffer.

    RECT _rcAltSavedClientNew = {0};
    RECT _rcAltSavedClientOld = {0};
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
