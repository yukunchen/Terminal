/*++
Copyright (c) Microsoft Corporation

Module Name:
- settings.hpp

Abstract:
- This module is used for all configurable settings in the console

Author(s):
- Michael Niksa (MiNiksa) 23-Jul-2014
- Paul Campbell (PaulCam) 23-Jul-2014

Revision History:
- From components of consrv.h
- This is a reduced/de-duplicated version of settings that were stored in the registry, link files, and in the console information state.
--*/
#pragma once

#include "TextAttribute.hpp"

// To prevent invisible windows, set a lower threshold on window alpha channel.
#define MIN_WINDOW_OPACITY 0x4D // 0x4D is approximately 30% visible/opaque (70% transparent). Valid range is 0x00-0xff.
#define COLOR_TABLE_SIZE (16)
#define XTERM_COLOR_TABLE_SIZE (256)

#include "ConsoleArguments.hpp"
#include "TextAttribute.hpp"
#include "../inc/conattrs.hpp"

class Settings
{
public:
    Settings();

    void ApplyDesktopSpecificDefaults();

    void ApplyStartupInfo(const Settings* const pStartupSettings);
    void ApplyCommandlineArguments(const ConsoleArguments& consoleArgs);
    void InitFromStateInfo(_In_ PCONSOLE_STATE_INFO pStateInfo);
    void Validate();

    DWORD GetVirtTermLevel() const;
    void SetVirtTermLevel(const DWORD dwVirtTermLevel);

    bool IsAltF4CloseAllowed() const;
    void SetAltF4CloseAllowed(const bool fAllowAltF4Close);

    bool IsReturnOnNewlineAutomatic() const;
    void SetAutomaticReturnOnNewline(const bool fAutoReturnOnNewline);

    bool IsGridRenderingAllowedWorldwide() const;
    void SetGridRenderingAllowedWorldwide(const bool fGridRenderingAllowed);

    BOOL GetFilterOnPaste() const;
    void SetFilterOnPaste(const BOOL fFilterOnPaste);

    const WCHAR* const GetLaunchFaceName() const;
    void SetLaunchFaceName(_In_ PCWSTR const LaunchFaceName, const size_t cchLength);

    UINT GetCodePage() const;
    void SetCodePage(const UINT uCodePage);

    UINT GetScrollScale() const;
    void SetScrollScale(const UINT uScrollScale);

    BOOL GetTrimLeadingZeros() const;
    void SetTrimLeadingZeros(const BOOL fTrimLeadingZeros);

    BOOL GetEnableColorSelection() const;
    void SetEnableColorSelection(const BOOL fEnableColorSelection);

    BOOL GetLineSelection() const;
    void SetLineSelection(const BOOL bLineSelection);

    bool GetWrapText () const;
    void SetWrapText (const bool bWrapText );

    BOOL GetCtrlKeyShortcutsDisabled () const;
    void SetCtrlKeyShortcutsDisabled (const BOOL fCtrlKeyShortcutsDisabled );

    BYTE GetWindowAlpha() const;
    void SetWindowAlpha(const BYTE bWindowAlpha);

    DWORD GetHotKey() const;
    void SetHotKey(const DWORD dwHotKey);

    bool IsStartupTitleIsLinkNameSet() const;

    DWORD GetStartupFlags() const;
    void SetStartupFlags(const DWORD dwStartupFlags);
    void UnsetStartupFlag(const DWORD dwFlagToUnset);

    WORD GetFillAttribute() const;
    void SetFillAttribute(const WORD wFillAttribute);

    WORD GetPopupFillAttribute() const;
    void SetPopupFillAttribute(const WORD wPopupFillAttribute);

    WORD GetShowWindow() const;
    void SetShowWindow(const WORD wShowWindow);

    WORD GetReserved() const;
    void SetReserved(const WORD wReserved);

    COORD GetScreenBufferSize() const;
    void SetScreenBufferSize(const COORD dwScreenBufferSize);

    COORD GetWindowSize() const;
    void SetWindowSize(const COORD dwWindowSize);

    bool IsWindowSizePixelsValid() const;
    COORD GetWindowSizePixels() const;
    void SetWindowSizePixels(const COORD dwWindowSizePixels);

    COORD GetWindowOrigin() const;
    void SetWindowOrigin(const COORD dwWindowOrigin);

    DWORD GetFont() const;
    void SetFont(const DWORD dwFont);

    COORD GetFontSize() const;
    void SetFontSize(const COORD dwFontSize);

    UINT GetFontFamily() const;
    void SetFontFamily(const UINT uFontFamily);

