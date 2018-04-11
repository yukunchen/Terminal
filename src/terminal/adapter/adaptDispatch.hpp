/*++
Copyright (c) Microsoft Corporation

Module Name:
- adaptDispatch.hpp

Abstract:
- This serves as the Windows Console API-specific implementation of the callbacks from our generic Virtual Terminal parser.

Author(s):
- Michael Niksa (MiNiksa) 30-July-2015
--*/

#pragma once

#include "termDispatch.hpp"
#include "conGetSet.hpp"
#include "adaptDefaults.hpp"
#include "terminalOutput.hpp"
#include <math.h>

#define XTERM_COLOR_TABLE_SIZE (256)

namespace Microsoft::Console::VirtualTerminal
{
    class AdaptDispatch : public TermDispatch
    {
    public:

        AdaptDispatch(_Inout_ ConGetSet* const pConApi,
                        _Inout_ AdaptDefaults* const pDefaults,
                        const WORD wDefaultTextAttributes);

        void UpdateDefaults(_Inout_ AdaptDefaults* const pDefaults)
        {
            _pDefaults = pDefaults;
        }

        void UpdateDefaultColor(const WORD wAttributes)
        {
            _wDefaultTextAttributes = wAttributes;
        }

        virtual void Execute(const wchar_t wchControl)
        {
            _pDefaults->Execute(wchControl);
        }

        virtual void PrintString(_In_reads_(cch) wchar_t* const rgwch, const size_t cch);
        virtual void Print(const wchar_t wchPrintable);

        virtual bool CursorUp(_In_ unsigned int const uiDistance); // CUU
        virtual bool CursorDown(_In_ unsigned int const uiDistance); // CUD
        virtual bool CursorForward(_In_ unsigned int const uiDistance); // CUF
        virtual bool CursorBackward(_In_ unsigned int const uiDistance); // CUB
        virtual bool CursorNextLine(_In_ unsigned int const uiDistance); // CNL
        virtual bool CursorPrevLine(_In_ unsigned int const uiDistance); // CPL
        virtual bool CursorHorizontalPositionAbsolute(_In_ unsigned int const uiColumn); // CHA
        virtual bool VerticalLinePositionAbsolute(_In_ unsigned int const uiLine); // VPA
        virtual bool CursorPosition(_In_ unsigned int const uiLine, _In_ unsigned int const uiColumn); // CUP
        virtual bool CursorSavePosition(); // DECSC
        virtual bool CursorRestorePosition(); // DECRC
        virtual bool CursorVisibility(const bool fIsVisible); // DECTCEM
        virtual bool EraseInDisplay(const EraseType eraseType); // ED
        virtual bool EraseInLine(const EraseType eraseType); // EL
        virtual bool EraseCharacters(_In_ unsigned int const uiNumChars); // ECH
        virtual bool InsertCharacter(_In_ unsigned int const uiCount); // ICH
        virtual bool DeleteCharacter(_In_ unsigned int const uiCount); // DCH
        virtual bool SetGraphicsRendition(_In_reads_(cOptions) const GraphicsOptions* const rgOptions,
                                            const size_t cOptions); // SGR
        virtual bool DeviceStatusReport(const AnsiStatusType statusType); // DSR
        virtual bool DeviceAttributes(); // DA
        virtual bool ScrollUp(_In_ unsigned int const uiDistance); // SU
        virtual bool ScrollDown(_In_ unsigned int const uiDistance); // SD
        virtual bool InsertLine(_In_ unsigned int const uiDistance); // IL
        virtual bool DeleteLine(_In_ unsigned int const uiDistance); // DL
        virtual bool SetColumns(_In_ unsigned int const uiColumns); // DECSCPP, DECCOLM
        virtual bool SetPrivateModes(_In_reads_(cParams) const PrivateModeParams* const rParams,
                                        const size_t cParams); // DECSET
        virtual bool ResetPrivateModes(_In_reads_(cParams) const PrivateModeParams* const rParams,
                                        const size_t cParams); // DECRST
        virtual bool SetCursorKeysMode(const bool fApplicationMode);  // DECCKM
        virtual bool SetKeypadMode(const bool fApplicationMode);  // DECKPAM, DECKPNM
        virtual bool EnableCursorBlinking(const bool bEnable); // ATT610
        virtual bool SetTopBottomScrollingMargins(const SHORT sTopMargin,
                                                    const SHORT sBottomMargin,
                                                    const bool fResetCursor); // DECSTBM
        virtual bool ReverseLineFeed(); // RI
        virtual bool SetWindowTitle(_In_reads_(cchTitleLength) const wchar_t* const pwchWindowTitle,
                                    _In_ unsigned short cchTitleLength); // OscWindowTitle
        virtual bool UseAlternateScreenBuffer(); // ASBSET
        virtual bool UseMainScreenBuffer(); // ASBRST
        virtual bool HorizontalTabSet(); // HTS
        virtual bool ForwardTab(const SHORT sNumTabs); // CHT
        virtual bool BackwardsTab(const SHORT sNumTabs); // CBT
        virtual bool TabClear(const SHORT sClearType); // TBC
        virtual bool DesignateCharset(const wchar_t wchCharset); // DesignateCharset
        virtual bool SoftReset(); // DECSTR
        virtual bool HardReset(); // RIS
        virtual bool EnableVT200MouseMode(const bool fEnabled); // ?1000
        virtual bool EnableUTF8ExtendedMouseMode(const bool fEnabled); // ?1005
        virtual bool EnableSGRExtendedMouseMode(const bool fEnabled); // ?1006
        virtual bool EnableButtonEventMouseMode(const bool fEnabled); // ?1002
        virtual bool EnableAnyEventMouseMode(const bool fEnabled); // ?1003
        virtual bool EnableAlternateScroll(const bool fEnabled); // ?1007
        virtual bool SetCursorStyle(const DispatchCommon::CursorStyle cursorStyle); // DECSCUSR
        virtual bool SetCursorColor(const COLORREF cursorColor);

