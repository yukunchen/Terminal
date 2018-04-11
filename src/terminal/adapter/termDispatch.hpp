/*++
Copyright (c) Microsoft Corporation

Module Name:
- termDispatch.hpp

Abstract:
- This is the base class for all output state machine callbacks. When actions
    occur, they will be dispatched to the methods on this interface which must
    be implemented by a child class and passed into the state machine on
    creation.

Author(s):
- Michael Niksa (MiNiksa) 30-July-2015
--*/
#pragma once
#include "DispatchCommon.hpp"

namespace Microsoft::Console::VirtualTerminal
{
    class TermDispatch
    {
    public:
        virtual void Execute(const wchar_t wchControl) = 0;
        virtual void Print(const wchar_t wchPrintable) = 0;
        virtual void PrintString(_In_reads_(cch) wchar_t* const rgwch, const size_t cch) = 0;

        virtual bool CursorUp(_In_ unsigned int const /*uiDistance*/) { return false; }; // CUU
        virtual bool CursorDown(_In_ unsigned int const /*uiDistance*/) { return false; } // CUD
        virtual bool CursorForward(_In_ unsigned int const /*uiDistance*/) { return false; } // CUF
        virtual bool CursorBackward(_In_ unsigned int const /*uiDistance*/) { return false; } // CUB
        virtual bool CursorNextLine(_In_ unsigned int const /*uiDistance*/) { return false; } // CNL
        virtual bool CursorPrevLine(_In_ unsigned int const /*uiDistance*/) { return false; } // CPL
        virtual bool CursorHorizontalPositionAbsolute(_In_ unsigned int const /*uiColumn*/) { return false; } // CHA
        virtual bool VerticalLinePositionAbsolute(_In_ unsigned int const /*uiLine*/) { return false; } // VPA
        virtual bool CursorPosition(_In_ unsigned int const /*uiLine*/, _In_ unsigned int const /*uiColumn*/) { return false; } // CUP
        virtual bool CursorSavePosition() { return false; } // DECSC
        virtual bool CursorRestorePosition() { return false; } // DECRC
        virtual bool CursorVisibility(const bool /*fIsVisible*/) { return false; } // DECTCEM
        virtual bool InsertCharacter(_In_ unsigned int const /*uiCount*/) { return false; } // ICH
        virtual bool DeleteCharacter(_In_ unsigned int const /*uiCount*/) { return false; } // DCH
        virtual bool ScrollUp(_In_ unsigned int const /*uiDistance*/) { return false; } // SU
        virtual bool ScrollDown(_In_ unsigned int const /*uiDistance*/) { return false; } // SD
        virtual bool InsertLine(_In_ unsigned int const /*uiDistance*/) { return false; } // IL
        virtual bool DeleteLine(_In_ unsigned int const /*uiDistance*/) { return false; } // DL
        virtual bool SetColumns(_In_ unsigned int const /*uiColumns*/) { return false; } // DECSCPP, DECCOLM
        virtual bool SetCursorKeysMode(const bool /*fApplicationMode*/) { return false; }  // DECCKM
        virtual bool SetKeypadMode(const bool /*fApplicationMode*/) { return false; }  // DECKPAM, DECKPNM
        virtual bool EnableCursorBlinking(const bool /*fEnable*/) { return false; }  // ATT610
        virtual bool SetTopBottomScrollingMargins(const SHORT /*sTopMargin*/, const SHORT /*sBottomMargin*/, const bool /*fResetCursor*/) { return false; } // DECSTBM
        virtual bool ReverseLineFeed() { return false; } // RI
        virtual bool SetWindowTitle(_In_reads_(_Param_(2)) const wchar_t* const /*pwchWindowTitle*/, _In_ unsigned short /*sCchTitleLength*/) { return false; } // OscWindowTitle
        virtual bool UseAlternateScreenBuffer() { return false; } // ASBSET
        virtual bool UseMainScreenBuffer() { return false; } // ASBRST
        virtual bool HorizontalTabSet() { return false; } // HTS
        virtual bool ForwardTab(const SHORT /*sNumTabs*/) { return false; } // CHT
        virtual bool BackwardsTab(const SHORT /*sNumTabs*/) { return false; } // CBT
        virtual bool TabClear(const SHORT /*sClearType*/) { return false; } // TBC
        virtual bool EnableVT200MouseMode(const bool /*fEnabled*/) { return false; } // ?1000
        virtual bool EnableUTF8ExtendedMouseMode(const bool /*fEnabled*/) { return false; } // ?1005
        virtual bool EnableSGRExtendedMouseMode(const bool /*fEnabled*/) { return false; } // ?1006
        virtual bool EnableButtonEventMouseMode(const bool /*fEnabled*/) { return false; } // ?1002
        virtual bool EnableAnyEventMouseMode(const bool /*fEnabled*/) { return false; } // ?1003
        virtual bool EnableAlternateScroll(const bool /*fEnabled*/) { return false; } // ?1007
        virtual bool SetColorTableEntry(const size_t /*tableIndex*/, const DWORD /*dwColor*/) { return false; } // OSCColorTable

