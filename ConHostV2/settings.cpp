/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "settings.hpp"

#include "cursor.h"
#include "misc.h"

#pragma hdrstop

Settings::Settings() :
    _dwHotKey(0),
    _dwStartupFlags(0),
    _wFillAttribute(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE), // White (not bright) on black by default
    _wPopupFillAttribute(FOREGROUND_RED | FOREGROUND_BLUE | BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE | BACKGROUND_INTENSITY), // Purple on white (bright) by default
    _wShowWindow(SW_SHOWNORMAL),
    _wReserved(0),
    // dwScreenBufferSize initialized below
    // dwWindowSize initialized below
    // dwWindowOrigin initialized below
    _nFont(0),
    _nInputBufferSize(0), // TODO: reconcile
    // dwFontSize initialized below
    _uFontFamily(0),
    _uFontWeight(0),
    // FaceName initialized below
    _uCursorSize(CURSOR_SMALL_SIZE),
    _bFullScreen(false),
    _bQuickEdit(false),
    _bInsertMode(false),
    _bAutoPosition(true),
    _uHistoryBufferSize(DEFAULT_NUMBER_OF_COMMANDS),
    _uNumberOfHistoryBuffers(DEFAULT_NUMBER_OF_BUFFERS),
    _bHistoryNoDup(false),
    // ColorTable initialized below
    _uCodePage(g_uiOEMCP),
    _uScrollScale(1),
    _bLineSelection(false),
    _bWrapText(false),
    _fCtrlKeyShortcutsDisabled(false),
    _bWindowAlpha(BYTE_MAX), // 255 alpha = opaque. 0 = transparent.
    _fFilterOnPaste(false),
    _fTrimLeadingZeros(FALSE),
    _fEnableColorSelection(FALSE),
    _fExtendedEditKey(false),
    _fAllowAltF4Close(true),
    _dwVirtTermLevel(0),
    _fUseWindowSizePixels(false),
    _fAutoReturnOnNewline(true), // the historic Windows behavior defaults this to on.
    _fRenderGridWorldwide(false) // historically grid lines were only rendered in DBCS codepages, so this is false by default unless otherwise specified.
    // window size pixels initialized below
{
    _dwScreenBufferSize.X = 80;
    _dwScreenBufferSize.Y = 25;

    _dwWindowSize.X = _dwScreenBufferSize.X;
    _dwWindowSize.Y = _dwScreenBufferSize.Y;

    _dwWindowSizePixels = { 0 };

    _dwWindowOrigin.X = 0;
    _dwWindowOrigin.Y = 0;

    ZeroMemory((void*)&_dwFontSize, sizeof(_dwFontSize));
    ZeroMemory((void*)&_FaceName, sizeof(_FaceName));
    ZeroMemory((void*)&_LaunchFaceName, sizeof(_LaunchFaceName));

    _ColorTable[0] = RGB(0x0000, 0x0000, 0x0000);
    _ColorTable[1] = RGB(0x0000, 0x0000, 0x0080);
    _ColorTable[2] = RGB(0x0000, 0x0080, 0x0000);
    _ColorTable[3] = RGB(0x0000, 0x0080, 0x0080);
    _ColorTable[4] = RGB(0x0080, 0x0000, 0x0000);
    _ColorTable[5] = RGB(0x0080, 0x0000, 0x0080);
    _ColorTable[6] = RGB(0x0080, 0x0080, 0x0000);
    _ColorTable[7] = RGB(0x00C0, 0x00C0, 0x00C0);
    _ColorTable[8] = RGB(0x0080, 0x0080, 0x0080);
    _ColorTable[9] = RGB(0x0000, 0x0000, 0x00FF);
    _ColorTable[10] = RGB(0x0000, 0x00FF, 0x0000);
    _ColorTable[11] = RGB(0x0000, 0x00FF, 0x00FF);
    _ColorTable[12] = RGB(0x00FF, 0x0000, 0x0000);
    _ColorTable[13] = RGB(0x00FF, 0x0000, 0x00FF);
    _ColorTable[14] = RGB(0x00FF, 0x00FF, 0x0000);
    _ColorTable[15] = RGB(0x00FF, 0x00FF, 0x00FF);
}

