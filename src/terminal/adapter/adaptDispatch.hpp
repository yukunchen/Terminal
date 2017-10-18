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

#include "..\parser\termDispatch.hpp"
#include "conGetSet.hpp"
#include "adaptDefaults.hpp"
#include "terminalOutput.hpp"
#include <math.h>

#define XTERM_COLOR_TABLE_SIZE (256)

namespace Microsoft
{
    namespace Console
    {
        namespace VirtualTerminal
        {
            class AdaptDispatch : public TermDispatch
            {
            public:

                static bool CreateInstance(_Inout_ ConGetSet* const pConApi,
                                           _Inout_ AdaptDefaults* const pDefaults,
                                           _In_ const WORD wDefaultTextAttributes,
                                           _Outptr_ AdaptDispatch ** const ppDispatch);

                ~AdaptDispatch();

                void UpdateDefaults(_Inout_ AdaptDefaults* const pDefaults)
                {
                    _pDefaults = pDefaults;
                }

                void UpdateDefaultColor(_In_ WORD const wAttributes)
                {
                    _wDefaultTextAttributes = wAttributes;
                }

                virtual void Execute(_In_ wchar_t const wchControl)
                {
                    _pDefaults->Execute(wchControl);
                }

                virtual void PrintString(_In_reads_(cch) wchar_t* const rgwch, _In_ size_t const cch);
                virtual void Print(_In_ wchar_t const wchPrintable);

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
                virtual bool CursorVisibility(_In_ bool const fIsVisible); // DECTCEM
                virtual bool EraseInDisplay(_In_ EraseType const eraseType); // ED
                virtual bool EraseInLine(_In_ EraseType const eraseType); // EL
                virtual bool EraseCharacters(_In_ unsigned int const uiNumChars); // ECH
                virtual bool InsertCharacter(_In_ unsigned int const uiCount); // ICH
                virtual bool DeleteCharacter(_In_ unsigned int const uiCount); // DCH
                virtual bool SetGraphicsRendition(_In_reads_(cOptions) const GraphicsOptions* const rgOptions,
                                                  _In_ size_t const cOptions); // SGR
                virtual bool DeviceStatusReport(_In_ AnsiStatusType const statusType); // DSR
                virtual bool DeviceAttributes(); // DA
                virtual bool ScrollUp(_In_ unsigned int const uiDistance); // SU
                virtual bool ScrollDown(_In_ unsigned int const uiDistance); // SD
                virtual bool InsertLine(_In_ unsigned int const uiDistance); // IL
                virtual bool DeleteLine(_In_ unsigned int const uiDistance); // DL
                virtual bool SetColumns(_In_ unsigned int const uiColumns); // DECSCPP, DECCOLM
                virtual bool SetPrivateModes(_In_reads_(cParams) const PrivateModeParams* const rParams,
                                             _In_ size_t const cParams); // DECSET
                virtual bool ResetPrivateModes(_In_reads_(cParams) const PrivateModeParams* const rParams,
                                               _In_ size_t const cParams); // DECRST
                virtual bool SetCursorKeysMode(_In_ bool const fApplicationMode);  // DECCKM
                virtual bool SetKeypadMode(_In_ bool const fApplicationMode);  // DECKPAM, DECKPNM
                virtual bool EnableCursorBlinking(_In_ bool const bEnable); // ATT610
                virtual bool SetTopBottomScrollingMargins(_In_ SHORT const sTopMargin,
                                                          _In_ SHORT const sBottomMargin); // DECSTBM
                virtual bool ReverseLineFeed(); // RI
                virtual bool SetWindowTitle(_In_ const wchar_t* const pwchWindowTitle,
                                            _In_ unsigned short cchTitleLength); // OscWindowTitle
                virtual bool UseAlternateScreenBuffer(); // ASBSET
                virtual bool UseMainScreenBuffer(); // ASBRST
                virtual bool HorizontalTabSet(); // HTS
                virtual bool ForwardTab(_In_ SHORT const sNumTabs); // CHT
                virtual bool BackwardsTab(_In_ SHORT const sNumTabs); // CBT
                virtual bool TabClear(_In_ SHORT const sClearType); // TBC
                virtual bool DesignateCharset(_In_ wchar_t const wchCharset); // DesignateCharset
                virtual bool SoftReset(); // DECSTR
                virtual bool HardReset(); // RIS
                virtual bool EnableVT200MouseMode(_In_ bool const fEnabled); // ?1000
                virtual bool EnableUTF8ExtendedMouseMode(_In_ bool const fEnabled); // ?1005
                virtual bool EnableSGRExtendedMouseMode(_In_ bool const fEnabled); // ?1006
                virtual bool EnableButtonEventMouseMode(_In_ bool const fEnabled); // ?1002
                virtual bool EnableAnyEventMouseMode(_In_ bool const fEnabled); // ?1003
                virtual bool EnableAlternateScroll(_In_ bool const fEnabled); // ?1007
                virtual bool WindowManipulation(_In_ const WindowManipulationType uiFunction,
                                                _In_reads_(cParams) const unsigned short* const rgusParams,
                                                _In_ size_t const cParams); // DTTERM_WindowManipulation
            private:
                AdaptDispatch(_Inout_ ConGetSet* const pConApi,
                              _Inout_ AdaptDefaults* const pDefaults,
                              _In_ const WORD wDefaultTextAttributes);

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

