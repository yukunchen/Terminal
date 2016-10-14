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

// To prevent invisible windows, set a lower threshold on window alpha channel.
#define MIN_WINDOW_OPACITY 0x4D // 0x4D is approximately 30% visible/opaque (70% transparent). Valid range is 0x00-0xff.
#define COLOR_TABLE_SIZE (16)
#define XTERM_COLOR_TABLE_SIZE (256)
class TextAttribute;

class Settings
{
public:
    Settings();

    void ApplyStartupInfo(_In_ const Settings* const pStartupSettings);
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

    bool GetExtendedEditKey() const;
    void SetExtendedEditKey(_In_ bool const fExtendedEditKey);

    BOOL GetFilterOnPaste() const;
    void SetFilterOnPaste(_In_ BOOL const fFilterOnPaste);

    const WCHAR* const GetLaunchFaceName() const;
    void SetLaunchFaceName(_In_ PCWSTR const LaunchFaceName, _In_ size_t const cchLength);

    UINT GetCodePage() const;
    void SetCodePage(_In_ const UINT uCodePage);

    UINT GetScrollScale() const;
    void SetScrollScale(_In_ const UINT uScrollScale);

    BOOL GetTrimLeadingZeros() const;
    void SetTrimLeadingZeros(_In_ const BOOL fTrimLeadingZeros);

    BOOL GetEnableColorSelection() const;
    void SetEnableColorSelection(_In_ const BOOL fEnableColorSelection);

    BOOL GetLineSelection() const;
    void SetLineSelection(_In_ const BOOL bLineSelection);

    bool GetWrapText () const;
    void SetWrapText (_In_ const bool bWrapText );

    BOOL GetCtrlKeyShortcutsDisabled () const;
    void SetCtrlKeyShortcutsDisabled (_In_ const BOOL fCtrlKeyShortcutsDisabled );

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
    
    DWORD GetInputBufferSize() const;
    void SetInputBufferSize(_In_ const DWORD dwInputBufferSize);
    
    COORD GetFontSize() const;
    void SetFontSize(_In_ const COORD dwFontSize);
    
    UINT GetFontFamily() const;
    void SetFontFamily(_In_ const UINT uFontFamily);
    
    UINT GetFontWeight() const;
    void SetFontWeight(_In_ const UINT uFontWeight);
    
    const WCHAR* const GetFaceName() const;
    BOOL IsFaceNameSet() const;
    void SetFaceName(_In_ PCWSTR const pcszFaceName, _In_ size_t const cchLength);
    
    UINT GetCursorSize() const;
    void SetCursorSize(_In_ const UINT uCursorSize);
    
    BOOL GetFullScreen() const;
    void SetFullScreen(_In_ const BOOL fFullScreen);
    
    BOOL GetQuickEdit() const;
    void SetQuickEdit(_In_ const BOOL fQuickEdit);
    
    BOOL GetInsertMode() const;
    void SetInsertMode(_In_ const BOOL fInsertMode);
    
    BOOL GetAutoPosition() const;
    void SetAutoPosition(_In_ const BOOL fAutoPosition);
    
    UINT GetHistoryBufferSize() const;
    void SetHistoryBufferSize(_In_ const UINT uHistoryBufferSize);
    
    UINT GetNumberOfHistoryBuffers() const;
    void SetNumberOfHistoryBuffers(_In_ const UINT uNumberOfHistoryBuffers);
    
    BOOL GetHistoryNoDup() const;
    void SetHistoryNoDup(_In_ const BOOL fHistoryNoDup);
    
    const COLORREF* const GetColorTable() const;
    const size_t GetColorTableSize() const;
    void SetColorTable(_In_reads_(cSize) const COLORREF* const pColorTable, _In_ size_t const cSize);
    void SetColorTableEntry(_In_ size_t const index, _In_ COLORREF const ColorValue);
    COLORREF GetColorTableEntry(_In_ size_t const index) const;


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
    DWORD _nInputBufferSize;
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
    bool _fExtendedEditKey;
    WCHAR _LaunchFaceName[LF_FACESIZE];
    bool _fAllowAltF4Close;
    DWORD _dwVirtTermLevel;
    bool _fAutoReturnOnNewline;
    bool _fRenderGridWorldwide;

    COLORREF _XtermColorTable[XTERM_COLOR_TABLE_SIZE];
    void _InitXtermTableValue(_In_ const size_t iIndex, _In_ const BYTE bRed, _In_ const BYTE bGreen, _In_ const BYTE bBlue);

    // this is used for the special STARTF_USESIZE mode.
    bool _fUseWindowSizePixels;
    COORD _dwWindowSizePixels; 

    friend class RegistrySerialization;

    struct _HSL
    {
        double h, s, l;

        // constructs an HSL color from a RGB Color.
        _HSL(_In_ const COLORREF rgb)
        {
            const double r = (double) GetRValue(rgb);
            const double g = (double) GetGValue(rgb);
            const double b = (double) GetBValue(rgb);
            double min, max, diff, sum;

            max = max(max(r, g), b);
            min = min(min(r, g), b);

            diff = max - min;
            sum = max + min;
            // Luminence
            l = max / 255.0;

            // Saturation
            s = (max == 0) ? 0 : diff / max;

            //Hue
            double q = (diff == 0)? 0 : 60.0/diff;
            if (max == r)
            {
                h = (g < b)? ((360.0 + q * (g - b))/360.0) : ((q * (g - b))/360.0);
            }
            else if (max == g)
            {
                h = (120.0 + q * (b - r))/360.0;
            }
            else if (max == b)
            {
                h = (240.0 + q * (r - g))/360.0;
            }
            else
            {
                h = 0;
            }
        }
    };

public:

    WORD GenerateLegacyAttributes(_In_ const TextAttribute* const pAttributes) const;
    static double s_FindDifference(_In_ const _HSL* const phslColorA, _In_ const COLORREF rgbColorB);
    WORD FindNearestTableIndex(_In_ COLORREF const Color) const;

};
