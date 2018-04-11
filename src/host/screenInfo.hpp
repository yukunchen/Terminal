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
#include "../terminal/adapter/adaptDispatch.hpp"
#include "../terminal/parser/stateMachine.hpp"
#include "../terminal/parser/OutputStateMachineEngine.hpp"
#include "../server/ObjectHeader.h"

#include "../interactivity/inc/IAccessibilityNotifier.hpp"
#include "../interactivity/inc/IConsoleWindow.hpp"
#include "../interactivity/inc/IWindowMetrics.hpp"

#include "../inc/ITerminalOutputConnection.hpp"

#include "../types/inc/Viewport.hpp"

using namespace Microsoft::Console::Interactivity;
using namespace Microsoft::Console::VirtualTerminal;

class ConversionAreaInfo; // forward decl window. circular reference

class SCREEN_INFORMATION
{
public:
    [[nodiscard]]
    static NTSTATUS CreateInstance(_In_ COORD coordWindowSize,
                                   const FontInfo* const pfiFont,
                                   _In_ COORD coordScreenBufferSize,
                                   const CHAR_INFO ciFill,
                                   const CHAR_INFO ciPopupFill,
                                   const UINT uiCursorSize,
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

    COORD GetMinWindowSizeInCharacters(const COORD coordFontSize = { 1, 1 }) const;
    COORD GetMaxWindowSizeInCharacters(const COORD coordFontSize = { 1, 1 }) const;
    COORD GetLargestWindowSizeInCharacters(const COORD coordFontSize = { 1, 1 }) const;
    COORD GetScrollBarSizesInCharacters() const;

    void ProcessResizeWindow(const RECT* const prcClientNew, const RECT* const prcClientOld);
    void SetViewportSize(const COORD* const pcoordSize);

    COORD GetScreenBufferSize() const;
    void SetScreenBufferSize(const COORD coordNewBufferSize);

    COORD GetScreenFontSize() const;
    void UpdateFont(const FontInfo* const pfiNewFont);
    void RefreshFontWithRenderer();

    [[nodiscard]]
    NTSTATUS ResizeScreenBuffer(const COORD coordNewScreenSize, const bool fDoScrollBarUpdate);

    void ResetTextFlags(const short sStartX, const short sStartY, const short sEndX, const short sEndY);

    void UpdateScrollBars();
    void InternalUpdateScrollBars();

    bool IsMaximizedBoth() const;
    bool IsMaximizedX() const;
    bool IsMaximizedY() const;

    SMALL_RECT GetBufferViewport() const;
    void SetBufferViewport(const Microsoft::Console::Types::Viewport newViewport);
    // Forwarders to Window if we're the active buffer.
    [[nodiscard]]
    NTSTATUS SetViewportOrigin(const bool fAbsolute, const COORD coordWindowOrigin);
    void SetViewportRect(const Microsoft::Console::Types::Viewport newViewport);
    bool SendNotifyBeep() const;
    bool PostUpdateWindowSize() const;

    bool InVTMode() const;

    // TODO: MSFT 9358743 - http://osgvsowi/9358743
    // TODO: WARNING. This currently relies on the ConsoleObjectHeader being the FIRST portion of the console object
    //       structure or class. If it is not the first item, the cast back to the object won't work anymore.
    ConsoleObjectHeader Header;

    // TODO: MSFT 9355062 these methods should probably be a part of construction/destruction. http://osgvsowi/9355062
    static void s_InsertScreenBuffer(_In_ SCREEN_INFORMATION* const pScreenInfo);
    static void s_RemoveScreenBuffer(_In_ SCREEN_INFORMATION* const pScreenInfo);


    std::wstring ReadText(const size_t rowIndex) const;
    std::vector<OutputCell> ReadLine(const size_t rowIndex) const;
    std::vector<OutputCell> ReadLine(const size_t rowIndex,
                                     const size_t startIndex) const;
    std::vector<OutputCell> ReadLine(const size_t rowIndex,
                                     const size_t startIndex,
                                     const size_t count) const;

    std::pair<COORD, COORD> GetWordBoundary(const COORD position) const;




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

    bool IsActiveScreenBuffer() const;

    SHORT GetScreenWindowSizeX() const;
    SHORT GetScreenWindowSizeY() const;

    WriteBuffer* GetBufferWriter() const;
    AdaptDispatch* GetAdapterDispatch() const;
    StateMachine* GetStateMachine() const;


    void SetCursorInformation(const ULONG Size,
                              const bool Visible,
                              _In_ unsigned int const Color,
                              const CursorType Type);

    void SetCursorDBMode(const bool DoubleCursor);
    [[nodiscard]]
    NTSTATUS SetCursorPosition(const COORD Position, const bool TurnOn);

    void MakeCursorVisible(const COORD CursorPosition);

    SMALL_RECT GetScrollMargins() const;
    void SetScrollMargins(const SMALL_RECT* const psrMargins);

    [[nodiscard]]
    NTSTATUS UseAlternateScreenBuffer();
    void UseMainScreenBuffer();
    SCREEN_INFORMATION* const GetActiveBuffer();
    SCREEN_INFORMATION* const GetMainBuffer();

    typedef struct _TabStop
    {
        SHORT sColumn;
        struct _TabStop* ptsNext = nullptr;
    } TabStop;

    [[nodiscard]]
    NTSTATUS AddTabStop(const SHORT sColumn);
    void ClearTabStops();
    void ClearTabStop(const SHORT sColumn);
    COORD GetForwardTab(const COORD cCurrCursorPos);
    COORD GetReverseTab(const COORD cCurrCursorPos);
    bool AreTabsSet();

    TextAttribute GetAttributes() const;
    const TextAttribute* const GetPopupAttributes() const;

    void SetAttributes(const TextAttribute& attributes);
    void SetPopupAttributes(const TextAttribute& popupAttributes);
    void SetDefaultAttributes(const TextAttribute& attributes,
                              const TextAttribute& popupAttributes);
    void ReplaceDefaultAttributes(const TextAttribute& oldAttributes,
                                  const TextAttribute& oldPopupAttributes,
                                  const TextAttribute& newAttributes,
                                  const TextAttribute& newPopupAttributes);

    [[nodiscard]]
    HRESULT VtEraseAll();

    void SetTerminalConnection(_In_ Microsoft::Console::ITerminalOutputConnection* const pTtyConnection);

private:
    SCREEN_INFORMATION(_In_ IWindowMetrics *pMetrics,
                       _In_ IAccessibilityNotifier *pNotifier,
                       const CHAR_INFO ciFill,
                       const CHAR_INFO ciPopupFill);

    IWindowMetrics *_pConsoleWindowMetrics;
    IAccessibilityNotifier *_pAccessibilityNotifier;

    [[nodiscard]]
    HRESULT _AdjustScreenBufferHelper(const RECT* const prcClientNew,
                                      const COORD coordBufferOld,
                                      _Out_ COORD* const pcoordClientNewCharacters);
    [[nodiscard]]
    HRESULT _AdjustScreenBuffer(const RECT* const prcClientNew);
    void _CalculateViewportSize(const RECT* const prcClientArea, _Out_ COORD* const pcoordSize);
    void _AdjustViewportSize(const RECT* const prcClientNew, const RECT* const prcClientOld, const COORD* const pcoordSize);
    void _InternalSetViewportSize(const COORD* const pcoordSize, const bool fResizeFromTop, const bool fResizeFromLeft);

    static void s_CalculateScrollbarVisibility(const RECT* const prcClientArea,
                                               const COORD* const pcoordBufferSize,
                                               const COORD* const pcoordFontSize,
                                               _Out_ bool* const pfIsHorizontalVisible,
                                               _Out_ bool* const pfIsVerticalVisible);

    [[nodiscard]]
    NTSTATUS ResizeWithReflow(const COORD coordnewScreenSize);
    [[nodiscard]]
    NTSTATUS ResizeTraditional(const COORD coordNewScreenSize);

    [[nodiscard]]
    NTSTATUS _InitializeOutputStateMachine();
    void _FreeOutputStateMachine();

    [[nodiscard]]
    NTSTATUS _CreateAltBuffer(_Out_ SCREEN_INFORMATION** const ppsiNewScreenBuffer);

    bool _IsAltBuffer() const;
    bool _IsInPtyMode() const;

    void _InitializeBufferDimensions(const COORD coordScreenBufferSize,
                                     const COORD coordViewportSize);

    ConhostInternalGetSet* _pConApi;
    WriteBuffer* _pBufferWriter;
    AdaptDispatch* _pAdapter;
    StateMachine* _pStateMachine;
    std::shared_ptr<OutputStateMachineEngine> _pEngine;

    COORD _coordScreenBufferSize; // dimensions of buffer

    SMALL_RECT _srScrollMargins; //The margins of the VT specified scroll region. Left and Right are currently unused, but could be in the future.

    // Specifies which coordinates of the screen buffer are visible in the
    //      window client (the "viewport" into the buffer)
    Microsoft::Console::Types::Viewport _viewport;

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