void Settings::ApplyStartupInfo(_In_ const Settings* const pStartupSettings)
{
    const DWORD dwFlags = pStartupSettings->_dwStartupFlags;

    // See: http://msdn.microsoft.com/en-us/library/windows/desktop/ms686331(v=vs.85).aspx

    if (IsFlagSet(dwFlags, STARTF_USECOUNTCHARS))
    {
        _dwScreenBufferSize = pStartupSettings->_dwScreenBufferSize;
    }

    if (IsFlagSet(dwFlags, STARTF_USESIZE))
    {
        // WARNING: This size is in pixels when passed in the create process call.
        // It will need to be divided by the font size before use.
        // All other Window Size values (from registry/shortcuts) are stored in characters.
        _dwWindowSizePixels = pStartupSettings->_dwWindowSize;
        _fUseWindowSizePixels = true;
    }

    if (IsFlagSet(dwFlags, STARTF_USEPOSITION))
    {
        _dwWindowOrigin = pStartupSettings->_dwWindowOrigin;
        _bAutoPosition = FALSE;
    }

    if (IsFlagSet(dwFlags, STARTF_USEFILLATTRIBUTE))
    {
        _wFillAttribute = pStartupSettings->_wFillAttribute;
    }

    if (IsFlagSet(dwFlags, STARTF_USESHOWWINDOW))
    {
        _wShowWindow = pStartupSettings->_wShowWindow;
    }
}

// WARNING: this function doesn't perform any validation or conversion
void Settings::InitFromStateInfo(_In_ PCONSOLE_STATE_INFO pStateInfo)
{
    _wFillAttribute = pStateInfo->ScreenAttributes;
    _wPopupFillAttribute = pStateInfo->PopupAttributes;
    _dwScreenBufferSize = pStateInfo->ScreenBufferSize;
    _dwWindowSize = pStateInfo->WindowSize;
    _dwWindowOrigin.X = (SHORT)pStateInfo->WindowPosX;
    _dwWindowOrigin.Y = (SHORT)pStateInfo->WindowPosY;
    _dwFontSize = pStateInfo->FontSize;
    _uFontFamily = pStateInfo->FontFamily;
    _uFontWeight = pStateInfo->FontWeight;
    StringCchCopyW(_FaceName, ARRAYSIZE(_FaceName), pStateInfo->FaceName);
    _uCursorSize = pStateInfo->CursorSize;
    _bFullScreen = pStateInfo->FullScreen;
    _bQuickEdit = pStateInfo->QuickEdit;
    _bAutoPosition = pStateInfo->AutoPosition;
    _bInsertMode = pStateInfo->InsertMode;
    _bHistoryNoDup = pStateInfo->HistoryNoDup;
    _uHistoryBufferSize = pStateInfo->HistoryBufferSize;
    _uNumberOfHistoryBuffers = pStateInfo->NumberOfHistoryBuffers;
    CopyMemory(_ColorTable, pStateInfo->ColorTable, sizeof(_ColorTable));
    _uCodePage = pStateInfo->CodePage;
    _bWrapText = !!pStateInfo->fWrapText;
    _fFilterOnPaste = pStateInfo->fFilterOnPaste;
    _fCtrlKeyShortcutsDisabled = pStateInfo->fCtrlKeyShortcutsDisabled;
    _bLineSelection = pStateInfo->fLineSelection;
    _bWindowAlpha = pStateInfo->bWindowTransparency;
}