        enum class EraseType : unsigned int
        {
            ToEnd = 0,
            FromBeginning = 1,
            All = 2,
            Scrollback = 3
        };

        virtual bool EraseInDisplay(const EraseType /* eraseType*/) { return false; } // ED
        virtual bool EraseInLine(const EraseType /* eraseType*/) { return false; } // EL
        virtual bool EraseCharacters(_In_ unsigned int const /*uiNumChars*/){ return false; } // ECH

        enum GraphicsOptions : unsigned int
        {
            Off = 0,
            BoldBright = 1,
            RGBColor = 2,
            // 2 is also Faint, decreased intensity (ISO 6429).
            Underline = 4,
            Xterm256Index = 5,
            // 5 is also Blink (appears as Bold).
            // the 2 and 5 entries here are only for the extended graphics options
            // as we do not currently support those features individually
            Negative = 7,
            UnBold = 22,
            NoUnderline = 24,
            Positive = 27,
            ForegroundBlack = 30,
            ForegroundRed = 31,
            ForegroundGreen = 32,
            ForegroundYellow = 33,
            ForegroundBlue = 34,
            ForegroundMagenta = 35,
            ForegroundCyan = 36,
            ForegroundWhite = 37,
            ForegroundExtended = 38,
            ForegroundDefault = 39,
            BackgroundBlack = 40,
            BackgroundRed = 41,
            BackgroundGreen = 42,
            BackgroundYellow = 43,
            BackgroundBlue = 44,
            BackgroundMagenta = 45,
            BackgroundCyan = 46,
            BackgroundWhite = 47,
            BackgroundExtended = 48,
            BackgroundDefault = 49,
            BrightForegroundBlack = 90,
            BrightForegroundRed = 91,
            BrightForegroundGreen = 92,
            BrightForegroundYellow = 93,
            BrightForegroundBlue = 94,
            BrightForegroundMagenta = 95,
            BrightForegroundCyan = 96,
            BrightForegroundWhite = 97,
            BrightBackgroundBlack = 100,
            BrightBackgroundRed = 101,
            BrightBackgroundGreen = 102,
            BrightBackgroundYellow = 103,
            BrightBackgroundBlue = 104,
            BrightBackgroundMagenta = 105,
            BrightBackgroundCyan = 106,
            BrightBackgroundWhite = 107,
        };

        enum PrivateModeParams : unsigned short
        {
            DECCKM_CursorKeysMode = 1,
            DECCOLM_SetNumberOfColumns = 3,
            ATT610_StartCursorBlink = 12,
            DECTCEM_TextCursorEnableMode = 25,
            VT200_MOUSE_MODE = 1000,
            BUTTTON_EVENT_MOUSE_MODE = 1002,
            ANY_EVENT_MOUSE_MODE = 1003,
            UTF8_EXTENDED_MODE = 1005,
            SGR_EXTENDED_MODE = 1006,
            ALTERNATE_SCROLL = 1007,
            ASB_AlternateScreenBuffer = 1049
        };

        static const short s_sDECCOLMSetColumns = 132;
        static const short s_sDECCOLMResetColumns = 80;

        enum VTCharacterSets : wchar_t
        {
            DEC_LineDrawing = L'0',
            USASCII = L'B'
        };

        virtual bool SetGraphicsRendition(_In_reads_(_Param_(2)) const GraphicsOptions* const /*rgOptions*/,
                                            const size_t /*cOptions*/) { return false; } // SGR

        virtual bool SetPrivateModes(_In_reads_(_Param_(2)) const PrivateModeParams* const /*rgParams*/,
                                        const size_t /*cParams*/) { return false; } // DECSET

        virtual bool ResetPrivateModes(_In_reads_(_Param_(2)) const PrivateModeParams* const /*rgParams*/,
                                        const size_t /*cParams*/) { return false; } // DECRST

        enum class AnsiStatusType : unsigned int
        {
            CPR_CursorPositionReport = 6,
        };

        virtual bool DeviceStatusReport(const AnsiStatusType /*statusType*/) { return false; } // DSR
        virtual bool DeviceAttributes() { return false; } // DA

        virtual bool DesignateCharset(const wchar_t /*wchCharset*/){ return false; } // DesignateCharset

        enum TabClearType : unsigned short
        {
            ClearCurrentColumn = 0,
            ClearAllColumns = 3
        };

        virtual bool SoftReset(){ return false; } // DECSTR
        virtual bool HardReset(){ return false; } // RIS

        virtual bool SetCursorStyle(const DispatchCommon::CursorStyle /*cursorStyle*/){ return false; } // DECSCUSR
        virtual bool SetCursorColor(const COLORREF /*Color*/) { return false; } // OSCSetCursorColor, OSCResetCursorColor

        // DTTERM_WindowManipulation
        virtual bool WindowManipulation(const DispatchCommon::WindowManipulationType /*uiFunction*/,
                                        _In_reads_(_Param_(3)) const unsigned short* const /*rgusParams*/,
                                        const size_t /*cParams*/) { return false; }

    };
}