    UINT GetFontWeight() const;
    void SetFontWeight(const UINT uFontWeight);

    const WCHAR* const GetFaceName() const;
    BOOL IsFaceNameSet() const;
    void SetFaceName(_In_ PCWSTR const pcszFaceName, const size_t cchLength);

    UINT GetCursorSize() const;
    void SetCursorSize(const UINT uCursorSize);

    BOOL GetFullScreen() const;
    void SetFullScreen(const BOOL fFullScreen);

    BOOL GetQuickEdit() const;
    void SetQuickEdit(const BOOL fQuickEdit);

    BOOL GetInsertMode() const;
    void SetInsertMode(const BOOL fInsertMode);

    BOOL GetAutoPosition() const;
    void SetAutoPosition(const BOOL fAutoPosition);

    UINT GetHistoryBufferSize() const;
    void SetHistoryBufferSize(const UINT uHistoryBufferSize);

    UINT GetNumberOfHistoryBuffers() const;
    void SetNumberOfHistoryBuffers(const UINT uNumberOfHistoryBuffers);

    BOOL GetHistoryNoDup() const;
    void SetHistoryNoDup(const BOOL fHistoryNoDup);

    const COLORREF* const GetColorTable() const;
    const size_t GetColorTableSize() const;
    void SetColorTable(_In_reads_(cSize) const COLORREF* const pColorTable, const size_t cSize);
    void SetColorTableEntry(const size_t index, const COLORREF ColorValue);
    COLORREF GetColorTableEntry(const size_t index) const;

    COLORREF GetCursorColor() const noexcept;
    CursorType GetCursorType() const noexcept;

    void SetCursorColor(const COLORREF CursorColor) noexcept;
    void SetCursorType(const CursorType cursorType) noexcept;

    bool GetInterceptCopyPaste() const noexcept;
    void SetInterceptCopyPaste(const bool interceptCopyPaste) noexcept;

private:
    DWORD _dwHotKey;
    DWORD _dwStartupFlags;
    WORD _wFillAttribute;
    WORD _wPopupFillAttribute;
    WORD _wShowWindow; // used when window is created
    WORD _wReserved;
    // START - This section filled via memcpy from shortcut properties. Do not rearrange/change.
    COORD _dwScreenBufferSize;
    COORD _dwWindowSize; // this is in characters.
    COORD _dwWindowOrigin; // used when window is created
    DWORD _nFont;
    COORD _dwFontSize;
    UINT _uFontFamily;
    UINT _uFontWeight;
    WCHAR _FaceName[LF_FACESIZE];
    UINT _uCursorSize;
    BOOL _bFullScreen; // deprecated
    BOOL _bQuickEdit;
    BOOL _bInsertMode; // used by command line editing
    BOOL _bAutoPosition;
    UINT _uHistoryBufferSize;
    UINT _uNumberOfHistoryBuffers;
    BOOL _bHistoryNoDup;
    COLORREF _ColorTable[COLOR_TABLE_SIZE];
    // END - memcpy
    UINT _uCodePage;
    UINT _uScrollScale;
    BOOL _fTrimLeadingZeros;
    BOOL _fEnableColorSelection;
    BOOL _bLineSelection;
    bool _bWrapText; // whether to use text wrapping when resizing the window
    BOOL _fCtrlKeyShortcutsDisabled; // disables Ctrl+<something> key intercepts
    BYTE _bWindowAlpha; // describes the opacity of the window

    BOOL _fFilterOnPaste; // should we filter text when the user pastes? (e.g. remove <tab>)
    WCHAR _LaunchFaceName[LF_FACESIZE];
    bool _fAllowAltF4Close;
    DWORD _dwVirtTermLevel;
    bool _fAutoReturnOnNewline;
    bool _fRenderGridWorldwide;

    void _InitColorTable();

    COLORREF _XtermColorTable[XTERM_COLOR_TABLE_SIZE];
    void _InitXtermTableValue(const size_t iIndex, const BYTE bRed, const BYTE bGreen, const BYTE bBlue);

    // this is used for the special STARTF_USESIZE mode.
    bool _fUseWindowSizePixels;
    COORD _dwWindowSizePixels;

    // Technically a COLORREF, but using INVALID_COLOR as "Invert Colors"
    unsigned int _CursorColor;
    CursorType _CursorType;

    bool _fInterceptCopyPaste;

    friend class RegistrySerialization;

public:

    WORD GenerateLegacyAttributes(const TextAttribute attributes) const;
    WORD FindNearestTableIndex(const COLORREF Color) const;

};