void Settings::Validate()
{
    // If we were explicitly given a size in pixels from the startup info, divide by the font to turn it into characters.
    // See: https://msdn.microsoft.com/en-us/library/windows/desktop/ms686331%28v=vs.85%29.aspx
    if (IsFlagSet(_dwStartupFlags, STARTF_USESIZE))
    {
        // TODO: FIX
        //// Get the font that we're going to use to convert pixels to characters.
        //DWORD const dwFontIndexWant = FindCreateFont(_uFontFamily,
        //                                             _FaceName,
        //                                             _dwFontSize,
        //                                             _uFontWeight,
        //                                             _uCodePage);

        //_dwWindowSize.X /= g_pfiFontInfo[dwFontIndexWant].Size.X;
        //_dwWindowSize.Y /= g_pfiFontInfo[dwFontIndexWant].Size.Y;
    }

    // minimum screen buffer size 1x1
    _dwScreenBufferSize.X = max(_dwScreenBufferSize.X, 1);
    _dwScreenBufferSize.Y = max(_dwScreenBufferSize.Y, 1);

    // minimum window size size 1x1
    _dwWindowSize.X = max(_dwWindowSize.X, 1);
    _dwWindowSize.Y = max(_dwWindowSize.Y, 1);

    // if buffer size is less than window size, increase buffer size to meet window size
    _dwScreenBufferSize.X = max(_dwWindowSize.X, _dwScreenBufferSize.X);
    _dwScreenBufferSize.Y = max(_dwWindowSize.Y, _dwScreenBufferSize.Y);

    // ensure that the window alpha value is not below the minimum. (no invisible windows)
    // if it's below minimum, just set it to the opaque value
    if (_bWindowAlpha < MIN_WINDOW_OPACITY)
    {
        _bWindowAlpha = BYTE_MAX;
    }

    // If text wrapping is on, ensure that the window width is the same as the buffer width.
    if (_bWrapText)
    {
        _dwWindowSize.X = _dwScreenBufferSize.X;
    }

    ASSERT(_dwWindowSize.X > 0);
    ASSERT(_dwWindowSize.Y > 0);
    ASSERT(_dwScreenBufferSize.X > 0);
    ASSERT(_dwScreenBufferSize.Y > 0);
}

DWORD Settings::GetVirtTermLevel() const
{
    return this->_dwVirtTermLevel;
}
void Settings::SetVirtTermLevel(_In_ DWORD const dwVirtTermLevel)
{
    this->_dwVirtTermLevel = dwVirtTermLevel;
}

bool Settings::IsAltF4CloseAllowed() const
{
    return this->_fAllowAltF4Close;
}
void Settings::SetAltF4CloseAllowed(_In_ bool const fAllowAltF4Close)
{
    this->_fAllowAltF4Close = fAllowAltF4Close;
}

bool Settings::IsReturnOnNewlineAutomatic() const
{
    return this->_fAutoReturnOnNewline;
}
void Settings::SetAutomaticReturnOnNewline(_In_ bool const fAutoReturnOnNewline)
{
    this->_fAutoReturnOnNewline = fAutoReturnOnNewline;
}

bool Settings::IsGridRenderingAllowedWorldwide() const
{
    return this->_fRenderGridWorldwide;
}
void Settings::SetGridRenderingAllowedWorldwide(_In_ bool const fGridRenderingAllowed)
{
    this->_fRenderGridWorldwide = fGridRenderingAllowed;

    if (g_pRender != nullptr)
    {
        g_pRender->TriggerRedrawAll();
    }
}

bool Settings::GetExtendedEditKey() const
{
    return this->_fExtendedEditKey;
}
void Settings::SetExtendedEditKey(_In_ bool const fExtendedEditKey)
{
    this->_fExtendedEditKey = fExtendedEditKey;
}

BOOL Settings::GetFilterOnPaste() const
{
    return this->_fFilterOnPaste;
}
void Settings::SetFilterOnPaste(_In_ BOOL const fFilterOnPaste)
{
    this->_fFilterOnPaste = fFilterOnPaste;
}

const WCHAR* const Settings::GetLaunchFaceName() const
{
    return this->_LaunchFaceName;
}
void Settings::SetLaunchFaceName(_In_ PCWSTR const LaunchFaceName, _In_ size_t const cchLength)
{
    StringCchCopyW(this->_LaunchFaceName, cchLength, LaunchFaceName);
}

UINT Settings::GetCodePage() const
{
    return this->_uCodePage;
}
void Settings::SetCodePage(_In_ const UINT uCodePage)
{
    this->_uCodePage = uCodePage;
}

UINT Settings::GetScrollScale() const
{
    return this->_uScrollScale;
}
void Settings::SetScrollScale(_In_ const UINT uScrollScale)
{
    this->_uScrollScale = uScrollScale;
}

BOOL Settings::GetTrimLeadingZeros() const
{
    return this->_fTrimLeadingZeros;
}
void Settings::SetTrimLeadingZeros(_In_ const BOOL fTrimLeadingZeros)
{
    this->_fTrimLeadingZeros = fTrimLeadingZeros;
}

