/*++
Copyright (c) Microsoft Corporation

Module Name:
- MouseInput.hpp

Abstract:
- This serves as an adapter between mouse input from a user and the virtual terminal sequences that are
  typically emitted by an xterm-compatible console.

Author(s):
- Mike Griese (migrie) 01-Aug-2016
--*/
#pragma once

namespace Microsoft
{
    namespace Console
    {
        namespace VirtualTerminal
        {
            typedef void(*WriteInputEvents)(_In_reads_(cInput) INPUT_RECORD* rgInput, _In_ DWORD cInput);

            class MouseInput sealed
            {
            public:
                MouseInput(_In_ WriteInputEvents const pfnWriteEvents);
                ~MouseInput();

                bool HandleMouse(_In_ const COORD coordMousePosition,
                                 _In_ const unsigned int uiButton,
                                 _In_ const short sModifierKeystate,
                                 _In_ const short sWheelDelta);

                void SetUtf8ExtendedMode(_In_ const bool fEnable);
                void SetSGRExtendedMode(_In_ const bool fEnable);

                void EnableDefaultTracking(_In_ const bool fEnable);
                void EnableButtonEventTracking(_In_ const bool fEnable);
                void EnableAnyEventTracking(_In_ const bool fEnable);

                void EnableAlternateScroll(_In_ const bool fEnable);
                void UseAlternateScreenBuffer();
                void UseMainScreenBuffer();

                enum class ExtendedMode : unsigned int
                {
                    None,
                    Utf8,
                    Sgr,
                    Urxvt
                };

                enum class TrackingMode : unsigned int
                {
                    None,
                    Default,
                    ButtonEvent,
                    AnyEvent
                };

            private:
                static const int s_MaxDefaultCoordinate = 94;

                // static const wchar_t* const s_pwchCursorUpSequence = L"\x1b[A";
                // static const wchar_t* const s_pwchCursorDownSequence = L"\x1b[B";
                static const size_t cchCursorSequences = 3;

                WriteInputEvents _pfnWriteEvents;
                
                ExtendedMode _ExtendedMode = ExtendedMode::None;
                TrackingMode _TrackingMode = TrackingMode::None;
                
                bool _fAlternateScroll = false;
                bool _fInAlternateBuffer = false;
                
                COORD _coordLastPos = {-1,-1};

                void _SendInputSequence(_In_reads_(cchLength) const wchar_t* const pwszSequence, _In_ const size_t cchLength) const;
                bool _GenerateDefaultSequence(_In_ const COORD coordMousePosition,
                                              _In_ const unsigned int uiButton,
                                              _In_ const bool fIsHover,
                                              _In_ const short sModifierKeystate,
                                              _In_ const short sWheelDelta,
                                              _Out_ wchar_t** const ppwchSequence,
                                              _Out_ size_t* const pcchLength) const;
                bool _GenerateUtf8Sequence(_In_ const COORD coordMousePosition,
                                           _In_ const unsigned int uiButton,
                                           _In_ const bool fIsHover,
                                           _In_ const short sModifierKeystate,
                                           _In_ const short sWheelDelta,
                                           _Out_ wchar_t** const ppwchSequence,
                                           _Out_ size_t* const pcchLength) const;
                bool _GenerateSGRSequence(_In_ const COORD coordMousePosition,
                                          _In_ const unsigned int uiButton,
                                          _In_ const bool fIsHover,
                                          _In_ const short sModifierKeystate,
                                          _In_ const short sWheelDelta,
                                          _Out_ wchar_t** const ppwchSequence,
                                          _Out_ size_t* const pcchLength) const;

                bool _ShouldSendAlternateScroll(_In_ unsigned int uiButton, _In_ short sScrollDelta) const;
                bool _SendAlternateScroll(_In_ short sScrollDelta) const;

                static int s_WindowsButtonToXEncoding(_In_ const unsigned int uiButton,
                                                      _In_ const bool fIsHover,
                                                      _In_ const short sModifierKeystate,
                                                      _In_ const short sWheelDelta);
                static bool s_IsButtonDown(_In_ const unsigned int uiButton);
                static bool s_IsButtonMsg(_In_ const unsigned int uiButton);
                static bool s_IsHoverMsg(_In_ const unsigned int uiButton);
                static COORD s_WinToVTCoord(_In_ const COORD coordWinCoordinate);
                static short s_EncodeDefaultCoordinate(_In_ const short sCoordinateValue);
                static unsigned int s_GetPressedButton();
            };
        };
    };
};