        virtual bool SetColorTableEntry(const size_t tableIndex,
                                        const DWORD dwColor); // OscColorTable
        virtual bool WindowManipulation(const DispatchCommon::WindowManipulationType uiFunction,
                                        _In_reads_(cParams) const unsigned short* const rgusParams,
                                        const size_t cParams); // DTTERM_WindowManipulation

    private:

        enum class CursorDirection
        {
            Up,
            Down,
            Left,
            Right,
            NextLine,
            PrevLine
        };
        enum class ScrollDirection
        {
            Up,
            Down
        };

        bool _CursorMovement(const CursorDirection dir, _In_ unsigned int const uiDistance) const;
        bool _CursorMovePosition(_In_opt_ const unsigned int* const puiRow, _In_opt_ const unsigned int* const puiCol) const;
        bool _EraseSingleLineHelper(const CONSOLE_SCREEN_BUFFER_INFOEX* const pcsbiex, const EraseType eraseType, const SHORT sLineId, const WORD wFillColor) const;
        void _SetGraphicsOptionHelper(const GraphicsOptions opt, _Inout_ WORD* const pAttr);
        bool _EraseAreaHelper(const COORD coordStartPosition, const COORD coordLastPosition, const WORD wFillColor);
        bool _EraseSingleLineDistanceHelper(const COORD coordStartPosition, const DWORD dwLength, const WORD wFillColor) const;
        bool _EraseScrollback();
        bool _EraseAll();
        void _SetGraphicsOptionHelper(const GraphicsOptions opt, _Inout_ WORD* const pAttr) const;
        bool _InsertDeleteHelper(_In_ unsigned int const uiCount, const bool fIsInsert) const;
        bool _ScrollMovement(const ScrollDirection dir, _In_ unsigned int const uiDistance) const;
        bool _InsertDeleteLines(_In_ unsigned int const uiDistance, const bool fInsert) const;
        static void s_DisableAllColors(_Inout_ WORD* const pAttr, const bool fIsForeground);
        static void s_ApplyColors(_Inout_ WORD* const pAttr, const WORD wApplyThis, const bool fIsForeground);

        bool _CursorPositionReport() const;

        bool _WriteResponse(_In_reads_(cchReply) PCWSTR pwszReply, const size_t cchReply) const;
        bool _SetResetPrivateModes(_In_reads_(cParams) const PrivateModeParams* const rgParams, const size_t cParams, const bool fEnable);
        bool _PrivateModeParamsHelper(_In_ PrivateModeParams const param, const bool fEnable);
        bool _DoDECCOLMHelper(_In_ unsigned int uiColumns);

        ConGetSet* _pConApi;
        AdaptDefaults* _pDefaults;
        TerminalOutput _TermOutput;

        WORD _wDefaultTextAttributes;
        COORD _coordSavedCursor;
        WORD _wBrightnessState;
        SMALL_RECT _srScrollMargins;

        bool _fIsSetColumnsEnabled;

        bool _fChangedForeground;
        bool _fChangedBackground;
        bool _fChangedMetaAttrs;

        bool _SetRgbColorsHelper(_In_reads_(cOptions) const GraphicsOptions* const rgOptions,
                                    const size_t cOptions,
                                    _Out_ COLORREF* const prgbColor,
                                    _Out_ bool* const pfIsForeground,
                                    _Out_ size_t* const pcOptionsConsumed);
        static bool s_IsXtermColorOption(const GraphicsOptions opt);
        static bool s_IsRgbColorOption(const GraphicsOptions opt);

    };
}