BOOL Settings::GetEnableColorSelection() const
{
    return this->_fEnableColorSelection;
}
void Settings::SetEnableColorSelection(_In_ const BOOL fEnableColorSelection)
{
    this->_fEnableColorSelection = fEnableColorSelection;
}

BOOL Settings::GetLineSelection() const
{
    return this->_bLineSelection;
}
void Settings::SetLineSelection(_In_ const BOOL bLineSelection)
{
    this->_bLineSelection = bLineSelection;
}

bool Settings::GetWrapText () const
{
    return this->_bWrapText;
}
void Settings::SetWrapText (_In_ const bool bWrapText )
{
    this->_bWrapText = bWrapText;
}

BOOL Settings::GetCtrlKeyShortcutsDisabled () const
{
    return this->_fCtrlKeyShortcutsDisabled;
}
void Settings::SetCtrlKeyShortcutsDisabled (_In_ const BOOL fCtrlKeyShortcutsDisabled )
{
    this->_fCtrlKeyShortcutsDisabled = fCtrlKeyShortcutsDisabled;
}

BYTE Settings::GetWindowAlpha() const
{
    return this->_bWindowAlpha;
}
void Settings::SetWindowAlpha(_In_ const BYTE bWindowAlpha)
{
    // if we're out of bounds, set it to 100% opacity so it appears as if nothing happened.
    this->_bWindowAlpha = (bWindowAlpha < MIN_WINDOW_OPACITY)? BYTE_MAX : bWindowAlpha;
}

DWORD Settings::GetHotKey() const
{
    return this->_dwHotKey;
}
void Settings::SetHotKey(_In_ const DWORD dwHotKey)
{
    this->_dwHotKey = dwHotKey;
}

DWORD Settings::GetStartupFlags() const
{
    return this->_dwStartupFlags;
}
void Settings::SetStartupFlags(_In_ const DWORD dwStartupFlags)
{
    this->_dwStartupFlags = dwStartupFlags;
}

WORD Settings::GetFillAttribute() const
{
    return this->_wFillAttribute;
}
void Settings::SetFillAttribute(_In_ const WORD wFillAttribute)
{
    this->_wFillAttribute = wFillAttribute;
}

WORD Settings::GetPopupFillAttribute() const
{
    return this->_wPopupFillAttribute;
}
void Settings::SetPopupFillAttribute(_In_ const WORD wPopupFillAttribute)
{
    this->_wPopupFillAttribute = wPopupFillAttribute;
}

WORD Settings::GetShowWindow() const
{
    return this->_wShowWindow;
}
void Settings::SetShowWindow(_In_ const WORD wShowWindow)
{
    this->_wShowWindow = wShowWindow;
}

WORD Settings::GetReserved() const
{
    return this->_wReserved;
}
void Settings::SetReserved(_In_ const WORD wReserved)
{
    this->_wReserved = wReserved;
}

COORD Settings::GetScreenBufferSize() const
{
    return this->_dwScreenBufferSize;
}
void Settings::SetScreenBufferSize(_In_ const COORD dwScreenBufferSize)
{
    this->_dwScreenBufferSize = dwScreenBufferSize;
}

COORD Settings::GetWindowSize() const
{
    return this->_dwWindowSize;
}
void Settings::SetWindowSize(_In_ const COORD dwWindowSize)
{
    this->_dwWindowSize = dwWindowSize;
}

bool Settings::IsWindowSizePixelsValid() const
{
    return _fUseWindowSizePixels;
}
COORD Settings::GetWindowSizePixels() const
{
    return _dwWindowSizePixels;
}
void Settings::SetWindowSizePixels(_In_ const COORD dwWindowSizePixels)
{
    _dwWindowSizePixels = dwWindowSizePixels;
}

COORD Settings::GetWindowOrigin() const
{
    return this->_dwWindowOrigin;
}
void Settings::SetWindowOrigin(_In_ const COORD dwWindowOrigin)
{
    this->_dwWindowOrigin = dwWindowOrigin;
}

DWORD Settings::GetFont() const
{
    return this->_nFont;
}
void Settings::SetFont(_In_ const DWORD nFont)
{
    this->_nFont = nFont;
}

