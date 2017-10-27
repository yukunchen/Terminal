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

namespace Microsoft
{
    namespace Console
    {
        namespace VirtualTerminal
        {
            class TermDispatch
            {
            public:
                virtual void Execute(_In_ wchar_t const wchControl) = 0;
                virtual void Print(_In_ wchar_t const wchPrintable) = 0;
                virtual void PrintString(_In_reads_(cch) wchar_t* const rgwch, _In_ size_t const cch) = 0;

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
                virtual bool CursorVisibility(_In_ bool const /*fIsVisible*/) { return false; } // DECTCEM
                virtual bool InsertCharacter(_In_ unsigned int const /*uiCount*/) { return false; } // ICH
                virtual bool DeleteCharacter(_In_ unsigned int const /*uiCount*/) { return false; } // DCH
                virtual bool ScrollUp(_In_ unsigned int const /*uiDistance*/) { return false; } // SU
                virtual bool ScrollDown(_In_ unsigned int const /*uiDistance*/) { return false; } // SD
                virtual bool InsertLine(_In_ unsigned int const /*uiDistance*/) { return false; } // IL
                virtual bool DeleteLine(_In_ unsigned int const /*uiDistance*/) { return false; } // DL
                virtual bool SetColumns(_In_ unsigned int const /*uiColumns*/) { return false; } // DECSCPP, DECCOLM
                virtual bool SetCursorKeysMode(_In_ bool const /*fApplicationMode*/) { return false; }  // DECCKM
                virtual bool SetKeypadMode(_In_ bool const /*fApplicationMode*/) { return false; }  // DECKPAM, DECKPNM
                virtual bool EnableCursorBlinking(_In_ bool const /*fEnable*/) { return false; }  // ATT610
                virtual bool SetTopBottomScrollingMargins(_In_ SHORT const /*sTopMargin*/, _In_ SHORT const /*sBottomMargin*/) { return false; } // DECSTBM
                virtual bool ReverseLineFeed() { return false; } // RI
                virtual bool SetWindowTitle(_In_ const wchar_t* const /*pwchWindowTitle*/, _In_ unsigned short /*sCchTitleLength*/) { return false; } // OscWindowTitle
                virtual bool UseAlternateScreenBuffer() { return false; } // ASBSET
                virtual bool UseMainScreenBuffer() { return false; } // ASBRST
                virtual bool HorizontalTabSet() { return false; } // HTS
                virtual bool ForwardTab(_In_ SHORT const /*sNumTabs*/) { return false; } // CHT
                virtual bool BackwardsTab(_In_ SHORT const /*sNumTabs*/) { return false; } // CBT
                virtual bool TabClear(_In_ SHORT const /*sClearType*/) { return false; } // TBC
                virtual bool EnableVT200MouseMode(_In_ bool const /*fEnabled*/) { return false; } // ?1000
                virtual bool EnableUTF8ExtendedMouseMode(_In_ bool const /*fEnabled*/) { return false; } // ?1005
                virtual bool EnableSGRExtendedMouseMode(_In_ bool const /*fEnabled*/) { return false; } // ?1006
                virtual bool EnableButtonEventMouseMode(_In_ bool const /*fEnabled*/) { return false; } // ?1002
                virtual bool EnableAnyEventMouseMode(_In_ bool const /*fEnabled*/) { return false; } // ?1003
                virtual bool EnableAlternateScroll(_In_ bool const /*fEnabled*/) { return false; } // ?1007

                enum class EraseType : unsigned int
                {
                    ToEnd = 0,
                    FromBeginning = 1,
                    All = 2,
                    Scrollback = 3
                };

                virtual bool EraseInDisplay(_In_ EraseType const /* eraseType*/) { return false; } // ED
                virtual bool EraseInLine(_In_ EraseType const /* eraseType*/) { return false; } // EL
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

                virtual bool SetGraphicsRendition(_In_reads_(cOptions) const GraphicsOptions* const /*rgOptions*/, _In_ size_t const cOptions) { UNREFERENCED_PARAMETER(cOptions); return false; } // SGR

                virtual bool SetPrivateModes(_In_reads_(cParams) const PrivateModeParams* const /*rgParams*/, _In_ size_t const cParams) { UNREFERENCED_PARAMETER(cParams); return false; } // DECSET
                virtual bool ResetPrivateModes(_In_reads_(cParams) const PrivateModeParams* const /*rgParams*/, _In_ size_t const cParams) { UNREFERENCED_PARAMETER(cParams); return false; } // DECRST

                enum class AnsiStatusType : unsigned int
                {
                    CPR_CursorPositionReport = 6,
                };

                virtual bool DeviceStatusReport(_In_ AnsiStatusType const /*statusType*/) { return false; } // DSR
                virtual bool DeviceAttributes() { return false; } // DA

                virtual bool DesignateCharset(_In_ wchar_t const /*wchCharset*/){ return false; } // DesignateCharset

                enum TabClearType : unsigned short
                {
                    ClearCurrentColumn = 0,
                    ClearAllColumns = 3
                };

                virtual bool SoftReset(){ return false; } // DECSTR
                virtual bool HardReset(){ return false; } // RIS

                // DTTERM_WindowManipulation
                virtual bool WindowManipulation(_In_ const DispatchCommon::WindowManipulationType /*uiFunction*/,
                                                _In_reads_(cParams) const unsigned short* const /*rgusParams*/,
                                                _In_ size_t const /*cParams*/) { return false; } 

            };
        };
    };
};
