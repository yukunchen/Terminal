/*++
Copyright (c) Microsoft Corporation

Module Name:
- conGetSet.hpp

Abstract:
- This serves as an abstraction layer for the adapters to connect to the console API functions.
- The abstraction allows for the substitution of the functions for internal/external to Conhost.exe use as well as easy testing.

Author(s):
- Michael Niksa (MiNiksa) 30-July-2015
--*/

#pragma once

#include "..\parser\termDispatch.hpp"

namespace Microsoft
{
    namespace Console
    {
        namespace VirtualTerminal
        {
            class ConGetSet
            {
            public:
                virtual BOOL GetConsoleCursorInfo(_In_ CONSOLE_CURSOR_INFO* const pConsoleCursorInfo) const = 0;
                virtual BOOL GetConsoleScreenBufferInfoEx(_Out_ CONSOLE_SCREEN_BUFFER_INFOEX* const pConsoleScreenBufferInfoEx) const = 0;
                virtual BOOL SetConsoleScreenBufferInfoEx(_In_ const CONSOLE_SCREEN_BUFFER_INFOEX* const pConsoleScreenBufferInfoEx) const = 0;
                virtual BOOL SetConsoleCursorInfo(_In_ const CONSOLE_CURSOR_INFO* const pConsoleCursorInfo) = 0;
                virtual BOOL SetConsoleCursorPosition(_In_ COORD const coordCursorPosition) = 0;
                virtual BOOL FillConsoleOutputCharacterW(_In_ WCHAR const wch, _In_ DWORD const nLength, _In_ COORD const dwWriteCoord, _Out_ DWORD* const pNumberOfCharsWritten) = 0;
                virtual BOOL FillConsoleOutputAttribute(_In_ WORD const wAttribute, _In_ DWORD const nLength, _In_ COORD const dwWriteCoord, _Out_ DWORD* const pNumberOfAttrsWritten) = 0;
                virtual BOOL SetConsoleTextAttribute(_In_ WORD const wAttr) = 0;
                virtual BOOL PrivateSetLegacyAttributes(_In_ WORD const wAttr, _In_ const bool fForeground, _In_ const bool fBackground, _In_ const bool fMeta) = 0;
                virtual BOOL SetConsoleXtermTextAttribute(_In_ int const iXtermTableEntry, _In_ const bool fIsForeground) = 0;
                virtual BOOL SetConsoleRGBTextAttribute(_In_ COLORREF const rgbColor, _In_ const bool fIsForeground) = 0;
                virtual BOOL WriteConsoleInputW(_In_reads_(nLength) INPUT_RECORD* const rgInputRecords, _In_ DWORD const nLength, _Out_ DWORD* const pNumberOfEventsWritten) = 0;
                virtual BOOL ScrollConsoleScreenBufferW(_In_ const SMALL_RECT* pScrollRectangle, _In_opt_ const SMALL_RECT* pClipRectangle, _In_ COORD dwDestinationOrigin, _In_ const CHAR_INFO* pFill) = 0;
                virtual BOOL SetConsoleWindowInfo(_In_ BOOL const bAbsolute, _In_ const SMALL_RECT* const lpConsoleWindow) = 0;
                virtual BOOL PrivateSetCursorKeysMode(_In_ bool const fApplicationMode) = 0;
                virtual BOOL PrivateSetKeypadMode(_In_ bool const fApplicationMode) = 0;
                virtual BOOL PrivateAllowCursorBlinking(_In_ bool const fEnable) = 0;
                virtual BOOL PrivateSetScrollingRegion(_In_ const SMALL_RECT* const psrScrollMargins) = 0;
                virtual BOOL PrivateReverseLineFeed() = 0;
                virtual BOOL SetConsoleTitleW(_In_ const wchar_t* const pwchWindowTitle, _In_ unsigned short sCchTitleLength) = 0;
                virtual BOOL PrivateUseAlternateScreenBuffer() = 0;
                virtual BOOL PrivateUseMainScreenBuffer() = 0;
                virtual BOOL PrivateHorizontalTabSet() = 0;
                virtual BOOL PrivateForwardTab(_In_ SHORT const sNumTabs) = 0;
                virtual BOOL PrivateBackwardsTab(_In_ SHORT const sNumTabs) = 0;
                virtual BOOL PrivateTabClear(_In_ bool const fClearAll) = 0;
                virtual BOOL PrivateEnableVT200MouseMode(_In_ bool const fEnabled) = 0;
                virtual BOOL PrivateEnableUTF8ExtendedMouseMode(_In_ bool const fEnabled) = 0;
                virtual BOOL PrivateEnableSGRExtendedMouseMode(_In_ bool const fEnabled) = 0;
                virtual BOOL PrivateEnableButtonEventMouseMode(_In_ bool const fEnabled) = 0;
                virtual BOOL PrivateEnableAnyEventMouseMode(_In_ bool const fEnabled) = 0;
                virtual BOOL PrivateEnableAlternateScroll(_In_ bool const fEnabled) = 0;
                virtual BOOL PrivateEraseAll() = 0;
                virtual BOOL SetCursorStyle(_In_ unsigned int const cursorType) = 0;
            };
        };
    };
};