DWORD Settings::GetInputBufferSize() const
{
    return this->_nInputBufferSize;
}
void Settings::SetInputBufferSize(_In_ const DWORD nInputBufferSize)
{
    this->_nInputBufferSize = nInputBufferSize;
}

COORD Settings::GetFontSize() const
{
    return this->_dwFontSize;
}
void Settings::SetFontSize(_In_ const COORD dwFontSize)
{
    this->_dwFontSize = dwFontSize;
}

UINT Settings::GetFontFamily() const
{
    return this->_uFontFamily;
}
void Settings::SetFontFamily(_In_ const UINT uFontFamily)
{
    this->_uFontFamily = uFontFamily;
}

UINT Settings::GetFontWeight() const
{
    return this->_uFontWeight;
}
void Settings::SetFontWeight(_In_ const UINT uFontWeight)
{
    this->_uFontWeight = uFontWeight;
}

const WCHAR* const Settings::GetFaceName() const
{
    return this->_FaceName;
}
void Settings::SetFaceName(_In_ PCWSTR const pcszFaceName, _In_ size_t const cchLength)
{
    StringCchCopyW(this->_FaceName, cchLength, pcszFaceName);
}

UINT Settings::GetCursorSize() const
{
    return this->_uCursorSize;
}
void Settings::SetCursorSize(_In_ const UINT uCursorSize)
{
    this->_uCursorSize = uCursorSize;
}

BOOL Settings::GetFullScreen() const
{
    return this->_bFullScreen;
}
void Settings::SetFullScreen(_In_ const BOOL bFullScreen)
{
    this->_bFullScreen = bFullScreen;
}

BOOL Settings::GetQuickEdit() const
{
    return this->_bQuickEdit;
}
void Settings::SetQuickEdit(_In_ const BOOL bQuickEdit)
{
    this->_bQuickEdit = bQuickEdit;
}

BOOL Settings::GetInsertMode() const
{
    return this->_bInsertMode;
}
void Settings::SetInsertMode(_In_ const BOOL bInsertMode)
{
    this->_bInsertMode = bInsertMode;
}

BOOL Settings::GetAutoPosition() const
{
    return this->_bAutoPosition;
}
void Settings::SetAutoPosition(_In_ const BOOL bAutoPosition)
{
    this->_bAutoPosition = bAutoPosition;
}

UINT Settings::GetHistoryBufferSize() const
{
    return this->_uHistoryBufferSize;
}
void Settings::SetHistoryBufferSize(_In_ const UINT uHistoryBufferSize)
{
    this->_uHistoryBufferSize = uHistoryBufferSize;
}

UINT Settings::GetNumberOfHistoryBuffers() const
{
    return this->_uNumberOfHistoryBuffers;
}
void Settings::SetNumberOfHistoryBuffers(_In_ const UINT uNumberOfHistoryBuffers)
{
    this->_uNumberOfHistoryBuffers = uNumberOfHistoryBuffers;
}

BOOL Settings::GetHistoryNoDup() const
{
    return this->_bHistoryNoDup;
}
void Settings::SetHistoryNoDup(_In_ const BOOL bHistoryNoDup)
{
    this->_bHistoryNoDup = bHistoryNoDup;
}

const COLORREF* const Settings::GetColorTable() const
{
    return this->_ColorTable;
}
void Settings::SetColorTable(_In_reads_(cSize) const COLORREF* const pColorTable, _In_ size_t const cSize)
{
    size_t cSizeWritten = min(cSize, COLOR_TABLE_SIZE);

    CopyMemory(this->_ColorTable, pColorTable, cSizeWritten * sizeof(COLORREF));
}
void Settings::SetColorTableEntry(_In_ size_t const index, _In_ COLORREF const ColorValue)
{
    this->_ColorTable[index] = ColorValue;
}

BOOL Settings::IsStartupFlagSet(_In_ const int Flag) const
{
    return IsFlagSet(this->_dwStartupFlags, Flag);
}

BOOL Settings::IsFaceNameSet() const
{
    return this->GetFaceName()[0] != '\0';
}

void Settings::UnsetStartupFlag(_In_ DWORD const dwFlagToUnset)
{
    this->_dwStartupFlags &= ~dwFlagToUnset;
}

const size_t Settings::GetColorTableSize() const
{
    return ARRAYSIZE(this->_ColorTable);
}
