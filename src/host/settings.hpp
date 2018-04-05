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

    void ApplyStartupInfo(_In_ const Settings* const pStartupSettings);
    void ApplyCommandlineArguments(_In_ const ConsoleArguments& consoleArgs);
    void InitFromStateInfo(_In_ PCONSOLE_STATE_INFO pStateInfo);
    void Validate();

    DWORD GetVirtTermLevel() const;
    void SetVirtTermLevel(_In_ DWORD const dwVirtTermLevel);

    bool IsAltF4CloseAllowed() const;
    void SetAltF4CloseAllowed(_In_ bool const fAllowAltF4Close);

    bool IsReturnOnNewlineAutomatic() const;
    void SetAutomaticReturnOnNewline(_In_ bool const fAutoReturnOnNewline);

    bool IsGridRenderingAllowedWorldwide() const;
    void SetGridRenderingAllowedWorldwide(_In_ bool const fGridRenderingAllowed);

    bool GetFilterOnPaste() const;
    void SetFilterOnPaste(_In_ bool const fFilterOnPaste);

    const WCHAR* const GetLaunchFaceName() const;
    void SetLaunchFaceName(_In_ PCWSTR const LaunchFaceName, _In_ size_t const cchLength);

    UINT GetCodePage() const;
    void SetCodePage(_In_ const UINT uCodePage);

    UINT GetScrollScale() const;
    void SetScrollScale(_In_ const UINT uScrollScale);

    bool GetTrimLeadingZeros() const;
    void SetTrimLeadingZeros(_In_ const bool fTrimLeadingZeros);

    bool GetEnableColorSelection() const;
    void SetEnableColorSelection(_In_ const bool fEnableColorSelection);

    bool GetLineSelection() const;
    void SetLineSelection(_In_ const bool bLineSelection);

    bool GetWrapText () const;
    void SetWrapText (_In_ const bool bWrapText );

    bool GetCtrlKeyShortcutsDisabled () const;
    void SetCtrlKeyShortcutsDisabled (_In_ const bool fCtrlKeyShortcutsDisabled );

    BYTE GetWindowAlpha() const;
    void SetWindowAlpha(_In_ const BYTE bWindowAlpha);

    DWORD GetHotKey() const;
    void SetHotKey(_In_ const DWORD dwHotKey);

    bool IsStartupTitleIsLinkNameSet() const;

    DWORD GetStartupFlags() const;
    void SetStartupFlags(_In_ const DWORD dwStartupFlags);
    void UnsetStartupFlag(_In_ DWORD const dwFlagToUnset);

    WORD GetFillAttribute() const;
    void SetFillAttribute(_In_ const WORD wFillAttribute);

    WORD GetPopupFillAttribute() const;
    void SetPopupFillAttribute(_In_ const WORD wPopupFillAttribute);

    WORD GetShowWindow() const;
    void SetShowWindow(_In_ const WORD wShowWindow);

    WORD GetReserved() const;
    void SetReserved(_In_ const WORD wReserved);

    COORD GetScreenBufferSize() const;
    void SetScreenBufferSize(_In_ const COORD dwScreenBufferSize);

    COORD GetWindowSize() const;
    void SetWindowSize(_In_ const COORD dwWindowSize);

    bool IsWindowSizePixelsValid() const;
    COORD GetWindowSizePixels() const;
    void SetWindowSizePixels(_In_ const COORD dwWindowSizePixels);

    COORD GetWindowOrigin() const;
    void SetWindowOrigin(_In_ const COORD dwWindowOrigin);

    DWORD GetFont() const;
    void SetFont(_In_ const DWORD dwFont);

    COORD GetFontSize() const;
    void SetFontSize(_In_ const COORD dwFontSize);

    UINT GetFontFamily() const;
    void SetFontFamily(_In_ const UINT uFontFamily);

    UINT GetFontWeight() const;
    void SetFontWeight(_In_ const UINT uFontWeight);

    const WCHAR* const GetFaceName() const;
    bool IsFaceNameSet() const;
    void SetFaceName(_In_ PCWSTR const pcszFaceName, _In_ size_t const cchLength);

    UINT GetCursorSize() const;
    void SetCursorSize(_In_ const UINT uCursorSize);

    bool GetFullScreen() const;
    void SetFullScreen(_In_ const bool fFullScreen);

    bool GetQuickEdit() const;
    void SetQuickEdit(_In_ const bool fQuickEdit);

    bool GetInsertMode() const;
    void SetInsertMode(_In_ const bool fInsertMode);

    bool GetAutoPosition() const;
    void SetAutoPosition(_In_ const bool fAutoPosition);

    UINT GetHistoryBufferSize() const;
    void SetHistoryBufferSize(_In_ const UINT uHistoryBufferSize);

    UINT GetNumberOfHistoryBuffers() const;
    void SetNumberOfHistoryBuffers(_In_ const UINT uNumberOfHistoryBuffers);

    bool GetHistoryNoDup() const;
    void SetHistoryNoDup(_In_ const bool fHistoryNoDup);

    const COLORREF* const GetColorTable() const;
    const size_t GetColorTableSize() const;
    void SetColorTable(_In_reads_(cSize) const COLORREF* const pColorTable, _In_ size_t const cSize);
    void SetColorTableEntry(_In_ size_t const index, _In_ COLORREF const ColorValue);
    COLORREF GetColorTableEntry(_In_ size_t const index) const;

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
    bool _fTrimLeadingZeros;
    bool _fEnableColorSelection;
    bool _bLineSelection;
    bool _bWrapText; // whether to use text wrapping when resizing the window
    bool _fCtrlKeyShortcutsDisabled; // disables Ctrl+<something> key intercepts
    BYTE _bWindowAlpha; // describes the opacity of the window

    bool _fFilterOnPaste; // should we filter text when the user pastes? (e.g. remove <tab>)
    WCHAR _LaunchFaceName[LF_FACESIZE];
    bool _fAllowAltF4Close;
    DWORD _dwVirtTermLevel;
    bool _fAutoReturnOnNewline;
    bool _fRenderGridWorldwide;

    void _InitColorTable();

    COLORREF _XtermColorTable[XTERM_COLOR_TABLE_SIZE];
    void _InitXtermTableValue(_In_ const size_t iIndex, _In_ const BYTE bRed, _In_ const BYTE bGreen, _In_ const BYTE bBlue);

    // this is used for the special STARTF_USESIZE mode.
    bool _fUseWindowSizePixels;
    COORD _dwWindowSizePixels;

    // Technically a COLORREF, but using INVALID_COLOR as "Invert Colors"
    unsigned int _CursorColor;
    CursorType _CursorType;

    bool _fInterceptCopyPaste;

    friend class RegistrySerialization;

public:

    WORD GenerateLegacyAttributes(_In_ const TextAttribute attributes) const;
    WORD FindNearestTableIndex(_In_ COLORREF const Color) const;

};