                bool _CursorMovement(_In_ CursorDirection const dir, _In_ unsigned int const uiDistance) const;
                bool _CursorMovePosition(_In_opt_ const unsigned int* const puiRow, _In_opt_ const unsigned int* const puiCol) const;
                bool _EraseSingleLineHelper(_In_ const CONSOLE_SCREEN_BUFFER_INFOEX* const pcsbiex, _In_ EraseType const eraseType, _In_ SHORT const sLineId, _In_ WORD const wFillColor) const;
                void _SetGraphicsOptionHelper(_In_ GraphicsOptions const opt, _Inout_ WORD* const pAttr);
                bool _EraseAreaHelper(_In_ COORD const coordStartPosition, _In_ COORD const coordLastPosition, _In_ WORD const wFillColor);
                bool _EraseSingleLineDistanceHelper(_In_ COORD const coordStartPosition, _In_ DWORD const dwLength, _In_ WORD const wFillColor) const;
                bool _EraseScrollback();
                bool _EraseAll();
                void _SetGraphicsOptionHelper(_In_ GraphicsOptions const opt, _Inout_ WORD* const pAttr) const;
                bool _InsertDeleteHelper(_In_ unsigned int const uiCount, _In_ bool const fIsInsert) const;
                bool _ScrollMovement(_In_ ScrollDirection const dir, _In_ unsigned int const uiDistance) const;
                bool _InsertDeleteLines(_In_ unsigned int const uiDistance, _In_ bool const fInsert) const;
                static void s_DisableAllColors(_Inout_ WORD* const pAttr, _In_ bool const fIsForeground);
                static void s_ApplyColors(_Inout_ WORD* const pAttr, _In_ WORD const wApplyThis, _In_ bool const fIsForeground);

                bool _CursorPositionReport() const;

                bool _WriteResponse(_In_reads_(cchReply) PCWSTR pwszReply, _In_ size_t const cchReply) const;
                bool _SetResetPrivateModes(_In_reads_(cParams) const PrivateModeParams* const rgParams, _In_ size_t const cParams, _In_ bool const fEnable);
                bool _PrivateModeParamsHelper(_In_ PrivateModeParams const param, _In_ bool const fEnable);
                bool _DoDECCOLMHelper(_In_ unsigned int uiColumns);
                ConGetSet* _pConApi;
                AdaptDefaults* _pDefaults;
                TerminalOutput* _pTermOutput;

                WORD _wDefaultTextAttributes;
                COORD _coordSavedCursor;
                WORD _wBrightnessState;
                SMALL_RECT _srScrollMargins;

                bool _fIsSetColumnsEnabled;

                bool _fChangedForeground;
                bool _fChangedBackground;
                bool _fChangedMetaAttrs;

                bool _SetRgbColorsHelper(_In_reads_(cOptions) const GraphicsOptions* const rgOptions,
                                         _In_ size_t const cOptions,
                                         _Out_ COLORREF* const prgbColor,
                                         _Out_ bool* const pfIsForeground,
                                         _Out_ size_t* const pcOptionsConsumed);
                static bool s_IsXtermColorOption(_In_ GraphicsOptions const opt);
                static bool s_IsRgbColorOption(_In_ GraphicsOptions const opt);

                bool _ResizeWindow(_In_ const unsigned short usWidth, _In_ const unsigned short usHeight);
            };
        };
    };
};