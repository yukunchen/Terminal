/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"
#include <wextestclass.h>
#include "..\..\inc\consoletaeftemplates.hpp"

#include "adaptDispatch.hpp"

using namespace WEX::Common;
using namespace WEX::Logging;
using namespace WEX::TestExecution;

namespace Microsoft
{
    namespace Console
    {
        namespace VirtualTerminal
        {
            class AdapterTest;
            class ConAdapterTestGetSet;
        };
    };
};


enum class CursorY
{
    TOP,
    BOTTOM,
    YCENTER
};

enum class CursorX
{
    LEFT,
    RIGHT,
    XCENTER
};

enum class CursorDirection : unsigned int
{
    UP = 0,
    DOWN = 1,
    RIGHT = 2,
    LEFT = 3,
    NEXTLINE = 4,
    PREVLINE = 5
};

enum class ScrollDirection : unsigned int
{
    UP = 0,
    DOWN = 1
};

enum class AbsolutePosition : unsigned int
{
    CursorHorizontal = 0,
    VerticalLine = 1,
};

using namespace Microsoft::Console::VirtualTerminal;

class TestGetSet : public ConGetSet
{
public:
    virtual BOOL GetConsoleScreenBufferInfoEx(_Out_ CONSOLE_SCREEN_BUFFER_INFOEX* const psbiex) const
    {
        Log::Comment(L"GetConsoleScreenBufferInfoEx MOCK returning data...");

        if (_fGetConsoleScreenBufferInfoExResult)
        {
            psbiex->dwSize = _coordBufferSize;
            psbiex->srWindow = _srViewport;
            psbiex->dwCursorPosition = _coordCursorPos;
            psbiex->wAttributes = _wAttribute;
        }

        return _fGetConsoleScreenBufferInfoExResult;
    }
    virtual BOOL SetConsoleScreenBufferInfoEx(_In_ const CONSOLE_SCREEN_BUFFER_INFOEX* const psbiex) const
    {
        Log::Comment(L"SetConsoleScreenBufferInfoEx MOCK returning data...");

        if (_fSetConsoleScreenBufferInfoExResult)
        {
            VERIFY_ARE_EQUAL(_coordExpectedCursorPos, psbiex->dwCursorPosition);
            VERIFY_ARE_EQUAL(_coordExpectedScreenBufferSize, psbiex->dwSize);
            VERIFY_ARE_EQUAL(_srExpectedScreenBufferViewport, psbiex->srWindow);
            VERIFY_ARE_EQUAL(_wExpectedAttributes, psbiex->wAttributes);
        }
        return _fSetConsoleScreenBufferInfoExResult;
    }
    virtual BOOL SetConsoleCursorPosition(_In_ COORD const dwCursorPosition)
    {
        Log::Comment(L"SetConsoleCursorPosition MOCK called...");

        if (_fSetConsoleCursorPositionResult)
        {
            VERIFY_ARE_EQUAL(_coordExpectedCursorPos, dwCursorPosition);
            _coordCursorPos = dwCursorPosition;
        }

        return _fSetConsoleCursorPositionResult;
    }

    virtual BOOL GetConsoleCursorInfo(_In_ CONSOLE_CURSOR_INFO* const pConsoleCursorInfo) const
    {
        Log::Comment(L"GetConsoleCursorInfo MOCK called...");

        if (_fGetConsoleCursorInfoResult)
        {
            pConsoleCursorInfo->dwSize = _dwCursorSize;
            pConsoleCursorInfo->bVisible = _fCursorVisible;
        }

        return _fGetConsoleCursorInfoResult;
    }

    virtual BOOL SetConsoleCursorInfo(_In_ const CONSOLE_CURSOR_INFO* const pConsoleCursorInfo)
    {
        Log::Comment(L"SetConsoleCursorInfo MOCK called...");

        if (_fSetConsoleCursorInfoResult)
        {
            VERIFY_ARE_EQUAL(_dwExpectedCursorSize, pConsoleCursorInfo->dwSize);
            VERIFY_ARE_EQUAL(_fExpectedCursorVisible, pConsoleCursorInfo->bVisible);
        }

        return _fSetConsoleCursorInfoResult;
    }

    virtual BOOL SetConsoleWindowInfo(_In_ BOOL const bAbsolute, _In_ const SMALL_RECT* const lpConsoleWindow)
    {
        Log::Comment(L"SetConsoleWindowInfo MOCK called...");

        if (_fSetConsoleWindowInfoResult)
        {
            VERIFY_ARE_EQUAL(_fExpectedWindowAbsolute, bAbsolute);
            VERIFY_ARE_EQUAL(_srExpectedConsoleWindow, *lpConsoleWindow);
            _srViewport = *lpConsoleWindow;
        }

        return _fSetConsoleWindowInfoResult;
    }

    virtual BOOL PrivateSetCursorKeysMode(_In_ bool const fCursorKeysApplicationMode)
    {
        Log::Comment(L"PrivateSetCursorKeysMode MOCK called...");

        if (_fPrivateSetCursorKeysModeResult)
        {
            VERIFY_ARE_EQUAL(_fCursorKeysApplicationMode, fCursorKeysApplicationMode);
        }

        return _fPrivateSetCursorKeysModeResult;
    }

    virtual BOOL PrivateSetKeypadMode(_In_ bool const fKeypadApplicationMode)
    {
        Log::Comment(L"PrivateSetKeypadMode MOCK called...");

        if (_fPrivateSetKeypadModeResult)
        {
            VERIFY_ARE_EQUAL(_fKeypadApplicationMode, fKeypadApplicationMode);
        }

        return _fPrivateSetKeypadModeResult;
    }

    virtual BOOL PrivateAllowCursorBlinking(_In_ bool const fEnable)
    {
        Log::Comment(L"PrivateAllowCursorBlinking MOCK called...");

        if (_fPrivateAllowCursorBlinkingResult)
        {
            VERIFY_ARE_EQUAL(_fEnable, fEnable);
        }

        return _fPrivateAllowCursorBlinkingResult;
    }

    virtual BOOL FillConsoleOutputCharacterW(_In_ WCHAR const wch, _In_ DWORD const nLength, _In_ COORD const dwWriteCoord, _Out_ DWORD* const pNumberOfCharsWritten)
    {
        Log::Comment(L"FillConsoleOutputCharacterW MOCK called...");

        DWORD dwCharsWritten = 0;

        if (_fFillConsoleOutputCharacterWResult)
        {
            Log::Comment(NoThrowString().Format(L"Filling (X: %d, Y:%d) for %d characters with '%c'...", dwWriteCoord.X, dwWriteCoord.Y, nLength, wch));

            COORD dwCurrentPos = dwWriteCoord;

            while (dwCharsWritten < nLength)
            {
                CHAR_INFO* pchar = _GetCharAt(dwCurrentPos.Y, dwCurrentPos.X);
                pchar->Char.UnicodeChar = wch;
                dwCharsWritten++;
                _IncrementCoordPos(&dwCurrentPos);
            }
        }

        *pNumberOfCharsWritten = dwCharsWritten;

        Log::Comment(NoThrowString().Format(L"Fill wrote %d characters.", dwCharsWritten));

        return _fFillConsoleOutputCharacterWResult;
    }

    virtual BOOL FillConsoleOutputAttribute(_In_ WORD const wAttribute, _In_ DWORD const nLength, _In_ COORD const dwWriteCoord, _Out_ DWORD* const pNumberOfAttrsWritten)
    {
        Log::Comment(L"FillConsoleOutputAttribute MOCK called...");

        DWORD dwCharsWritten = 0;

        if (_fFillConsoleOutputAttributeResult)
        {
            Log::Comment(NoThrowString().Format(L"Filling (X: %d, Y:%d) for %d characters with 0x%x attribute...", dwWriteCoord.X, dwWriteCoord.Y, nLength, wAttribute));

            COORD dwCurrentPos = dwWriteCoord;

            while (dwCharsWritten < nLength)
            {
                CHAR_INFO* pchar = _GetCharAt(dwCurrentPos.Y, dwCurrentPos.X);
                pchar->Attributes = wAttribute;
                dwCharsWritten++;
                _IncrementCoordPos(&dwCurrentPos);
            }
        }

        *pNumberOfAttrsWritten = dwCharsWritten;

        Log::Comment(NoThrowString().Format(L"Fill modified %d characters.", dwCharsWritten));

        return _fFillConsoleOutputAttributeResult;
    }

    virtual BOOL SetConsoleTextAttribute(_In_ WORD const wAttr)
    {
        Log::Comment(L"SetConsoleTextAttribute MOCK called...");

        if (_fSetConsoleTextAttributeResult)
        {
            VERIFY_ARE_EQUAL(_wExpectedAttribute, wAttr);
            _wAttribute = wAttr;
            _fUsingRgbColor = false;
        }

        return _fSetConsoleTextAttributeResult;
    }

    virtual BOOL PrivateSetLegacyAttributes(_In_ WORD const wAttr, _In_ const bool fForeground, _In_ const bool fBackground, _In_ const bool fMeta)
    {
        Log::Comment(L"PrivateSetLegacyAttributes MOCK called...");
        if (_fPrivateSetLegacyAttributesResult)
        {
            VERIFY_ARE_EQUAL(_fExpectedForeground, fForeground);
            VERIFY_ARE_EQUAL(_fExpectedBackground, fBackground);
            VERIFY_ARE_EQUAL(_fExpectedMeta, fMeta);
            if (fForeground)
            {
                UpdateFlagsInMask(_wAttribute, FG_ATTRS, wAttr);
            }
            if (fBackground)
            {
                UpdateFlagsInMask(_wAttribute, BG_ATTRS, wAttr);
            }
            if (fMeta)
            {
                UpdateFlagsInMask(_wAttribute, META_ATTRS, wAttr);
            }

            VERIFY_ARE_EQUAL(_wExpectedAttribute, wAttr);

            _fExpectedForeground = _fExpectedBackground = _fExpectedMeta = false;
        }

        return _fPrivateSetLegacyAttributesResult;
    }

    virtual BOOL SetConsoleXtermTextAttribute(_In_ int const iXtermTableEntry, _In_ const bool fIsForeground)
    {
        Log::Comment(L"SetConsoleXtermTextAttribute MOCK called...");

        if (_fSetConsoleXtermTextAttributeResult)
        {
            VERIFY_ARE_EQUAL(_fExpectedIsForeground, fIsForeground);
            _fIsForeground = fIsForeground;
            VERIFY_ARE_EQUAL(_iExpectedXtermTableEntry, iXtermTableEntry);
            _iXtermTableEntry = iXtermTableEntry;
            // if the table entry is less than 16, keep using the legacy attr
            _fUsingRgbColor = iXtermTableEntry > 16;
            if (!_fUsingRgbColor)
            {
                //Convert the xterm index to the win index
                bool fRed = (iXtermTableEntry & 0x01) > 0;
                bool fGreen = (iXtermTableEntry & 0x02) > 0;
                bool fBlue = (iXtermTableEntry & 0x04) > 0;
                bool fBright = (iXtermTableEntry & 0x08) > 0;
                WORD iWinEntry = (fRed? 0x4:0x0) | (fGreen? 0x2:0x0) | (fBlue? 0x1:0x0) | (fBright? 0x8:0x0);
                _wAttribute = fIsForeground ? ((_wAttribute & 0xF0) | iWinEntry)
                                            : ((iWinEntry<<4) | (_wAttribute & 0x0F));
            }
        }

        return _fSetConsoleXtermTextAttributeResult;
    }

    virtual BOOL SetConsoleRGBTextAttribute(_In_ COLORREF const rgbColor, _In_ const bool fIsForeground)
    {
        Log::Comment(L"SetConsoleRGBTextAttribute MOCK called...");
        if (_fSetConsoleRGBTextAttributeResult)
        {
            VERIFY_ARE_EQUAL(_fExpectedIsForeground, fIsForeground);
            _fIsForeground = fIsForeground;
            VERIFY_ARE_EQUAL(_ExpectedColor, rgbColor);
            _rgbColor = rgbColor;
            _fUsingRgbColor = true;
        }

        return _fSetConsoleRGBTextAttributeResult;
    }

    virtual BOOL WriteConsoleInputW(_Inout_ std::deque<std::unique_ptr<IInputEvent>>& events,
                                    _Out_ size_t& eventsWritten)
    {
        Log::Comment(L"WriteConsoleInputW MOCK called...");

        if (_fWriteConsoleInputWResult)
        {
            // move all the input events we were given into local storage so we can test against them
            Log::Comment(NoThrowString().Format(L"Moving %zu input events into local storage...", events.size()));

            _events.clear();
            _events.swap(events);
            eventsWritten = _events.size();
        }

        return _fWriteConsoleInputWResult;
    }
    
    virtual BOOL PrivatePrependConsoleInput(_Inout_ std::deque<std::unique_ptr<IInputEvent>>& events,
                                            _Out_ size_t& eventsWritten)
    {
        Log::Comment(L"PrivatePrependConsoleInput MOCK called...");

        if (_fPrivatePrependConsoleInputResult)
        {
            // move all the input events we were given into local storage so we can test against them
            Log::Comment(NoThrowString().Format(L"Moving %zu input events into local storage...", events.size()));

            _events.clear();
            _events.swap(events);
            eventsWritten = _events.size();
        }

        return _fPrivatePrependConsoleInputResult;
    }

    virtual BOOL PrivateWriteConsoleControlInput(_In_ KeyEvent key)
    {
        Log::Comment(L"PrivateWriteConsoleControlInput MOCK called...");

        if (_fPrivateWriteConsoleControlInputResult)
        {
            VERIFY_ARE_EQUAL('C', key.GetVirtualKeyCode());
            VERIFY_ARE_EQUAL(0x3, key.GetCharData());
            VERIFY_ARE_EQUAL(true, key.IsCtrlPressed());
        }

        return _fPrivateWriteConsoleControlInputResult;
    }

    bool _IsInsideClip(_In_ const SMALL_RECT* const pClipRectangle, _In_ SHORT const iRow, _In_ SHORT const iCol)
    {
        if (pClipRectangle == nullptr)
        {
            return true;
        }
        else
        {
            return iRow >= pClipRectangle->Top && iRow < pClipRectangle->Bottom && iCol >= pClipRectangle->Left && iCol < pClipRectangle->Right;
        }
    }

    virtual BOOL ScrollConsoleScreenBufferW(_In_ const SMALL_RECT* pScrollRectangle, _In_opt_ const SMALL_RECT* pClipRectangle, _In_ COORD dwDestinationOrigin, _In_ const CHAR_INFO* pFill)
    {
        Log::Comment(L"ScrollConsoleScreenBufferW MOCK called...");

        if (_fScrollConsoleScreenBufferWResult)
        {
            if (pClipRectangle != nullptr)
            {
                Log::Comment(NoThrowString().Format(
                    L"\tScrolling Rectangle (T: %d, B: %d, L: %d, R: %d) "
                    L"into new top-left coordinate (X: %d, Y:%d) with Fill ('%c', 0x%x) "
                    L"clipping to (T: %d, B: %d, L: %d, R: %d)...",
                    pScrollRectangle->Top, pScrollRectangle->Bottom, pScrollRectangle->Left, pScrollRectangle->Right,
                    dwDestinationOrigin.X, dwDestinationOrigin.Y, pFill->Char.UnicodeChar, pFill->Attributes,
                    pClipRectangle->Top, pClipRectangle->Bottom, pClipRectangle->Left, pClipRectangle->Right));
            }
            else
            {
                Log::Comment(NoThrowString().Format(
                    L"\tScrolling Rectangle (T: %d, B: %d, L: %d, R: %d) "
                    L"into new top-left coordinate (X: %d, Y:%d) with Fill ('%c', 0x%x) ",
                    pScrollRectangle->Top, pScrollRectangle->Bottom, pScrollRectangle->Left, pScrollRectangle->Right,
                    dwDestinationOrigin.X, dwDestinationOrigin.Y, pFill->Char.UnicodeChar, pFill->Attributes));
            }

            // allocate buffer space to hold scrolling rectangle
            SHORT width = pScrollRectangle->Right - pScrollRectangle->Left;
            SHORT height = pScrollRectangle->Bottom - pScrollRectangle->Top + 1;
            size_t const cch = width * height;
            CHAR_INFO* const ciBuffer = new CHAR_INFO[cch];
            size_t cciFilled = 0;

            Log::Comment(NoThrowString().Format(L"\tCopy buffer size is %zu chars", cch));

            for (SHORT iCharY = pScrollRectangle->Top; iCharY <= pScrollRectangle->Bottom; iCharY++)
            {
                // back up space and fill it with the fill.
                for (SHORT iCharX = pScrollRectangle->Left; iCharX < pScrollRectangle->Right; iCharX++)
                {

                    COORD coordTarget;
                    coordTarget.X = (SHORT)iCharX;
                    coordTarget.Y = iCharY;

                    CHAR_INFO* const pciStored = _GetCharAt(coordTarget.Y, coordTarget.X);

                    // back up to buffer
                    ciBuffer[cciFilled] = *pciStored;
                    cciFilled++;

                    // fill with fill
                    if (_IsInsideClip(pClipRectangle, coordTarget.Y, coordTarget.X))
                    {
                        *pciStored = *pFill;
                    }
                }

            }
            Log::Comment(NoThrowString().Format(L"\tCopied a total %zu chars", cciFilled));
            Log::Comment(L"\tCopying chars back");
            for (SHORT iCharY = pScrollRectangle->Top; iCharY <= pScrollRectangle->Bottom; iCharY++)
            {
                // back up space and fill it with the fill.
                for (SHORT iCharX = pScrollRectangle->Left; iCharX < pScrollRectangle->Right; iCharX++)
                {
                    COORD coordTarget;
                    coordTarget.X = dwDestinationOrigin.X + (iCharX - pScrollRectangle->Left);
                    coordTarget.Y = dwDestinationOrigin.Y + (iCharY - pScrollRectangle->Top);

                    CHAR_INFO* const pciStored = _GetCharAt(coordTarget.Y, coordTarget.X);

                    if (_IsInsideClip(pClipRectangle, coordTarget.Y, coordTarget.X) && _IsInsideClip(pClipRectangle, iCharY, iCharX))
                    {
                        size_t index = (width) * (iCharY - pScrollRectangle->Top) + (iCharX - pScrollRectangle->Left);
                        CHAR_INFO charFromBuffer = ciBuffer[index];
                        *pciStored = charFromBuffer;
                    }
                }
            }

            delete[] ciBuffer;
        }

        return _fScrollConsoleScreenBufferWResult;
    }

    virtual BOOL PrivateSetScrollingRegion(_In_ const SMALL_RECT* const psrScrollMargins)
    {
        Log::Comment(L"PrivateSetScrollingRegion MOCK called...");

        if (_fPrivateSetScrollingRegionResult)
        {
            VERIFY_ARE_EQUAL(_srExpectedScrollRegion, *psrScrollMargins);
        }

        return _fPrivateSetScrollingRegionResult;
    }

    virtual BOOL PrivateReverseLineFeed()
    {
        Log::Comment(L"PrivateReverseLineFeed MOCK called...");
        // We made it through the adapter, woo! Return true.
        return TRUE;
    }

    virtual BOOL SetConsoleTitleW(_In_ const wchar_t* const pwchWindowTitle, _In_ unsigned short sCchTitleLength)
    {
        Log::Comment(L"SetConsoleTitleW MOCK called...");

        if (_fSetConsoleTitleWResult)
        {
            VERIFY_ARE_EQUAL(_pwchExpectedWindowTitle, pwchWindowTitle);
            VERIFY_ARE_EQUAL(_sCchExpectedTitleLength, sCchTitleLength);
        }
        return TRUE;
    }

    virtual BOOL PrivateUseAlternateScreenBuffer()
    {
        Log::Comment(L"PrivateUseAlternateScreenBuffer MOCK called...");
        return true;
    }

    virtual BOOL PrivateUseMainScreenBuffer()
    {
        Log::Comment(L"PrivateUseMainScreenBuffer MOCK called...");
        return true;
    }

    virtual BOOL PrivateHorizontalTabSet()
    {
        Log::Comment(L"PrivateHorizontalTabSet MOCK called...");
        // We made it through the adapter, woo! Return true.
        return TRUE;
    }

    virtual BOOL PrivateForwardTab(_In_ SHORT const sNumTabs)
    {
        Log::Comment(L"PrivateForwardTab MOCK called...");
        if (_fPrivateForwardTabResult)
        {
            VERIFY_ARE_EQUAL(_sExpectedNumTabs, sNumTabs);
        }
        return TRUE;
    }

    virtual BOOL PrivateBackwardsTab(_In_ SHORT const sNumTabs)
    {
        Log::Comment(L"PrivateBackwardsTab MOCK called...");
        if (_fPrivateBackwardsTabResult)
        {
            VERIFY_ARE_EQUAL(_sExpectedNumTabs, sNumTabs);
        }
        return TRUE;
    }

    virtual BOOL PrivateTabClear(_In_ bool const fClearAll)
    {
        Log::Comment(L"PrivateTabClear MOCK called...");
        if (_fPrivateTabClearResult)
        {
            VERIFY_ARE_EQUAL(_fExpectedClearAll, fClearAll);
        }
        return TRUE;
    }

    virtual BOOL PrivateEnableVT200MouseMode(_In_ bool const fEnabled)
    {
        Log::Comment(L"PrivateEnableVT200MouseMode MOCK called...");
        if (_fPrivateEnableVT200MouseModeResult)
        {
            VERIFY_ARE_EQUAL(_fExpectedMouseEnabled, fEnabled);
        }
        return _fPrivateEnableVT200MouseModeResult;
    }

    virtual BOOL PrivateEnableUTF8ExtendedMouseMode(_In_ bool const fEnabled)
    {
        Log::Comment(L"PrivateEnableUTF8ExtendedMouseMode MOCK called...");
        if (_fPrivateEnableUTF8ExtendedMouseModeResult)
        {
            VERIFY_ARE_EQUAL(_fExpectedMouseEnabled, fEnabled);
        }
        return _fPrivateEnableUTF8ExtendedMouseModeResult;
    }

    virtual BOOL PrivateEnableSGRExtendedMouseMode(_In_ bool const fEnabled)
    {
        Log::Comment(L"PrivateEnableSGRExtendedMouseMode MOCK called...");
        if (_fPrivateEnableSGRExtendedMouseModeResult)
        {
            VERIFY_ARE_EQUAL(_fExpectedMouseEnabled, fEnabled);
        }
        return _fPrivateEnableSGRExtendedMouseModeResult;
    }

    virtual BOOL PrivateEnableButtonEventMouseMode(_In_ bool const fEnabled)
    {
        Log::Comment(L"PrivateEnableButtonEventMouseMode MOCK called...");
        if (_fPrivateEnableButtonEventMouseModeResult)
        {
            VERIFY_ARE_EQUAL(_fExpectedMouseEnabled, fEnabled);
        }
        return _fPrivateEnableButtonEventMouseModeResult;
    }

    virtual BOOL PrivateEnableAnyEventMouseMode(_In_ bool const fEnabled)
    {
        Log::Comment(L"PrivateEnableAnyEventMouseMode MOCK called...");
        if (_fPrivateEnableAnyEventMouseModeResult)
        {
            VERIFY_ARE_EQUAL(_fExpectedMouseEnabled, fEnabled);
        }
        return _fPrivateEnableAnyEventMouseModeResult;
    }

    virtual BOOL PrivateEnableAlternateScroll(_In_ bool const fEnabled)
    {
        Log::Comment(L"PrivateEnableAlternateScroll MOCK called...");
        if (_fPrivateEnableAlternateScrollResult)
        {
            VERIFY_ARE_EQUAL(_fExpectedAlternateScrollEnabled, fEnabled);
        }
        return _fPrivateEnableAlternateScrollResult;
    }

    virtual BOOL PrivateEraseAll()
    {
        Log::Comment(L"PrivateEraseAll MOCK called...");
        return TRUE;
    }

    virtual BOOL PrivateGetConsoleScreenBufferAttributes(_Out_ WORD* const pwAttributes)
    {
        Log::Comment(L"PrivateGetConsoleScreenBufferAttributes MOCK returning data...");

        if (pwAttributes != nullptr && _fPrivateGetConsoleScreenBufferAttributesResult)
        {
            *pwAttributes = _wAttribute;
        }

        return _fPrivateGetConsoleScreenBufferAttributesResult;
    }
    
    virtual BOOL PrivateRefreshWindow()
    {
        Log::Comment(L"PrivateRefreshWindow MOCK called...");
        // We made it through the adapter, woo! Return true.
        return TRUE;
    }

    void _IncrementCoordPos(_Inout_ COORD* pcoord)
    {
        pcoord->X++;

        if (pcoord->X >= _coordBufferSize.X)
        {
            pcoord->X = 0;
            pcoord->Y++;

            if (pcoord->Y >= _coordBufferSize.Y)
            {
                pcoord->Y = _coordBufferSize.Y - 1;
            }
        }
    }

    void PrepData()
    {
        PrepData(CursorDirection::UP); // if called like this, the cursor direction doesn't matter.
    }

    void PrepData(CursorDirection dir)
    {
        switch (dir)
        {
        case CursorDirection::UP:
            return PrepData(CursorX::LEFT, CursorY::TOP);
        case CursorDirection::DOWN:
            return PrepData(CursorX::LEFT, CursorY::BOTTOM);
        case CursorDirection::LEFT:
            return PrepData(CursorX::LEFT, CursorY::TOP);
        case CursorDirection::RIGHT:
            return PrepData(CursorX::RIGHT, CursorY::TOP);
        case CursorDirection::NEXTLINE:
            return PrepData(CursorX::LEFT, CursorY::BOTTOM);
        case CursorDirection::PREVLINE:
            return PrepData(CursorX::LEFT, CursorY::TOP);
        }
    }

    void PrepData(CursorX xact, CursorY yact)
    {
        PrepData(xact, yact, s_wchDefault, s_wDefaultAttribute);
    }

    void PrepData(CursorX xact, CursorY yact, WCHAR wch, WORD wAttr)
    {
        Log::Comment(L"Resetting mock data state.");

        // APIs succeed by default
        _fSetConsoleCursorPositionResult = TRUE;
        _fGetConsoleScreenBufferInfoExResult = TRUE;
        _fGetConsoleCursorInfoResult = TRUE;
        _fSetConsoleCursorInfoResult = TRUE;
        _fFillConsoleOutputCharacterWResult = TRUE;
        _fFillConsoleOutputAttributeResult = TRUE;
        _fSetConsoleTextAttributeResult = TRUE;
        _fWriteConsoleInputWResult = TRUE;
        _fPrivatePrependConsoleInputResult = TRUE;
        _fPrivateWriteConsoleControlInputResult = TRUE;
        _fScrollConsoleScreenBufferWResult = TRUE;
        _fSetConsoleWindowInfoResult = TRUE;
        _fPrivateGetConsoleScreenBufferAttributesResult = TRUE;

        _PrepCharsBuffer(wch, wAttr);

        // Viewport sitting in the "middle" of the buffer somewhere (so all sides have excess buffer around them)
        _srViewport.Top = 20;
        _srViewport.Bottom = 49;
        _srViewport.Left = 30;
        _srViewport.Right = 59;

        // Call cursor positions seperately
        PrepCursor(xact, yact);

        _dwCursorSize = 33;
        _dwExpectedCursorSize = _dwCursorSize;

        _fCursorVisible = TRUE;
        _fExpectedCursorVisible = _fCursorVisible;

        // Attribute default is gray on black.
        _wAttribute = FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED;
        _wExpectedAttribute = _wAttribute;
    }

    void PrepCursor(CursorX xact, CursorY yact)
    {
        Log::Comment(L"Adjusting cursor within viewport... Expected will match actual when done.");

        switch (xact)
        {
        case CursorX::LEFT:
            Log::Comment(L"Cursor set to left edge of viewport.");
            _coordCursorPos.X = _srViewport.Left;
            break;
        case CursorX::RIGHT:
            Log::Comment(L"Cursor set to right edge of viewport.");
            _coordCursorPos.X = _srViewport.Right - 1;
            break;
        case CursorX::XCENTER:
            Log::Comment(L"Cursor set to centered X of viewport.");
            _coordCursorPos.X = _srViewport.Left + ((_srViewport.Right - _srViewport.Left) / 2);
            break;
        }

        switch (yact)
        {
        case CursorY::TOP:
            Log::Comment(L"Cursor set to top edge of viewport.");
            _coordCursorPos.Y = _srViewport.Top;
            break;
        case CursorY::BOTTOM:
            Log::Comment(L"Cursor set to bottom edge of viewport.");
            _coordCursorPos.Y = _srViewport.Bottom - 1;
            break;
        case CursorY::YCENTER:
            Log::Comment(L"Cursor set to centered Y of viewport.");
            _coordCursorPos.Y = _srViewport.Top + ((_srViewport.Bottom - _srViewport.Top) / 2);
            break;
        }

        _coordExpectedCursorPos = _coordCursorPos;
    }

    void _PrepCharsBuffer()
    {
        _PrepCharsBuffer(s_wchDefault, s_wDefaultAttribute);
    }

    void _PrepCharsBuffer(WCHAR const wch, WORD const wAttr)
    {
        // Buffer large
        _coordBufferSize.X = 100;
        _coordBufferSize.Y = 600;

        // Buffer data
        _FreeCharsBuffer();

        DWORD const cchTotalBufferSize = _coordBufferSize.Y * _coordBufferSize.X;

        _rgchars = new CHAR_INFO[cchTotalBufferSize];

        COORD coordStart = { 0 };
        DWORD dwCharsWritten = 0;

        // Fill buffer with Zs.
        Log::Comment(L"Filling buffer with characters so we can tell what's deleted.");
        FillConsoleOutputCharacterW(wch, cchTotalBufferSize, coordStart, &dwCharsWritten);

        // Fill attributes with 0s
        Log::Comment(L"Filling buffer with attributes so we can tell what happened.");
        FillConsoleOutputAttribute(wAttr, cchTotalBufferSize, coordStart, &dwCharsWritten);

        VERIFY_ARE_EQUAL(((DWORD)cchTotalBufferSize), dwCharsWritten, L"Ensure the writer says all characters in the buffer were filled.");
    }

    void _FreeCharsBuffer()
    {
        if (_rgchars != nullptr)
        {
            delete[] _rgchars;
            _rgchars = nullptr;
        }
    }

    void InsertString(COORD coordTarget, PWSTR pwszText, WORD wAttr)
    {
        Log::Comment(NoThrowString().Format(L"Writing string '%s' to target (X: %d, Y:%d) with color/attr 0x%x", pwszText, coordTarget.X, coordTarget.Y, wAttr));

        size_t cchModified = 0;

        if (pwszText != nullptr)
        {
            size_t cch;
            if (SUCCEEDED(StringCchLengthW(pwszText, STRSAFE_MAX_LENGTH, &cch)))
            {
                COORD coordInsertPoint = coordTarget;

                for (size_t i = 0; i < cch; i++)
                {
                    CHAR_INFO* const pci = _GetCharAt(coordInsertPoint.Y, coordInsertPoint.X);
                    pci->Char.UnicodeChar = pwszText[i];
                    pci->Attributes = wAttr;

                    _IncrementCoordPos(&coordInsertPoint);
                    cchModified++;
                }
            }
        }

        Log::Comment(NoThrowString().Format(L"Wrote %zu characters into buffer.", cchModified));
    }

    void FillRectangle(SMALL_RECT srRect, wchar_t wch, WORD wAttr)
    {
        Log::Comment(NoThrowString().Format(L"Filling area (L: %d, R: %d, T: %d, B: %d) with '%c' in attr 0x%x", srRect.Left, srRect.Right, srRect.Top, srRect.Bottom, wch, wAttr));

        size_t cchModified = 0;

        for (SHORT iRow = srRect.Top; iRow < srRect.Bottom; iRow++)
        {
            for (SHORT iCol = srRect.Left; iCol < srRect.Right; iCol++)
            {
                CHAR_INFO* const pci = _GetCharAt(iRow, iCol);
                pci->Char.UnicodeChar = wch;
                pci->Attributes = wAttr;

                cchModified++;
            }
        }

        Log::Comment(NoThrowString().Format(L"Filled %zu characters.", cchModified));
    }

    void ValidateInputEvent(_In_ PCWSTR pwszExpectedResponse)
    {
        size_t const cchResponse = wcslen(pwszExpectedResponse);
        size_t const eventCount = _events.size();

        VERIFY_ARE_EQUAL(cchResponse * 2, eventCount, L"We should receive TWO input records for every character in the expected string. Key down and key up.");

        for (size_t iInput = 0; iInput < eventCount; iInput++)
        {
            wchar_t const wch = pwszExpectedResponse[iInput / 2]; // the same portion of the string will be used twice. 0/2 = 0. 1/2 = 0. 2/2 = 1. 3/2 = 1. and so on.


            VERIFY_ARE_EQUAL(InputEventType::KeyEvent, _events[iInput]->EventType());

            const KeyEvent* const keyEvent = static_cast<const KeyEvent* const>(_events[iInput].get());

            // every even key is down. every odd key is up. DOWN = 0, UP = 1. DOWN = 2, UP = 3. and so on.
            VERIFY_ARE_EQUAL((bool)!(iInput % 2), keyEvent->IsKeyDown());
            VERIFY_ARE_EQUAL(0u, keyEvent->GetActiveModifierKeys());
            Log::Comment(NoThrowString().Format(L"Comparing '%c' with '%c'...", wch, keyEvent->GetCharData()));
            VERIFY_ARE_EQUAL(wch, keyEvent->GetCharData());
            VERIFY_ARE_EQUAL(1u, keyEvent->GetRepeatCount());
            VERIFY_ARE_EQUAL(0u, keyEvent->GetVirtualKeyCode());
            VERIFY_ARE_EQUAL(0u, keyEvent->GetVirtualScanCode());
        }
    }

    bool ValidateString(COORD const coordTarget, PCWSTR pwszText, WORD const wAttr)
    {
        Log::Comment(NoThrowString().Format(L"Validating that the string %s is written starting at (X: %d, Y: %d) with the color/attr 0x%x", pwszText, coordTarget.X, coordTarget.Y, wAttr));

        bool fSuccess = true;

        if (pwszText != nullptr)
        {
            size_t cch;
            fSuccess = SUCCEEDED(StringCchLengthW(pwszText, STRSAFE_MAX_LENGTH, &cch));

            if (fSuccess)
            {
                COORD coordGetPos = coordTarget;

                for (size_t i = 0; i < cch; i++)
                {
                    const CHAR_INFO* const pci = _GetCharAt(coordGetPos.Y, coordGetPos.X);

                    const wchar_t wchActual = pci->Char.UnicodeChar;
                    const wchar_t wchExpected = pwszText[i];

                    fSuccess = wchExpected == wchActual;

                    if (!fSuccess)
                    {
                        Log::Comment(NoThrowString().Format(L"ValidateString failed char comparison at (X: %d, Y: %d). Expected: '%c' Actual: '%c'", coordGetPos.X, coordGetPos.Y, wchExpected, wchActual));
                        break;
                    }

                    const WORD wAttrActual = pci->Attributes;
                    const WORD wAttrExpected = wAttr;

                    if (!fSuccess)
                    {
                        Log::Comment(NoThrowString().Format(L"ValidateString failed attr comparison at (X: %d, Y: %d). Expected: '0x%x' Actual: '0x%x'", coordGetPos.X, coordGetPos.Y, wAttrExpected, wAttrActual));
                        break;
                    }

                    _IncrementCoordPos(&coordGetPos);
                }
            }
        }

        return fSuccess;
    }

    bool ValidateRectangleContains(SMALL_RECT srRect, wchar_t wchExpected, WORD wAttrExpected)
    {
        Log::Comment(NoThrowString().Format(L"Validating that the area inside (L: %d, R: %d, T: %d, B: %d) char '%c' and attr 0x%x", srRect.Left, srRect.Right, srRect.Top, srRect.Bottom, wchExpected, wAttrExpected));

        bool fStateValid = true;

        for (SHORT iRow = srRect.Top; iRow < srRect.Bottom; iRow++)
        {
            Log::Comment(NoThrowString().Format(L"Validating row(y=) %d", iRow));
            for (SHORT iCol = srRect.Left; iCol < srRect.Right; iCol++)
            {
                CHAR_INFO* const pci = _GetCharAt(iRow, iCol);

                fStateValid = pci->Char.UnicodeChar == wchExpected;
                if (!fStateValid)
                {
                    Log::Comment(NoThrowString().Format(L"Region match failed at (X: %d, Y: %d). Expected: '%c'. Actual: '%c'", iCol, iRow, wchExpected, pci->Char.UnicodeChar));
                    break;
                }

                fStateValid = pci->Attributes == wAttrExpected;
                if (!fStateValid)
                {
                    Log::Comment(NoThrowString().Format(L"Region match failed at (X: %d, Y: %d). Expected Attr: 0x%x. Actual Attr: 0x%x", iCol, iRow, wAttrExpected, pci->Attributes));
                }
            }

            if (!fStateValid)
            {
                break;
            }
        }

        return fStateValid;
    }

    bool ValidateRectangleContains(SMALL_RECT srRect, wchar_t wchExpected, WORD wAttrExpected, SMALL_RECT srExcept)
    {
        bool fStateValid = true;

        Log::Comment(NoThrowString().Format(L"Validating that the area inside (L: %d, R: %d, T: %d, B: %d) but outside (L: %d, R: %d, T: %d, B: %d) contains char '%c' and attr 0x%x", srRect.Left, srRect.Right, srRect.Top, srRect.Bottom, srExcept.Left, srExcept.Right, srExcept.Top, srExcept.Bottom, wchExpected, wAttrExpected));

        for (SHORT iRow = srRect.Top; iRow < srRect.Bottom; iRow++)
        {
            for (SHORT iCol = srRect.Left; iCol < srRect.Right; iCol++)
            {
                if (iRow >= srExcept.Top && iRow < srExcept.Bottom && iCol >= srExcept.Left && iCol < srExcept.Right)
                {
                    // if in exception range, skip comparison.
                    continue;
                }
                else
                {
                    CHAR_INFO* const pci = _GetCharAt(iRow, iCol);

                    fStateValid = pci->Char.UnicodeChar == wchExpected;
                    if (!fStateValid)
                    {
                        Log::Comment(NoThrowString().Format(L"Region match failed at (X: %d, Y: %d). Expected: '%c'. Actual: '%c'", iCol, iRow, wchExpected, pci->Char.UnicodeChar));
                        break;
                    }

                    fStateValid = pci->Attributes == wAttrExpected;
                    if (!fStateValid)
                    {
                        Log::Comment(NoThrowString().Format(L"Region match failed at (X: %d, Y: %d). Expected Attr: 0x%x. Actual Attr: 0x%x", iCol, iRow, wAttrExpected, pci->Attributes));
                    }
                }
            }

            if (!fStateValid)
            {
                break;
            }
        }

        return fStateValid;
    }

    bool ValidateEraseBufferState(SMALL_RECT* rgsrRegions, size_t cRegions, wchar_t wchExpectedInRegions, WORD wAttrExpectedInRegions)
    {
        bool fStateValid = true;

        Log::Comment(NoThrowString().Format(L"The following %zu regions are used as in-bounds for this test:", cRegions));
        for (size_t iRegion = 0; iRegion < cRegions; iRegion++)
        {
            SMALL_RECT srRegion = rgsrRegions[iRegion];

            Log::Comment(NoThrowString().Format(L"#%zu - (T: %d, B: %d, L: %d, R:%d)", iRegion, srRegion.Top, srRegion.Bottom, srRegion.Left, srRegion.Right));
        }

        Log::Comment(L"Now checking every character within the buffer...");
        for (short iRow = 0; iRow < _coordBufferSize.Y; iRow++)
        {
            for (short iCol = 0; iCol < _coordBufferSize.X; iCol++)
            {
                CHAR_INFO* pchar = _GetCharAt(iRow, iCol);

                bool const fIsInclusive = _IsAnyRegionInclusive(rgsrRegions, cRegions, iRow, iCol);

                WCHAR const wchExpected = fIsInclusive ? wchExpectedInRegions : TestGetSet::s_wchDefault;

                WORD const wAttrExpected = fIsInclusive ? wAttrExpectedInRegions : TestGetSet::s_wDefaultAttribute;

                if (pchar->Char.UnicodeChar != wchExpected)
                {
                    fStateValid = false;

                    Log::Comment(NoThrowString().Format(L"Region match failed at (X: %d, Y: %d). Expected: '%c'. Actual: '%c'", iCol, iRow, wchExpected, pchar->Char.UnicodeChar));

                    break;
                }

                if (pchar->Attributes != wAttrExpected)
                {
                    fStateValid = false;

                    Log::Comment(NoThrowString().Format(L"Region match failed at (X: %d, Y: %d). Expected Attr: 0x%x. Actual Attr: 0x%x", iCol, iRow, wAttrExpected, pchar->Attributes));

                    break;
                }
            }

            if (!fStateValid)
            {
                break;
            }
        }

        return fStateValid;
    }

    bool _IsAnyRegionInclusive(SMALL_RECT* rgsrRegions, size_t cRegions, short sRow, short sCol)
    {
        bool fIncludesChar = false;

        for (size_t iRegion = 0; iRegion < cRegions; iRegion++)
        {
            fIncludesChar = _IsInRegionInclusive(rgsrRegions[iRegion], sRow, sCol);

            if (fIncludesChar)
            {
                break;
            }
        }

        return fIncludesChar;
    }

    bool _IsInRegionInclusive(SMALL_RECT srRegion, short sRow, short sCol)
    {
        return srRegion.Left <= sCol &&
            srRegion.Right >= sCol &&
            srRegion.Top <= sRow &&
            srRegion.Bottom >= sRow;

    }

    CHAR_INFO* _GetCharAt(size_t const iRow, size_t const iCol)
    {
        CHAR_INFO* pchar = nullptr;

        if (_rgchars != nullptr)
        {
            pchar = &(_rgchars[(iRow * _coordBufferSize.X) + iCol]);
        }

        if (pchar == nullptr)
        {
            VERIFY_FAIL(L"Failed to retrieve character position from buffer.");
        }

        return pchar;
    }

    void _PrepForScroll(ScrollDirection const dir, int const distance)
    {
        _fExpectedWindowAbsolute = FALSE;
        _srExpectedConsoleWindow.Top = (SHORT)distance;
        _srExpectedConsoleWindow.Bottom = (SHORT)distance;
        _srExpectedConsoleWindow.Left = 0;
        _srExpectedConsoleWindow.Right = 0;
        if (dir == ScrollDirection::UP)
        {
            _srExpectedConsoleWindow.Top *= -1;
            _srExpectedConsoleWindow.Bottom *= -1;
        }
    }

    void _SetMarginsHelper(SMALL_RECT* rect, SHORT top, SHORT bottom)
    {
        rect->Top = top;
        rect->Bottom = bottom;
        //The rectangle is going to get converted from VT space to conhost space
        _srExpectedScrollRegion.Top = (top > 0)? rect->Top - 1 : rect->Top;
        _srExpectedScrollRegion.Bottom = (bottom > 0)? rect->Bottom - 1 : rect->Bottom;
    }

    ~TestGetSet()
    {
        _FreeCharsBuffer();
    }

    static const WCHAR s_wchErase = (WCHAR)0x20;
    static const WCHAR s_wchDefault = L'Z';
    static const WORD s_wAttrErase = FOREGROUND_BLUE | FOREGROUND_GREEN | BACKGROUND_RED | BACKGROUND_INTENSITY;
    static const WORD s_wDefaultAttribute = 0;

    CHAR_INFO* _rgchars = nullptr;
    std::deque<std::unique_ptr<IInputEvent>> _events;

    COORD _coordBufferSize;
    SMALL_RECT _srViewport;
    SMALL_RECT _srExpectedConsoleWindow;
    COORD _coordCursorPos;
    SMALL_RECT _srExpectedScrollRegion;

    DWORD _dwCursorSize;
    BOOL _fCursorVisible;

    COORD _coordExpectedCursorPos;
    DWORD _dwExpectedCursorSize;
    BOOL _fExpectedCursorVisible;

    WORD _wAttribute;
    WORD _wExpectedAttribute;
    int _iXtermTableEntry;
    int _iExpectedXtermTableEntry;
    COLORREF _rgbColor;
    COLORREF _ExpectedColor;
    bool _fIsForeground;
    bool _fExpectedIsForeground;
    bool _fUsingRgbColor = false;
    bool _fExpectedForeground = false;
    bool _fExpectedBackground = false;
    bool _fExpectedMeta = false;

    BOOL _fGetConsoleScreenBufferInfoExResult;
    BOOL _fSetConsoleCursorPositionResult;
    BOOL _fGetConsoleCursorInfoResult;
    BOOL _fSetConsoleCursorInfoResult;
    BOOL _fFillConsoleOutputCharacterWResult;
    BOOL _fFillConsoleOutputAttributeResult;
    BOOL _fSetConsoleTextAttributeResult;
    BOOL _fWriteConsoleInputWResult;
    BOOL _fPrivatePrependConsoleInputResult;
    BOOL _fPrivateWriteConsoleControlInputResult;
    BOOL _fScrollConsoleScreenBufferWResult;

    BOOL _fSetConsoleWindowInfoResult;
    BOOL _fExpectedWindowAbsolute;
    BOOL _fSetConsoleScreenBufferInfoExResult;

    COORD _coordExpectedScreenBufferSize;
    SMALL_RECT _srExpectedScreenBufferViewport;
    WORD  _wExpectedAttributes;
    BOOL _fPrivateSetCursorKeysModeResult;
    BOOL _fPrivateSetKeypadModeResult;
    bool _fCursorKeysApplicationMode;
    bool _fKeypadApplicationMode;
    BOOL _fPrivateAllowCursorBlinkingResult;
    bool _fEnable; // for cursor blinking
    BOOL _fPrivateSetScrollingRegionResult;
    BOOL _fPrivateReverseLineFeedResult;

    BOOL _fSetConsoleTitleWResult;
    wchar_t* _pwchExpectedWindowTitle;
    unsigned short _sCchExpectedTitleLength;
    BOOL _fPrivateHorizontalTabSetResult;
    BOOL _fPrivateForwardTabResult;
    BOOL _fPrivateBackwardsTabResult;
    SHORT _sExpectedNumTabs;
    BOOL _fPrivateTabClearResult;
    bool _fExpectedClearAll;
    bool _fExpectedMouseEnabled;
    bool _fExpectedAlternateScrollEnabled;
    BOOL _fPrivateEnableVT200MouseModeResult;
    BOOL _fPrivateEnableUTF8ExtendedMouseModeResult;
    BOOL _fPrivateEnableSGRExtendedMouseModeResult;
    BOOL _fPrivateEnableButtonEventMouseModeResult;
    BOOL _fPrivateEnableAnyEventMouseModeResult;
    BOOL _fPrivateEnableAlternateScrollResult;
    BOOL _fSetConsoleXtermTextAttributeResult;
    BOOL _fSetConsoleRGBTextAttributeResult;
    BOOL _fPrivateSetLegacyAttributesResult;
    BOOL _fPrivateGetConsoleScreenBufferAttributesResult;

private:
    HANDLE _hCon;
};


class AdapterTest : public AdaptDefaults
{
public:

    TEST_CLASS(AdapterTest);

    TEST_METHOD_SETUP(SetupMethods)
    {
        bool fSuccess = true;

        _pTest = new TestGetSet();

        if (_pTest == nullptr)
        {
            fSuccess = false;
        }
        if (fSuccess)
        {
            _pDispatch = new AdaptDispatch(_pTest, this, s_wDefaultFill);
        }
        if (_pDispatch == nullptr)
        {
            fSuccess = false;
        }
        return fSuccess;
    }

    TEST_METHOD_CLEANUP(CleanupMethods)
    {
        delete _pTest;
        delete _pDispatch;
        return true;
    }

    void Print(_In_ wchar_t const wch)
    {
        UNREFERENCED_PARAMETER(wch);
    }

    void PrintString(_In_reads_(cch) wchar_t* const rgwch, _In_ size_t const cch)
    {
        UNREFERENCED_PARAMETER(rgwch);
        UNREFERENCED_PARAMETER(cch);
    }

    void Execute(_In_ wchar_t const wch)
    {
        UNREFERENCED_PARAMETER(wch);
    }

    TEST_METHOD(CursorMovementTest)
    {
        BEGIN_TEST_METHOD_PROPERTIES()
            TEST_METHOD_PROPERTY(L"Data:uiDirection", L"{0, 1, 2, 3, 4, 5}") // These values align with the CursorDirection enum class to try all the directions.
        END_TEST_METHOD_PROPERTIES()

        Log::Comment(L"Starting test...");

        // Used to switch between the various function options.
        typedef bool(AdaptDispatch::*CursorMoveFunc)(unsigned int);
        CursorMoveFunc moveFunc = nullptr;

        // Modify variables based on directionality of this test
        CursorDirection direction;
        unsigned int dir;
        VERIFY_SUCCEEDED_RETURN(TestData::TryGetValue(L"uiDirection", dir));
        direction = (CursorDirection)dir;

        switch (direction)
        {
        case CursorDirection::UP:
            Log::Comment(L"Testing up direction.");
            moveFunc = &AdaptDispatch::CursorUp;
            break;
        case CursorDirection::DOWN:
            Log::Comment(L"Testing down direction.");
            moveFunc = &AdaptDispatch::CursorDown;
            break;
        case CursorDirection::RIGHT:
            Log::Comment(L"Testing right direction.");
            moveFunc = &AdaptDispatch::CursorForward;
            break;
        case CursorDirection::LEFT:
            Log::Comment(L"Testing left direction.");
            moveFunc = &AdaptDispatch::CursorBackward;
            break;
        case CursorDirection::NEXTLINE:
            Log::Comment(L"Testing next line direction.");
            moveFunc = &AdaptDispatch::CursorNextLine;
            break;
        case CursorDirection::PREVLINE:
            Log::Comment(L"Testing prev line direction.");
            moveFunc = &AdaptDispatch::CursorPrevLine;
            break;
        }

        if (moveFunc == nullptr)
        {
            VERIFY_FAIL();
            return;
        }

        // success cases
        // place cursor in top left. moving up is expected to go nowhere (it should get bounded by the viewport)
        Log::Comment(L"Test 1: Cursor doesn't move when placed in corner of viewport.");
        _pTest->PrepData(direction);
        VERIFY_IS_TRUE((_pDispatch->*(moveFunc))(1));

        Log::Comment(L"Test 1b: Cursor moves to left of line with next/prev line command when cursor can't move higher/lower.");

        bool fDoTest1b = false;

        switch (direction)
        {
        case CursorDirection::NEXTLINE:
            _pTest->PrepData(CursorX::RIGHT, CursorY::BOTTOM);
            fDoTest1b = true;
            break;
        case CursorDirection::PREVLINE:
            _pTest->PrepData(CursorX::RIGHT, CursorY::TOP);
            fDoTest1b = true;
            break;
        }

        if (fDoTest1b)
        {
            _pTest->_coordExpectedCursorPos.X = _pTest->_srViewport.Left;
            VERIFY_IS_TRUE((_pDispatch->*(moveFunc))(1));
        }
        else
        {
            Log::Comment(L"Test not applicable to direction selected. Skipping.");
        }

        // place cursor lower, move up 1.
        Log::Comment(L"Test 2: Cursor moves 1 in the correct direction from viewport.");
        _pTest->PrepData(CursorX::XCENTER, CursorY::YCENTER);

        switch (direction)
        {
        case CursorDirection::UP:
            _pTest->_coordExpectedCursorPos.Y--;
            break;
        case CursorDirection::DOWN:
            _pTest->_coordExpectedCursorPos.Y++;
            break;
        case CursorDirection::RIGHT:
            _pTest->_coordExpectedCursorPos.X++;
            break;
        case CursorDirection::LEFT:
            _pTest->_coordExpectedCursorPos.X--;
            break;
        case CursorDirection::NEXTLINE:
            _pTest->_coordExpectedCursorPos.Y++;
            _pTest->_coordExpectedCursorPos.X = _pTest->_srViewport.Left;
            break;
        case CursorDirection::PREVLINE:
            _pTest->_coordExpectedCursorPos.Y--;
            _pTest->_coordExpectedCursorPos.X = _pTest->_srViewport.Left;
            break;
        }

        VERIFY_IS_TRUE((_pDispatch->*(moveFunc))(1));

        // place cursor and move it up too far. It should get bounded by the viewport.
        Log::Comment(L"Test 3: Cursor moves and gets stuck at viewport when started away from edges and moved beyond edges.");
        _pTest->PrepData(CursorX::XCENTER, CursorY::YCENTER);

        // Bottom and right viewports are -1 because those two sides are specified to be 1 outside the viewable area.

        switch (direction)
        {
        case CursorDirection::UP:
            _pTest->_coordExpectedCursorPos.Y = _pTest->_srViewport.Top;
            break;
        case CursorDirection::DOWN:
            _pTest->_coordExpectedCursorPos.Y = _pTest->_srViewport.Bottom - 1;
            break;
        case CursorDirection::RIGHT:
            _pTest->_coordExpectedCursorPos.X = _pTest->_srViewport.Right - 1;
            break;
        case CursorDirection::LEFT:
            _pTest->_coordExpectedCursorPos.X = _pTest->_srViewport.Left;
            break;
        case CursorDirection::NEXTLINE:
            _pTest->_coordExpectedCursorPos.X = _pTest->_srViewport.Left;
            _pTest->_coordExpectedCursorPos.Y = _pTest->_srViewport.Bottom - 1;
            break;
        case CursorDirection::PREVLINE:
            _pTest->_coordExpectedCursorPos.X = _pTest->_srViewport.Left;
            _pTest->_coordExpectedCursorPos.Y = _pTest->_srViewport.Top;
            break;
        }

        VERIFY_IS_TRUE((_pDispatch->*(moveFunc))(100));

        // error cases
        // give too large an up distance, cursor move should fail, cursor should stay the same.
        Log::Comment(L"Test 4: When given invalid (massive) move distance that doesn't fit in a short, call fails and cursor doesn't move.");
        _pTest->PrepData(CursorX::XCENTER, CursorY::YCENTER);

        VERIFY_IS_FALSE((_pDispatch->*(moveFunc))(UINT_MAX));
        VERIFY_ARE_EQUAL(_pTest->_coordExpectedCursorPos, _pTest->_coordCursorPos);

        // cause short underflow. cursor move should fail. cursor should stay the same.
        Log::Comment(L"Test 5: When an over/underflow occurs in cursor math, call fails and cursor doesn't move.");
        _pTest->PrepData(direction);

        switch (direction)
        {
        case CursorDirection::UP:
        case CursorDirection::PREVLINE:
            _pTest->_coordCursorPos.Y = -10;
            break;
        case CursorDirection::DOWN:
        case CursorDirection::NEXTLINE:
            _pTest->_coordCursorPos.Y = 10;
            break;
        case CursorDirection::RIGHT:
            _pTest->_coordCursorPos.X = 10;
            break;
        case CursorDirection::LEFT:
            _pTest->_coordCursorPos.X = -10;
            break;
        }

        _pTest->_coordExpectedCursorPos = _pTest->_coordCursorPos;

        VERIFY_IS_FALSE((_pDispatch->*(moveFunc))(SHRT_MAX));
        VERIFY_ARE_EQUAL(_pTest->_coordExpectedCursorPos, _pTest->_coordCursorPos);

        // SetConsoleCursorPosition throws failure. Parameters are otherwise normal.
        Log::Comment(L"Test 6: When SetConsoleCursorPosition throws a failure, call fails and cursor doesn't move.");
        _pTest->PrepData(direction);
        _pTest->_fSetConsoleCursorPositionResult = FALSE;

        VERIFY_IS_FALSE((_pDispatch->*(moveFunc))(0));
        VERIFY_ARE_EQUAL(_pTest->_coordExpectedCursorPos, _pTest->_coordCursorPos);

        // GetConsoleScreenBufferInfo throws failure. Parameters are otherwise normal.
        Log::Comment(L"Test 7: When GetConsoleScreenBufferInfo throws a failure, call fails and cursor doesn't move.");
        _pTest->PrepData(CursorX::LEFT, CursorY::TOP);
        _pTest->_fGetConsoleScreenBufferInfoExResult = FALSE;

        VERIFY_IS_FALSE((_pDispatch->*(moveFunc))(0));
        VERIFY_ARE_EQUAL(_pTest->_coordExpectedCursorPos, _pTest->_coordCursorPos);
    }

    TEST_METHOD(CursorPositionTest)
    {
        Log::Comment(L"Starting test...");


        Log::Comment(L"Test 1: Place cursor within the viewport. Start from top left, move to middle.");
        _pTest->PrepData(CursorX::LEFT, CursorY::TOP);

        short sCol = (_pTest->_srViewport.Right - _pTest->_srViewport.Left) / 2;
        short sRow = (_pTest->_srViewport.Bottom - _pTest->_srViewport.Top) / 2;

        _pTest->_coordExpectedCursorPos.X = _pTest->_srViewport.Left + (sCol - 1);
        _pTest->_coordExpectedCursorPos.Y = _pTest->_srViewport.Top + (sRow - 1);

        VERIFY_IS_TRUE(_pDispatch->CursorPosition(sRow, sCol));

        Log::Comment(L"Test 2: Move to 0, 0 (which is 1,1 in VT speak)");
        _pTest->PrepData(CursorX::RIGHT, CursorY::BOTTOM);

        _pTest->_coordExpectedCursorPos.X = _pTest->_srViewport.Left;
        _pTest->_coordExpectedCursorPos.Y = _pTest->_srViewport.Top;

        VERIFY_IS_TRUE(_pDispatch->CursorPosition(1, 1));

        Log::Comment(L"Test 3: Move beyond rectangle (down/right too far). Should be bounded back in.");
        _pTest->PrepData(CursorX::LEFT, CursorY::TOP);

        sCol = (_pTest->_srViewport.Right - _pTest->_srViewport.Left) * 2;
        sRow = (_pTest->_srViewport.Bottom - _pTest->_srViewport.Top) * 2;

        _pTest->_coordExpectedCursorPos.X = _pTest->_srViewport.Right - 1;
        _pTest->_coordExpectedCursorPos.Y = _pTest->_srViewport.Bottom - 1;

        VERIFY_IS_TRUE(_pDispatch->CursorPosition(sRow, sCol));

        Log::Comment(L"Test 4: Values too large for short. Cursor shouldn't move. Return false.");
        _pTest->PrepData(CursorX::LEFT, CursorY::TOP);

        VERIFY_IS_FALSE(_pDispatch->CursorPosition(UINT_MAX, UINT_MAX));

        Log::Comment(L"Test 5: Overflow during addition. Cursor shouldn't move. Return false.");
        _pTest->PrepData(CursorX::LEFT, CursorY::TOP);

        _pTest->_srViewport.Left = SHRT_MAX;
        _pTest->_srViewport.Top = SHRT_MAX;

        VERIFY_IS_FALSE(_pDispatch->CursorPosition(5, 5));

        Log::Comment(L"Test 6: GetConsoleInfo API returns false. No move, return false.");
        _pTest->PrepData(CursorX::LEFT, CursorY::TOP);

        _pTest->_fGetConsoleScreenBufferInfoExResult = FALSE;

        VERIFY_IS_FALSE(_pDispatch->CursorPosition(1, 1));

        Log::Comment(L"Test 7: SetCursor API returns false. No move, return false.");
        _pTest->PrepData(CursorX::LEFT, CursorY::TOP);

        _pTest->_fSetConsoleCursorPositionResult = FALSE;

        VERIFY_IS_FALSE(_pDispatch->CursorPosition(1, 1));

        Log::Comment(L"Test 8: Move to 0,0. Cursor shouldn't move. Return false. 1,1 is the top left corner in VT100 speak. 0,0 isn't a position. The parser will give 1 for a 0 input.");
        _pTest->PrepData(CursorX::LEFT, CursorY::TOP);

        VERIFY_IS_FALSE(_pDispatch->CursorPosition(0, 0));

    }

    TEST_METHOD(CursorSingleDimensionMoveTest)
    {
        BEGIN_TEST_METHOD_PROPERTIES()
            TEST_METHOD_PROPERTY(L"Data:uiDirection", L"{0, 1}") // These values align with the CursorDirection enum class to try all the directions.
        END_TEST_METHOD_PROPERTIES()

        Log::Comment(L"Starting test...");


        //// Used to switch between the various function options.
        typedef bool(AdaptDispatch::*CursorMoveFunc)(unsigned int);
        CursorMoveFunc moveFunc = nullptr;
        SHORT* psViewportEnd = nullptr;
        SHORT* psViewportStart = nullptr;
        SHORT* psCursorExpected = nullptr;

        // Modify variables based on directionality of this test
        AbsolutePosition direction;
        unsigned int dir;
        VERIFY_SUCCEEDED_RETURN(TestData::TryGetValue(L"uiDirection", dir));
        direction = (AbsolutePosition)dir;

        switch (direction)
        {
        case AbsolutePosition::CursorHorizontal:
            Log::Comment(L"Testing cursor horizontal movement.");
            psViewportEnd = &_pTest->_srViewport.Right;
            psViewportStart = &_pTest->_srViewport.Left;
            psCursorExpected = &_pTest->_coordExpectedCursorPos.X;
            moveFunc = &AdaptDispatch::CursorHorizontalPositionAbsolute;
            break;
        case AbsolutePosition::VerticalLine:
            Log::Comment(L"Testing vertical line movement.");
            psViewportEnd = &_pTest->_srViewport.Bottom;
            psViewportStart = &_pTest->_srViewport.Top;
            psCursorExpected = &_pTest->_coordExpectedCursorPos.Y;
            moveFunc = &AdaptDispatch::VerticalLinePositionAbsolute;
            break;
        }

        if (moveFunc == nullptr || psViewportEnd == nullptr || psViewportStart == nullptr || psCursorExpected == nullptr)
        {
            VERIFY_FAIL();
            return;
        }

        Log::Comment(L"Test 1: Place cursor within the viewport. Start from top left, move to middle.");
        _pTest->PrepData(CursorX::LEFT, CursorY::TOP);

        short sVal = (*psViewportEnd - *psViewportStart) / 2;

        *psCursorExpected = *psViewportStart + (sVal - 1);

        VERIFY_IS_TRUE((_pDispatch->*(moveFunc))(sVal));

        Log::Comment(L"Test 2: Move to 0 (which is 1 in VT speak)");
        _pTest->PrepData(CursorX::RIGHT, CursorY::BOTTOM);

        *psCursorExpected = *psViewportStart;
        sVal = 1;

        VERIFY_IS_TRUE((_pDispatch->*(moveFunc))(sVal));

        Log::Comment(L"Test 3: Move beyond rectangle (down/right too far). Should be bounded back in.");
        _pTest->PrepData(CursorX::LEFT, CursorY::TOP);

        sVal = (*psViewportEnd - *psViewportStart) * 2;

        *psCursorExpected = *psViewportEnd - 1;

        VERIFY_IS_TRUE((_pDispatch->*(moveFunc))(sVal));

        Log::Comment(L"Test 4: Values too large for short. Cursor shouldn't move. Return false.");
        _pTest->PrepData(CursorX::LEFT, CursorY::TOP);

        sVal = SHORT_MAX;

        VERIFY_IS_FALSE((_pDispatch->*(moveFunc))(sVal));

        Log::Comment(L"Test 5: Overflow during addition. Cursor shouldn't move. Return false.");
        _pTest->PrepData(CursorX::LEFT, CursorY::TOP);

        _pTest->_srViewport.Left = SHRT_MAX;

        sVal = 5;

        VERIFY_IS_FALSE((_pDispatch->*(moveFunc))(sVal));

        Log::Comment(L"Test 6: GetConsoleInfo API returns false. No move, return false.");
        _pTest->PrepData(CursorX::LEFT, CursorY::TOP);

        _pTest->_fGetConsoleScreenBufferInfoExResult = FALSE;

        sVal = 1;

        VERIFY_IS_FALSE((_pDispatch->*(moveFunc))(sVal));

        Log::Comment(L"Test 7: SetCursor API returns false. No move, return false.");
        _pTest->PrepData(CursorX::LEFT, CursorY::TOP);

        _pTest->_fSetConsoleCursorPositionResult = FALSE;

        sVal = 1;

        VERIFY_IS_FALSE((_pDispatch->*(moveFunc))(sVal));

        Log::Comment(L"Test 8: Move to 0. Cursor shouldn't move. Return false. 1 is the left edge in VT100 speak. 0 isn't a position. The parser will give 1 for a 0 input.");
        _pTest->PrepData(CursorX::LEFT, CursorY::TOP);

        sVal = 0;

        VERIFY_IS_FALSE((_pDispatch->*(moveFunc))(sVal));
    }

    TEST_METHOD(CursorSaveRestoreTest)
    {
        Log::Comment(L"Starting test...");


        COORD coordExpected = { 0 };

        Log::Comment(L"Test 1: Restore with no saved data should move to top-left corner, the null/default position.");

        // Move cursor to top left and save off expected position.
        _pTest->PrepData(CursorX::LEFT, CursorY::TOP);
        coordExpected = _pTest->_coordExpectedCursorPos;

        // Then move cursor to the middle and reset the expected to the top left.
        _pTest->PrepData(CursorX::XCENTER, CursorY::YCENTER);
        _pTest->_coordExpectedCursorPos = coordExpected;

        VERIFY_IS_TRUE(_pDispatch->CursorRestorePosition(), L"By default, restore to top left corner (0,0 offset from viewport).");

        Log::Comment(L"Test 2: Place cursor in center. Save. Move cursor to corner. Restore. Should come back to center.");
        _pTest->PrepData(CursorX::XCENTER, CursorY::YCENTER);
        VERIFY_IS_TRUE(_pDispatch->CursorSavePosition(), L"Succeed at saving position.");

        Log::Comment(L"Backup expected cursor (in the middle). Move cursor to corner. Then re-set expected cursor to middle.");
        // save expected cursor position
        coordExpected = _pTest->_coordExpectedCursorPos;

        // adjust cursor to corner
        _pTest->PrepData(CursorX::LEFT, CursorY::BOTTOM);

        // restore expected cursor position to center.
        _pTest->_coordExpectedCursorPos = coordExpected;

        VERIFY_IS_TRUE(_pDispatch->CursorRestorePosition(), L"Restoring to corner should succeed. API call inside will test that cursor matched expected position.");
    }

    TEST_METHOD(CursorHideShowTest)
    {
        BEGIN_TEST_METHOD_PROPERTIES()
            TEST_METHOD_PROPERTY(L"Data:fStartingVis", L"{TRUE, FALSE}")
            TEST_METHOD_PROPERTY(L"Data:fEndingVis", L"{TRUE, FALSE}")
        END_TEST_METHOD_PROPERTIES()

        Log::Comment(L"Starting test...");


        // Modify variables based on permutations of this test.
        bool fStart;
        bool fEnd;
        VERIFY_SUCCEEDED_RETURN(TestData::TryGetValue(L"fStartingVis", fStart));
        VERIFY_SUCCEEDED_RETURN(TestData::TryGetValue(L"fEndingVis", fEnd));

        Log::Comment(L"Test 1: Verify successful API call modifies visibility state.");
        _pTest->PrepData();
        _pTest->_fCursorVisible = fStart;
        _pTest->_fExpectedCursorVisible = fEnd;
        VERIFY_IS_TRUE(_pDispatch->CursorVisibility(fEnd));

        Log::Comment(L"Test 2: When we fail to get existing cursor information, the dispatch should fail.");
        _pTest->PrepData();
        _pTest->_fGetConsoleCursorInfoResult = false;
        VERIFY_IS_FALSE(_pDispatch->CursorVisibility(fEnd));

        Log::Comment(L"Test 3: When we fail to set updated cursor information, the dispatch should fail.");
        _pTest->PrepData();
        _pTest->_fSetConsoleCursorInfoResult = false;
        VERIFY_IS_FALSE(_pDispatch->CursorVisibility(fEnd));
    }

    TEST_METHOD(InsertCharacterTests)
    {
        Log::Comment(L"Starting test...");

        Log::Comment(L"Test 1: The big one. Fill the buffer with Qs. Fill the window with Rs. Write a line of ABCDE at the cursor. Then insert 5 spaces at the cursor. Watch spaces get inserted, ABCDE slide right eating up the Rs in the viewport but not modifying the Qs outside.");

        // place the cursor in the center.
        _pTest->PrepData(CursorX::XCENTER, CursorY::YCENTER);

        // Save the cursor position. It shouldn't move for the rest of the test.
        COORD coordCursorExpected = _pTest->_coordCursorPos;

        // Fill the entire buffer with Qs. Blue on Green.
        WCHAR const wchOuterBuffer = 'Q';
        WORD const wAttrOuterBuffer = FOREGROUND_BLUE | BACKGROUND_GREEN;
        SMALL_RECT srOuterBuffer;
        srOuterBuffer.Top = 0;
        srOuterBuffer.Left = 0;
        srOuterBuffer.Bottom = _pTest->_coordBufferSize.Y;
        srOuterBuffer.Right= _pTest->_coordBufferSize.X;
        _pTest->FillRectangle(srOuterBuffer, wchOuterBuffer, wAttrOuterBuffer);

        // Fill the viewport with Rs. Red on Blue.
        WCHAR const wchViewport = 'R';
        WORD const wAttrViewport = FOREGROUND_RED | BACKGROUND_BLUE;
        SMALL_RECT srViewport = _pTest->_srViewport;
        _pTest->FillRectangle(srViewport, wchViewport, wAttrViewport);

        // fill some of the text right of the cursor so we can verify it moved it and didn't overwrite it.
        // change the color too so we can make sure that it's fine

        WORD const wAttrTestText = FOREGROUND_GREEN;
        PWSTR const pwszTestText = L"ABCDE";
        size_t cchTestText = wcslen(pwszTestText);
        SMALL_RECT srTestText;
        srTestText.Top = _pTest->_coordCursorPos.Y;
        srTestText.Bottom = srTestText.Top + 1;
        srTestText.Left = _pTest->_coordCursorPos.X;
        srTestText.Right = srTestText.Left + (SHORT)cchTestText;
        _pTest->InsertString(_pTest->_coordCursorPos, pwszTestText, wAttrTestText);

        WCHAR const wchInsertExpected = L' ';
        WORD const wAttrInsertExpected = _pTest->_wAttribute;
        size_t const cchInsertSize = 5;
        SMALL_RECT srInsertExpected;
        srInsertExpected.Top = _pTest->_coordCursorPos.Y;
        srInsertExpected.Bottom = srInsertExpected.Top + 1;
        srInsertExpected.Left = _pTest->_coordCursorPos.X;
        srInsertExpected.Right = srInsertExpected.Left + (SHORT)cchInsertSize;

        // the text we inserted is going to move right by the insert size, so adjust that rectangle right.
        srTestText.Left += cchInsertSize;
        srTestText.Right += cchInsertSize;

        // insert out 5 spots. this should clear them out with spaces and the default fill from the original cursor position
        VERIFY_IS_TRUE(_pDispatch->InsertCharacter(cchInsertSize), L"Verify insert call was sucessful.");

        // the combined area of the letters + the spaces will be 10 characters wide:
        SMALL_RECT srModifiedSpace;
        srModifiedSpace.Top = _pTest->_coordCursorPos.Y;
        srModifiedSpace.Bottom = srModifiedSpace.Top + 1;
        srModifiedSpace.Left = _pTest->_coordCursorPos.X;
        srModifiedSpace.Right = srModifiedSpace.Left +(SHORT)cchInsertSize + (SHORT)cchTestText;

        // verify cursor didn't move
        VERIFY_ARE_EQUAL(coordCursorExpected, _pTest->_coordCursorPos, L"Verify cursor didn't move from insert operation.");

        // e.g. we had this in the buffer: QQQRRRRRRABCDERRRRRRRQQQ with the cursor on the A.
        // now we should have this buffer: QQQRRRRRR     ABCDERRQQQ with the cursor on the first space.

        // Verify the field of Qs didn't change outside the viewport.
        VERIFY_IS_TRUE(_pTest->ValidateRectangleContains(srOuterBuffer, wchOuterBuffer, wAttrOuterBuffer, srViewport), L"Field of Qs outside viewport should remain unchanged.");

        // Verify the field of Rs within the viewport not including the inserted range and the ABCDE shifted right. (10 characters)
        VERIFY_IS_TRUE(_pTest->ValidateRectangleContains(srViewport, wchViewport, wAttrViewport, srModifiedSpace), L"Field of Rs in the viewport outside modified space should remain unchanged.");

        // Verify the 5 spaces inserted from the cursor.
        VERIFY_IS_TRUE(_pTest->ValidateRectangleContains(srInsertExpected, wchInsertExpected, wAttrInsertExpected), L"Spaces should be inserted with the proper attributes at the cursor.");

        // Verify the ABCDE sequence was shifted right.
        COORD coordTestText;
        coordTestText.X = srTestText.Left;
        coordTestText.Y = srTestText.Top;
        VERIFY_IS_TRUE(_pTest->ValidateString(coordTestText, pwszTestText, wAttrTestText), L"Inserted string should have moved to the right by the number of spaces inserted, attributes and text preserved.");

        // Test case needed for exact end of line (and full line) insert/delete lengths
        Log::Comment(L"Test 2: Inserting at the exact end of the line. Same field of Qs and Rs. Move cursor to right edge of window and insert > 1 space. Only 1 should be inserted, everything else unchanged.");

        _pTest->FillRectangle(srOuterBuffer, wchOuterBuffer, wAttrOuterBuffer);
        _pTest->FillRectangle(srViewport, wchViewport, wAttrViewport);

        // move cursor to right edge
        _pTest->_coordCursorPos.X = _pTest->_srViewport.Right - 1;
        coordCursorExpected = _pTest->_coordCursorPos;

        // the rectangle where the space should be is exactly the size of the cursor.
        srModifiedSpace.Top = _pTest->_coordCursorPos.Y;
        srModifiedSpace.Bottom = srModifiedSpace.Top + 1;
        srModifiedSpace.Left = _pTest->_coordCursorPos.X;
        srModifiedSpace.Right = srModifiedSpace.Left + 1;

        // insert out 5 spots. this should clear them out with spaces and the default fill from the original cursor position
        VERIFY_IS_TRUE(_pDispatch->InsertCharacter(cchInsertSize), L"Verify insert call was sucessful.");

        // cursor didn't move
        VERIFY_ARE_EQUAL(coordCursorExpected, _pTest->_coordCursorPos, L"Verify cursor didn't move from insert operation.");

        // Qs are the same outside the viewport
        VERIFY_IS_TRUE(_pTest->ValidateRectangleContains(srOuterBuffer, wchOuterBuffer, wAttrOuterBuffer, srViewport), L"Field of Qs outside viewport should remain unchanged.");

        // Entire viewport is Rs except the one space spot
        VERIFY_IS_TRUE(_pTest->ValidateRectangleContains(srViewport, wchViewport, wAttrViewport, srModifiedSpace), L"Field of Rs in the viewport outside modified space should remain unchanged.");

        // The 5 inserted spaces at the right edge resulted in 1 space at the right edge
        VERIFY_IS_TRUE(_pTest->ValidateRectangleContains(srModifiedSpace, wchInsertExpected, wAttrInsertExpected), L"A space was inserted at the cursor position. All extra spaces were discarded as they hit the right boundary.");

        Log::Comment(L"Test 3: Inserting at the exact beginning of the line. Same field of Qs and Rs. Move cursor to left edge of window and insert > screen width of space. The whole row should be spaces but nothing outside the viewport should be changed.");

        _pTest->FillRectangle(srOuterBuffer, wchOuterBuffer, wAttrOuterBuffer);
        _pTest->FillRectangle(srViewport, wchViewport, wAttrViewport);

        // move cursor to left edge
        _pTest->_coordCursorPos.X = _pTest->_srViewport.Left;
        coordCursorExpected = _pTest->_coordCursorPos;

        // the rectangle of spaces should be the entire line at the cursor.
        srModifiedSpace.Top = _pTest->_coordCursorPos.Y;
        srModifiedSpace.Bottom = srModifiedSpace.Top + 1;
        srModifiedSpace.Left = _pTest->_srViewport.Left;
        srModifiedSpace.Right = _pTest->_srViewport.Right;

        // insert greater than the entire viewport (the entire buffer width) at the cursor position
        VERIFY_IS_TRUE(_pDispatch->InsertCharacter(_pTest->_coordBufferSize.X), L"Verify insert call was successful.");

        // cursor didn't move
        VERIFY_ARE_EQUAL(coordCursorExpected, _pTest->_coordCursorPos, L"Verify cursor didn't move from insert operation.");

        // Qs are the same outside the viewport
        VERIFY_IS_TRUE(_pTest->ValidateRectangleContains(srOuterBuffer, wchOuterBuffer, wAttrOuterBuffer, srViewport), L"Field of Qs outside viewport should remain unchanged.");

        // Entire viewport is Rs except the one space spot
        VERIFY_IS_TRUE(_pTest->ValidateRectangleContains(srViewport, wchViewport, wAttrViewport, srModifiedSpace), L"Field of Rs in the viewport outside modified space should remain unchanged.");

        // The inserted spaces at the left edge resulted in an entire line of spaces bounded by the viewport
        VERIFY_IS_TRUE(_pTest->ValidateRectangleContains(srModifiedSpace, wchInsertExpected, wAttrInsertExpected), L"A whole line of spaces was inserted at the cursor position. All extra spaces were discarded as they hit the right boundary.");
    }

    TEST_METHOD(DeleteCharacterTests)
    {
        Log::Comment(L"Starting test...");

        Log::Comment(L"Test 1: The big one. Fill the buffer with Qs. Fill the window with Rs. Write a line of ABCDE at the cursor. Then insert 5 spaces at the cursor. Watch spaces get inserted, ABCDE slide right eating up the Rs in the viewport but not modifying the Qs outside.");

        // place the cursor in the center.
        _pTest->PrepData(CursorX::XCENTER, CursorY::YCENTER);

        // Save the cursor position. It shouldn't move for the rest of the test.
        COORD coordCursorExpected = _pTest->_coordCursorPos;

        // Fill the entire buffer with Qs. Blue on Green.
        WCHAR const wchOuterBuffer = 'Q';
        WORD const wAttrOuterBuffer = FOREGROUND_BLUE | BACKGROUND_GREEN;
        SMALL_RECT srOuterBuffer;
        srOuterBuffer.Top = 0;
        srOuterBuffer.Left = 0;
        srOuterBuffer.Bottom = _pTest->_coordBufferSize.Y;
        srOuterBuffer.Right = _pTest->_coordBufferSize.X;
        _pTest->FillRectangle(srOuterBuffer, wchOuterBuffer, wAttrOuterBuffer);

        // Fill the viewport with Rs. Red on Blue.
        WCHAR const wchViewport = 'R';
        WORD const wAttrViewport = FOREGROUND_RED | BACKGROUND_BLUE;
        SMALL_RECT srViewport = _pTest->_srViewport;
        _pTest->FillRectangle(srViewport, wchViewport, wAttrViewport);

        // fill some of the text right of the cursor so we can verify it moved it and wasn't deleted
        // change the color too so we can make sure that it's fine
        WORD const wAttrTestText = FOREGROUND_GREEN;
        PWSTR const pwszTestText = L"ABCDE";
        size_t cchTestText = wcslen(pwszTestText);
        SMALL_RECT srTestText;
        srTestText.Top = _pTest->_coordCursorPos.Y;
        srTestText.Bottom = srTestText.Top + 1;
        srTestText.Left = _pTest->_coordCursorPos.X;
        srTestText.Right = srTestText.Left + (SHORT)cchTestText;
        _pTest->InsertString(_pTest->_coordCursorPos, pwszTestText, wAttrTestText);

        // We're going to delete "in" from the right edge, so set up that rectangle.
        WCHAR const wchDeleteExpected = L' ';
        WORD const wAttrDeleteExpected = _pTest->_wAttribute;
        size_t const cchDeleteSize = 5;
        SMALL_RECT srDeleteExpected;
        srDeleteExpected.Top = _pTest->_coordCursorPos.Y;
        srDeleteExpected.Bottom = srDeleteExpected.Top + 1;
        srDeleteExpected.Right = _pTest->_srViewport.Right;
        srDeleteExpected.Left = srDeleteExpected.Right - cchDeleteSize;

        // We want the ABCDE to shift left when we delete and onto the cursor. So move the cursor left 5 and adjust the srTestText rectangle left 5 to the new
        // final destination of where they will be after the delete operation occurs.
        _pTest->_coordCursorPos.X -= cchDeleteSize;
        coordCursorExpected = _pTest->_coordCursorPos;
        srTestText.Left -= cchDeleteSize;
        srTestText.Right -= cchDeleteSize;


        // delete out 5 spots. this should shift the ABCDE text left by 5 and insert 5 spaces at the end of the line
        VERIFY_IS_TRUE(_pDispatch->DeleteCharacter(cchDeleteSize), L"Verify delete call was sucessful.");

        // we're going to have ABCDERRRRRRRRRRRRR      QQQQQQQ
        // since this is a bit more complicated than the insert case, make this the "special" region and exempt it from the bulk "R" check
        // we'll check the inside of this rect in 3 pieces, for the ABCDE, then for the inner Rs, then for the 5 spaces after.
        SMALL_RECT srSpecialSpace;
        srSpecialSpace.Top = _pTest->_coordCursorPos.Y;
        srSpecialSpace.Bottom = srSpecialSpace.Top + 1;
        srSpecialSpace.Left = _pTest->_coordCursorPos.X;
        srSpecialSpace.Right = _pTest->_srViewport.Right;

        SMALL_RECT srGap; // gap space is the Rs between ABCDE and the spaces shifted in from the right
        srGap.Left = srTestText.Right;
        srGap.Right = srDeleteExpected.Left;
        srGap.Top = _pTest->_coordCursorPos.Y;
        srGap.Bottom = srGap.Top + 1;

        // verify cursor didn't move
        VERIFY_ARE_EQUAL(coordCursorExpected, _pTest->_coordCursorPos, L"Verify cursor didn't move from insert operation.");

        // e.g. we had this in the buffer: QQQRRRRRR-RRRRABCDERRQQQ with the cursor on the -.
        // now we should have this buffer: QQQRRRRRRABCDERR     QQQ with the cursor on the A.

        // Verify the field of Qs didn't change outside the viewport.
        VERIFY_IS_TRUE(_pTest->ValidateRectangleContains(srOuterBuffer, wchOuterBuffer, wAttrOuterBuffer, srViewport), L"Field of Qs outside viewport should remain unchanged.");

        // Verify the field of Rs within the viewport not including the special range of the ABCDE, the spaces shifted in from the right, and the Rs between them that went along for the ride.
        VERIFY_IS_TRUE(_pTest->ValidateRectangleContains(srViewport, wchViewport, wAttrViewport, srSpecialSpace), L"Field of Rs in the viewport outside modified space should remain unchanged.");

        // Verify the 5 spaces shifted in from the right edge due to the delete
        VERIFY_IS_TRUE(_pTest->ValidateRectangleContains(srDeleteExpected, wchDeleteExpected, wAttrDeleteExpected), L"Spaces should be inserted with the proper attributes from the right end of this line (viewport edge.)");

        // Verify the ABCDE sequence was shifted left by 5 toward the cursor.
        COORD coordTestText;
        coordTestText.X = srTestText.Left;
        coordTestText.Y = srTestText.Top;
        VERIFY_IS_TRUE(_pTest->ValidateString(coordTestText, pwszTestText, wAttrTestText), L"Inserted string should have moved to the left by the number of deletes, attributes and text preserved.");

        // Verify the field of Rs between the ABCDE and the spaces shifted in from the right
        VERIFY_IS_TRUE(_pTest->ValidateRectangleContains(srGap, wchViewport, wAttrViewport), L"Viewport Rs should be preserved/shifted left in between the ABCDE and the spaces that came in from the right edge.");

        // Test case needed for exact end of line (and full line) insert/delete lengths
        Log::Comment(L"Test 2: Deleting at the exact end of the line. Same field of Qs and Rs. Move cursor to right edge of window and delete > 1 space. Only 1 should be inserted from the right edge (delete inserts from the right), everything else unchanged.");

        _pTest->FillRectangle(srOuterBuffer, wchOuterBuffer, wAttrOuterBuffer);
        _pTest->FillRectangle(srViewport, wchViewport, wAttrViewport);

        // move cursor to right edge
        _pTest->_coordCursorPos.X = _pTest->_srViewport.Right - 1;
        coordCursorExpected = _pTest->_coordCursorPos;

        // the rectangle where the space should be is exactly the size of the cursor.
        SMALL_RECT srModifiedSpace;
        srModifiedSpace.Top = _pTest->_coordCursorPos.Y;
        srModifiedSpace.Bottom = srModifiedSpace.Top + 1;
        srModifiedSpace.Left = _pTest->_coordCursorPos.X;
        srModifiedSpace.Right = srModifiedSpace.Left + 1;

        // delete out 5 spots. this should clear them out with spaces and the default fill from the original cursor position
        VERIFY_IS_TRUE(_pDispatch->DeleteCharacter(cchDeleteSize), L"Verify delete call was sucessful.");

        // cursor didn't move
        VERIFY_ARE_EQUAL(coordCursorExpected, _pTest->_coordCursorPos, L"Verify cursor didn't move from delete operation.");

        // Qs are the same outside the viewport
        VERIFY_IS_TRUE(_pTest->ValidateRectangleContains(srOuterBuffer, wchOuterBuffer, wAttrOuterBuffer, srViewport), L"Field of Qs outside viewport should remain unchanged.");

        // Entire viewport is Rs except the one space spot
        VERIFY_IS_TRUE(_pTest->ValidateRectangleContains(srViewport, wchViewport, wAttrViewport, srModifiedSpace), L"Field of Rs in the viewport outside modified space should remain unchanged.");

        // The 5 deleted spaces at the right edge resulted in 1 space at the right edge
        VERIFY_IS_TRUE(_pTest->ValidateRectangleContains(srModifiedSpace, wchDeleteExpected, wAttrDeleteExpected), L"A space was inserted at the cursor position. All extra spaces deleted in from the right continued to cover that one space.");

        Log::Comment(L"Test 3: Deleting at the exact beginning of the line. Same field of Qs and Rs. Move cursor to left edge of window and delete > screen width of space. The whole row should be spaces but nothing outside the viewport should be changed.");

        _pTest->FillRectangle(srOuterBuffer, wchOuterBuffer, wAttrOuterBuffer);
        _pTest->FillRectangle(srViewport, wchViewport, wAttrViewport);

        // move cursor to left edge
        _pTest->_coordCursorPos.X = _pTest->_srViewport.Left;
        coordCursorExpected = _pTest->_coordCursorPos;

        // the rectangle of spaces should be the entire line at the cursor.
        srModifiedSpace.Top = _pTest->_coordCursorPos.Y;
        srModifiedSpace.Bottom = srModifiedSpace.Top + 1;
        srModifiedSpace.Left = _pTest->_srViewport.Left;
        srModifiedSpace.Right = _pTest->_srViewport.Right;

        // delete greater than the entire viewport (the entire buffer width) at the cursor position
        VERIFY_IS_TRUE(_pDispatch->DeleteCharacter(_pTest->_coordBufferSize.X), L"Verify delete call was successful.");

        // cursor didn't move
        VERIFY_ARE_EQUAL(coordCursorExpected, _pTest->_coordCursorPos, L"Verify cursor didn't move from insert operation.");

        // Qs are the same outside the viewport
        VERIFY_IS_TRUE(_pTest->ValidateRectangleContains(srOuterBuffer, wchOuterBuffer, wAttrOuterBuffer, srViewport), L"Field of Qs outside viewport should remain unchanged.");

        // Entire viewport is Rs except the one space spot
        VERIFY_IS_TRUE(_pTest->ValidateRectangleContains(srViewport, wchViewport, wAttrViewport, srModifiedSpace), L"Field of Rs in the viewport outside modified space should remain unchanged.");

        // The inserted spaces at the left edge resulted in an entire line of spaces bounded by the viewport
        VERIFY_IS_TRUE(_pTest->ValidateRectangleContains(srModifiedSpace, wchDeleteExpected, wAttrDeleteExpected), L"A whole line of spaces was inserted from the right (the cursor position was deleted enough times.) Extra deletes just covered up some of the spaces that were shifted in.");
    }

    // Ensures that EraseScrollback (^[[3J) deletes any content from the buffer
    //  above the viewport, and moves the contents of the buffer in the
    //  viewport to 0,0. This emulates the xterm behavior of clearing any
    //  scrollback content.
    TEST_METHOD(EraseScrollbackTests)
    {
        _pTest->PrepData(CursorX::XCENTER, CursorY::YCENTER);
        _pTest->_wAttribute = _pTest->s_wAttrErase;
        Log::Comment(L"Starting Test");

        _pTest->_fSetConsoleWindowInfoResult = true;
        _pTest->_fExpectedWindowAbsolute = true;
        SMALL_RECT srRegion = { 0 };
        srRegion.Bottom = _pTest->_srViewport.Bottom - _pTest->_srViewport.Top - 1;
        srRegion.Right = _pTest->_srViewport.Right - _pTest->_srViewport.Left - 1;
        _pTest->_srExpectedConsoleWindow = srRegion;

        // The cursor will be moved to the same relative location in the new viewport with origin @ 0, 0
        const COORD coordRelativeCursor = { _pTest->_coordCursorPos.X - _pTest->_srViewport.Left,
                                            _pTest->_coordCursorPos.Y - _pTest->_srViewport.Top };
        _pTest->_coordExpectedCursorPos = coordRelativeCursor;

        VERIFY_IS_TRUE(_pDispatch->EraseInDisplay(TermDispatch::EraseType::Scrollback));

        // There are two portions of the screen that are cleared -
        //  below the viewport and to the right of the viewport.
        size_t cRegionsToCheck = 2;
        SMALL_RECT rgsrRegionsModified[2];

        // Region 0 - Below the viewport
        srRegion.Top = _pTest->_srViewport.Bottom + 1;
        srRegion.Left = 0;

        srRegion.Bottom = _pTest->_coordBufferSize.Y;
        srRegion.Right = _pTest->_coordBufferSize.X;

        rgsrRegionsModified[0] = srRegion;

        // Region 1 - To the right of the viewport
        srRegion.Top = 0;
        srRegion.Left = _pTest->_srViewport.Right + 1;

        srRegion.Bottom = _pTest->_coordBufferSize.Y;
        srRegion.Right = _pTest->_coordBufferSize.X;

        rgsrRegionsModified[1] = srRegion;

        // Scan entire buffer and ensure only the necessary region has changed.
        bool fRegionSuccess = _pTest->ValidateEraseBufferState(rgsrRegionsModified, cRegionsToCheck, TestGetSet::s_wchErase, TestGetSet::s_wAttrErase);
        VERIFY_IS_TRUE(fRegionSuccess);

        Log::Comment(L"Test 2: Gracefully fail when getting console information fails.");
        _pTest->PrepData();
        _pTest->_fGetConsoleScreenBufferInfoExResult = false;

        VERIFY_IS_FALSE(_pDispatch->EraseInDisplay(TermDispatch::EraseType::Scrollback));

        Log::Comment(L"Test 3: Gracefully fail when filling the rectangle fails.");
        _pTest->PrepData();
        _pTest->_fFillConsoleOutputCharacterWResult = false;

        VERIFY_IS_FALSE(_pDispatch->EraseInDisplay(TermDispatch::EraseType::Scrollback));
    }

    TEST_METHOD(EraseTests)
    {
        BEGIN_TEST_METHOD_PROPERTIES()
            TEST_METHOD_PROPERTY(L"Data:uiEraseType", L"{0, 1, 2}") // corresponds to options in TermDispatch::EraseType
            TEST_METHOD_PROPERTY(L"Data:fEraseScreen", L"{FALSE, TRUE}") // corresponds to Line (FALSE) or Screen (TRUE)
        END_TEST_METHOD_PROPERTIES()

        // Modify variables based on type of this test
        TermDispatch::EraseType eraseType;
        unsigned int uiEraseType;
        VERIFY_SUCCEEDED_RETURN(TestData::TryGetValue(L"uiEraseType", uiEraseType));
        eraseType = (TermDispatch::EraseType)uiEraseType;

        bool fEraseScreen;
        VERIFY_SUCCEEDED_RETURN(TestData::TryGetValue(L"fEraseScreen", fEraseScreen));

        Log::Comment(L"Starting test...");

        // This combiniation is a simple VT api call
        // Verify that the adapter calls that function, and do nothing else.
        // This functionality is covered by ScreenBufferTests::EraseAllTests
        if (eraseType == TermDispatch::EraseType::All && fEraseScreen)
        {
            Log::Comment(L"Testing Erase in Display - All");
            VERIFY_IS_TRUE(_pDispatch->EraseInDisplay(eraseType));
            return;
        }

        Log::Comment(L"Test 1: Perform standard erase operation.");
        switch (eraseType)
        {
        case TermDispatch::EraseType::FromBeginning:
            Log::Comment(L"Erasing line from beginning to cursor.");
            break;
        case TermDispatch::EraseType::ToEnd:
            Log::Comment(L"Erasing line from cursor to end.");
            break;
        case TermDispatch::EraseType::All:
            Log::Comment(L"Erasing all.");
            break;
        default:
            VERIFY_FAIL(L"Unsupported erase type.");
        }

        if (!fEraseScreen)
        {
            Log::Comment(L"Erasing just one line (the cursor's line).");
        }
        else
        {
            Log::Comment(L"Erasing entire display (viewport). May be bounded by the cursor.");
        }

        _pTest->PrepData(CursorX::XCENTER, CursorY::YCENTER);
        _pTest->_wAttribute = _pTest->s_wAttrErase;

        if (!fEraseScreen)
        {
            VERIFY_IS_TRUE(_pDispatch->EraseInLine(eraseType));
        }
        else
        {
            VERIFY_IS_TRUE(_pDispatch->EraseInDisplay(eraseType));
        }

        // Will be always the region of the cursor line (minimum 1)
        // and 2 more if it's the display (for the regions before and after the cursor line, total 3)
        SMALL_RECT rgsrRegionsModified[3]; // max of 3 regions.

        // Determine selection rectangle for line containing the cursor.
        // All sides are inclusive of modified data. (unlike viewport normally)
        SMALL_RECT srRegion = { 0 };
        srRegion.Top = _pTest->_coordCursorPos.Y;
        srRegion.Bottom = srRegion.Top;

        switch (eraseType)
        {
        case TermDispatch::EraseType::FromBeginning:
        case TermDispatch::EraseType::All:
            srRegion.Left = _pTest->_srViewport.Left;
            break;
        case TermDispatch::EraseType::ToEnd:
            srRegion.Left = _pTest->_coordCursorPos.X;
            break;
        default:
            VERIFY_FAIL(L"Unsupported erase type.");
            break;
        }

        switch (eraseType)
        {
        case TermDispatch::EraseType::FromBeginning:
            srRegion.Right = _pTest->_coordCursorPos.X;
            break;
        case TermDispatch::EraseType::All:
        case TermDispatch::EraseType::ToEnd:
            srRegion.Right = _pTest->_srViewport.Right - 1;
            break;
        default:
            VERIFY_FAIL(L"Unsupported erase type.");
            break;
        }
        rgsrRegionsModified[0] = srRegion;

        size_t cRegionsToCheck = 1; // start with 1 region to check from the line above. We may add up to 2 more.

        // Need to calculate up to two more regions if this is a screen erase.
        if (fEraseScreen)
        {
            // If from beginning or all, add the region *before* the cursor line.
            if (eraseType == TermDispatch::EraseType::FromBeginning ||
                eraseType == TermDispatch::EraseType::All)
            {
                srRegion.Left = _pTest->_srViewport.Left;
                srRegion.Right = _pTest->_srViewport.Right - 1; // viewport is exclusive on the right. this test is inclusive so -1.
                srRegion.Top = _pTest->_srViewport.Top;

                srRegion.Bottom = _pTest->_coordCursorPos.Y - 1; // this might end up being above top. This will be checked below.

                // Only add it if this is still valid.
                if (srRegion.Bottom >= srRegion.Top)
                {
                    rgsrRegionsModified[cRegionsToCheck] = srRegion;
                    cRegionsToCheck++;
                }
            }

            // If from end or all, add the region *after* the cursor line.
            if (eraseType == TermDispatch::EraseType::ToEnd ||
                eraseType == TermDispatch::EraseType::All)
            {
                srRegion.Left = _pTest->_srViewport.Left;
                srRegion.Right = _pTest->_srViewport.Right - 1; // viewport is exclusive rectangle on the right. this test uses inclusive rectangles so -1.
                srRegion.Bottom = _pTest->_srViewport.Bottom - 1; // viewport is exclusive rectangle on the bottom. this test uses inclusive rectangles so -1;

                srRegion.Top = _pTest->_coordCursorPos.Y + 1; // this might end up being below bottom. This will be checked below.

                // Only add it if this is still valid.
                if (srRegion.Bottom >= srRegion.Top)
                {
                    rgsrRegionsModified[cRegionsToCheck] = srRegion;
                    cRegionsToCheck++;
                }
            }
        }

        // Scan entire buffer and ensure only the necessary region has changed.
        bool fRegionSuccess = _pTest->ValidateEraseBufferState(rgsrRegionsModified, cRegionsToCheck, TestGetSet::s_wchErase, TestGetSet::s_wAttrErase);
        VERIFY_IS_TRUE(fRegionSuccess);

        Log::Comment(L"Test 2: Gracefully fail when getting console information fails.");
        _pTest->PrepData();
        _pTest->_fGetConsoleScreenBufferInfoExResult = false;

        if (!fEraseScreen)
        {
            VERIFY_IS_FALSE(_pDispatch->EraseInLine(eraseType));
        }
        else
        {
            VERIFY_IS_FALSE(_pDispatch->EraseInDisplay(eraseType));
        }

        Log::Comment(L"Test 3: Gracefully fail when filling the rectangle fails.");
        _pTest->PrepData();
        _pTest->_fFillConsoleOutputCharacterWResult = false;

        if (!fEraseScreen)
        {
            VERIFY_IS_FALSE(_pDispatch->EraseInLine(eraseType));
        }
        else
        {
            VERIFY_IS_FALSE(_pDispatch->EraseInDisplay(eraseType));
        }
    }

    TEST_METHOD(GraphicsBaseTests)
    {
        Log::Comment(L"Starting test...");


        Log::Comment(L"Test 1: Send no options.");

        _pTest->PrepData();

        TermDispatch::GraphicsOptions rgOptions[16];
        size_t cOptions = 0;

        VERIFY_IS_TRUE(_pDispatch->SetGraphicsRendition(rgOptions, cOptions));

        Log::Comment(L"Test 2: Gracefully fail when getting buffer information fails.");

        _pTest->PrepData();
        _pTest->_fPrivateGetConsoleScreenBufferAttributesResult = FALSE;

        VERIFY_IS_FALSE(_pDispatch->SetGraphicsRendition(rgOptions, cOptions));

        Log::Comment(L"Test 3: Gracefully fail when setting attribute data fails.");

        _pTest->PrepData();
        _pTest->_fSetConsoleTextAttributeResult = FALSE;
        // Need at least one option in order for the call to be able to fail.
        rgOptions[0] = (TermDispatch::GraphicsOptions) 0;
        cOptions = 1;
        VERIFY_IS_FALSE(_pDispatch->SetGraphicsRendition(rgOptions, cOptions));
    }

    TEST_METHOD(GraphicsSingleTests)
    {
        BEGIN_TEST_METHOD_PROPERTIES()
            TEST_METHOD_PROPERTY(L"Data:uiGraphicsOptions", L"{0, 1, 4, 7, 24, 27, 30, 31, 32, 33, 34, 35, 36, 37, 39, 40, 41, 42, 43, 44, 45, 46, 47, 49, 90, 91, 92, 93, 94, 95, 96, 97, 100, 101, 102, 103, 104, 105, 106, 107}") // corresponds to options in TermDispatch::GraphicsOptions
        END_TEST_METHOD_PROPERTIES()

        Log::Comment(L"Starting test...");
        _pTest->PrepData();

        // Modify variables based on type of this test
        TermDispatch::GraphicsOptions graphicsOption;
        unsigned int uiGraphicsOption;
        VERIFY_SUCCEEDED_RETURN(TestData::TryGetValue(L"uiGraphicsOptions", uiGraphicsOption));
        graphicsOption = (TermDispatch::GraphicsOptions)uiGraphicsOption;

        TermDispatch::GraphicsOptions rgOptions[16];
        size_t cOptions = 1;
        rgOptions[0] = graphicsOption;

        _pTest->_fPrivateSetLegacyAttributesResult = TRUE;

        switch (graphicsOption)
        {
        case TermDispatch::GraphicsOptions::Off:
            Log::Comment(L"Testing graphics 'Off/Reset'");
            _pTest->_wAttribute = (WORD)~s_wDefaultFill;
            _pTest->_wExpectedAttribute = s_wDefaultFill;
            _pTest->_fExpectedForeground = true;
            _pTest->_fExpectedBackground = true;
            _pTest->_fExpectedMeta = true;
            break;
        case TermDispatch::GraphicsOptions::BoldBright:
            Log::Comment(L"Testing graphics 'Bold/Bright'");
            _pTest->_wAttribute = 0;
            _pTest->_wExpectedAttribute = FOREGROUND_INTENSITY;
            _pTest->_fExpectedForeground = true;
            break;
        case TermDispatch::GraphicsOptions::Underline:
            Log::Comment(L"Testing graphics 'Underline'");
            _pTest->_wAttribute = 0;
            _pTest->_wExpectedAttribute = COMMON_LVB_UNDERSCORE;
            _pTest->_fExpectedMeta = true;
            break;
        case TermDispatch::GraphicsOptions::Negative:
            Log::Comment(L"Testing graphics 'Negative'");
            _pTest->_wAttribute = 0;
            _pTest->_wExpectedAttribute = COMMON_LVB_REVERSE_VIDEO;
            _pTest->_fExpectedMeta = true;
            break;
        case TermDispatch::GraphicsOptions::NoUnderline:
            Log::Comment(L"Testing graphics 'No Underline'");
            _pTest->_wAttribute = COMMON_LVB_UNDERSCORE;
            _pTest->_wExpectedAttribute = 0;
            _pTest->_fExpectedMeta = true;
            break;
        case TermDispatch::GraphicsOptions::Positive:
            Log::Comment(L"Testing graphics 'Positive'");
            _pTest->_wAttribute = COMMON_LVB_REVERSE_VIDEO;
            _pTest->_wExpectedAttribute = 0;
            _pTest->_fExpectedMeta = true;
            break;
        case TermDispatch::GraphicsOptions::ForegroundBlack:
            Log::Comment(L"Testing graphics 'Foreground Color Black'");
            _pTest->_wAttribute = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
            _pTest->_wExpectedAttribute = 0;
            _pTest->_fExpectedForeground = true;
            break;
        case TermDispatch::GraphicsOptions::ForegroundBlue:
            Log::Comment(L"Testing graphics 'Foreground Color Blue'");
            _pTest->_wAttribute = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
            _pTest->_wExpectedAttribute = FOREGROUND_BLUE;
            _pTest->_fExpectedForeground = true;
            break;
        case TermDispatch::GraphicsOptions::ForegroundGreen:
            Log::Comment(L"Testing graphics 'Foreground Color Green'");
            _pTest->_wAttribute = FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
            _pTest->_wExpectedAttribute = FOREGROUND_GREEN;
            _pTest->_fExpectedForeground = true;
            break;
        case TermDispatch::GraphicsOptions::ForegroundCyan:
            Log::Comment(L"Testing graphics 'Foreground Color Cyan'");
            _pTest->_wAttribute = FOREGROUND_RED | FOREGROUND_INTENSITY;
            _pTest->_wExpectedAttribute = FOREGROUND_BLUE | FOREGROUND_GREEN;
            _pTest->_fExpectedForeground = true;
            break;
        case TermDispatch::GraphicsOptions::ForegroundRed:
            Log::Comment(L"Testing graphics 'Foreground Color Red'");
            _pTest->_wAttribute = FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
            _pTest->_wExpectedAttribute = FOREGROUND_RED;
            _pTest->_fExpectedForeground = true;
            break;
        case TermDispatch::GraphicsOptions::ForegroundMagenta:
            Log::Comment(L"Testing graphics 'Foreground Color Magenta'");
            _pTest->_wAttribute = FOREGROUND_GREEN | FOREGROUND_INTENSITY;
            _pTest->_wExpectedAttribute = FOREGROUND_BLUE | FOREGROUND_RED;
            _pTest->_fExpectedForeground = true;
            break;
        case TermDispatch::GraphicsOptions::ForegroundYellow:
            Log::Comment(L"Testing graphics 'Foreground Color Yellow'");
            _pTest->_wAttribute = FOREGROUND_BLUE | FOREGROUND_INTENSITY;
            _pTest->_wExpectedAttribute = FOREGROUND_GREEN | FOREGROUND_RED;
            _pTest->_fExpectedForeground = true;
            break;
        case TermDispatch::GraphicsOptions::ForegroundWhite:
            Log::Comment(L"Testing graphics 'Foreground Color White'");
            _pTest->_wAttribute = FOREGROUND_INTENSITY;
            _pTest->_wExpectedAttribute = FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED;
            _pTest->_fExpectedForeground = true;
            break;
        case TermDispatch::GraphicsOptions::ForegroundDefault:
            Log::Comment(L"Testing graphics 'Foreground Color Default'");
            _pTest->_wAttribute = (WORD)~_pTest->s_wDefaultAttribute; // set the current attribute to the opposite of default so we can ensure all relevant bits flip.
            // To get expected value, take what we started with and change ONLY the background series of bits to what the Default says.
            _pTest->_wExpectedAttribute = _pTest->_wAttribute; // expect = starting
            _pTest->_wExpectedAttribute &= ~(FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY); // turn off all bits related to the background
            _pTest->_wExpectedAttribute |= (s_wDefaultFill & (FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY)); // reapply ONLY background bits from the default attribute.
            _pTest->_fExpectedForeground = true;
            break;
        case TermDispatch::GraphicsOptions::BackgroundBlack:
            Log::Comment(L"Testing graphics 'Background Color Black'");
            _pTest->_wAttribute = BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE | BACKGROUND_INTENSITY;
            _pTest->_wExpectedAttribute = 0;
            _pTest->_fExpectedBackground = true;
            break;
        case TermDispatch::GraphicsOptions::BackgroundBlue:
            Log::Comment(L"Testing graphics 'Background Color Blue'");
            _pTest->_wAttribute = BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_INTENSITY;
            _pTest->_wExpectedAttribute = BACKGROUND_BLUE;
            _pTest->_fExpectedBackground = true;
            break;
        case TermDispatch::GraphicsOptions::BackgroundGreen:
            Log::Comment(L"Testing graphics 'Background Color Green'");
            _pTest->_wAttribute = BACKGROUND_RED | BACKGROUND_BLUE | BACKGROUND_INTENSITY;
            _pTest->_wExpectedAttribute = BACKGROUND_GREEN;
            _pTest->_fExpectedBackground = true;
            break;
        case TermDispatch::GraphicsOptions::BackgroundCyan:
            Log::Comment(L"Testing graphics 'Background Color Cyan'");
            _pTest->_wAttribute = BACKGROUND_RED | BACKGROUND_INTENSITY;
            _pTest->_wExpectedAttribute = BACKGROUND_BLUE | BACKGROUND_GREEN;
            _pTest->_fExpectedBackground = true;
            break;
        case TermDispatch::GraphicsOptions::BackgroundRed:
            Log::Comment(L"Testing graphics 'Background Color Red'");
            _pTest->_wAttribute = BACKGROUND_BLUE | BACKGROUND_GREEN | BACKGROUND_INTENSITY;
            _pTest->_wExpectedAttribute = BACKGROUND_RED;
            _pTest->_fExpectedBackground = true;
            break;
        case TermDispatch::GraphicsOptions::BackgroundMagenta:
            Log::Comment(L"Testing graphics 'Background Color Magenta'");
            _pTest->_wAttribute = BACKGROUND_GREEN | BACKGROUND_INTENSITY;
            _pTest->_wExpectedAttribute = BACKGROUND_BLUE | BACKGROUND_RED;
            _pTest->_fExpectedBackground = true;
            break;
        case TermDispatch::GraphicsOptions::BackgroundYellow:
            Log::Comment(L"Testing graphics 'Background Color Yellow'");
            _pTest->_wAttribute = BACKGROUND_BLUE | BACKGROUND_INTENSITY;
            _pTest->_wExpectedAttribute = BACKGROUND_GREEN | BACKGROUND_RED;
            _pTest->_fExpectedBackground = true;
            break;
        case TermDispatch::GraphicsOptions::BackgroundWhite:
            Log::Comment(L"Testing graphics 'Background Color White'");
            _pTest->_wAttribute = BACKGROUND_INTENSITY;
            _pTest->_wExpectedAttribute = BACKGROUND_BLUE | BACKGROUND_GREEN | BACKGROUND_RED;
            _pTest->_fExpectedBackground = true;
            break;
        case TermDispatch::GraphicsOptions::BackgroundDefault:
            Log::Comment(L"Testing graphics 'Background Color Default'");
            _pTest->_wAttribute = (WORD)~_pTest->s_wDefaultAttribute; // set the current attribute to the opposite of default so we can ensure all relevant bits flip.
            // To get expected value, take what we started with and change ONLY the background series of bits to what the Default says.
            _pTest->_wExpectedAttribute = _pTest->_wAttribute; // expect = starting
            _pTest->_wExpectedAttribute &= ~(BACKGROUND_BLUE| BACKGROUND_GREEN | BACKGROUND_RED | BACKGROUND_INTENSITY); // turn off all bits related to the background
            _pTest->_wExpectedAttribute |= (s_wDefaultFill & (BACKGROUND_BLUE | BACKGROUND_GREEN | BACKGROUND_RED | BACKGROUND_INTENSITY)); // reapply ONLY background bits from the default attribute.
            _pTest->_fExpectedBackground = true;
            break;
        case TermDispatch::GraphicsOptions::BrightForegroundBlack:
            Log::Comment(L"Testing graphics 'Bright Foreground Color Black'");
            _pTest->_wAttribute = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
            _pTest->_wExpectedAttribute = FOREGROUND_INTENSITY;
            _pTest->_fExpectedForeground = true;
            break;
        case TermDispatch::GraphicsOptions::BrightForegroundBlue:
            Log::Comment(L"Testing graphics 'Bright Foreground Color Blue'");
            _pTest->_wAttribute = FOREGROUND_RED | FOREGROUND_GREEN;
            _pTest->_wExpectedAttribute = FOREGROUND_INTENSITY | FOREGROUND_BLUE;
            _pTest->_fExpectedForeground = true;
            break;
        case TermDispatch::GraphicsOptions::BrightForegroundGreen:
            Log::Comment(L"Testing graphics 'Bright Foreground Color Green'");
            _pTest->_wAttribute = FOREGROUND_RED | FOREGROUND_BLUE;
            _pTest->_wExpectedAttribute = FOREGROUND_INTENSITY | FOREGROUND_GREEN;
            _pTest->_fExpectedForeground = true;
            break;
        case TermDispatch::GraphicsOptions::BrightForegroundCyan:
            Log::Comment(L"Testing graphics 'Bright Foreground Color Cyan'");
            _pTest->_wAttribute = FOREGROUND_RED;
            _pTest->_wExpectedAttribute = FOREGROUND_INTENSITY | FOREGROUND_BLUE | FOREGROUND_GREEN;
            _pTest->_fExpectedForeground = true;
            break;
        case TermDispatch::GraphicsOptions::BrightForegroundRed:
            Log::Comment(L"Testing graphics 'Bright Foreground Color Red'");
            _pTest->_wAttribute = FOREGROUND_BLUE | FOREGROUND_GREEN;
            _pTest->_wExpectedAttribute = FOREGROUND_INTENSITY | FOREGROUND_RED;
            _pTest->_fExpectedForeground = true;
            break;
        case TermDispatch::GraphicsOptions::BrightForegroundMagenta:
            Log::Comment(L"Testing graphics 'Bright Foreground Color Magenta'");
            _pTest->_wAttribute = FOREGROUND_GREEN;
            _pTest->_wExpectedAttribute = FOREGROUND_INTENSITY | FOREGROUND_BLUE | FOREGROUND_RED;
            _pTest->_fExpectedForeground = true;
            break;
        case TermDispatch::GraphicsOptions::BrightForegroundYellow:
            Log::Comment(L"Testing graphics 'Bright Foreground Color Yellow'");
            _pTest->_wAttribute = FOREGROUND_BLUE;
            _pTest->_wExpectedAttribute = FOREGROUND_INTENSITY | FOREGROUND_GREEN | FOREGROUND_RED;
            _pTest->_fExpectedForeground = true;
            break;
        case TermDispatch::GraphicsOptions::BrightForegroundWhite:
            Log::Comment(L"Testing graphics 'Bright Foreground Color White'");
            _pTest->_wAttribute = 0;
            _pTest->_wExpectedAttribute = FOREGROUND_INTENSITY | FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED;
            _pTest->_fExpectedForeground = true;
            break;
        case TermDispatch::GraphicsOptions::BrightBackgroundBlack:
            Log::Comment(L"Testing graphics 'Bright Background Color Black'");
            _pTest->_wAttribute = BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE;
            _pTest->_wExpectedAttribute = BACKGROUND_INTENSITY;
            _pTest->_fExpectedBackground = true;
            break;
        case TermDispatch::GraphicsOptions::BrightBackgroundBlue:
            Log::Comment(L"Testing graphics 'Bright Background Color Blue'");
            _pTest->_wAttribute = BACKGROUND_RED | BACKGROUND_GREEN;
            _pTest->_wExpectedAttribute = BACKGROUND_INTENSITY | BACKGROUND_BLUE;
            _pTest->_fExpectedBackground = true;
            break;
        case TermDispatch::GraphicsOptions::BrightBackgroundGreen:
            Log::Comment(L"Testing graphics 'Bright Background Color Green'");
            _pTest->_wAttribute = BACKGROUND_RED | BACKGROUND_BLUE;
            _pTest->_wExpectedAttribute = BACKGROUND_INTENSITY | BACKGROUND_GREEN;
            _pTest->_fExpectedBackground = true;
            break;
        case TermDispatch::GraphicsOptions::BrightBackgroundCyan:
            Log::Comment(L"Testing graphics 'Bright Background Color Cyan'");
            _pTest->_wAttribute = BACKGROUND_RED;
            _pTest->_wExpectedAttribute = BACKGROUND_INTENSITY | BACKGROUND_BLUE | BACKGROUND_GREEN;
            _pTest->_fExpectedBackground = true;
            break;
        case TermDispatch::GraphicsOptions::BrightBackgroundRed:
            Log::Comment(L"Testing graphics 'Bright Background Color Red'");
            _pTest->_wAttribute = BACKGROUND_BLUE | BACKGROUND_GREEN;
            _pTest->_wExpectedAttribute = BACKGROUND_INTENSITY | BACKGROUND_RED;
            _pTest->_fExpectedBackground = true;
            break;
        case TermDispatch::GraphicsOptions::BrightBackgroundMagenta:
            Log::Comment(L"Testing graphics 'Bright Background Color Magenta'");
            _pTest->_wAttribute = BACKGROUND_GREEN;
            _pTest->_wExpectedAttribute = BACKGROUND_INTENSITY | BACKGROUND_BLUE | BACKGROUND_RED;
            _pTest->_fExpectedBackground = true;
            break;
        case TermDispatch::GraphicsOptions::BrightBackgroundYellow:
            Log::Comment(L"Testing graphics 'Bright Background Color Yellow'");
            _pTest->_wAttribute = BACKGROUND_BLUE;
            _pTest->_wExpectedAttribute = BACKGROUND_INTENSITY | BACKGROUND_GREEN | BACKGROUND_RED;
            _pTest->_fExpectedBackground = true;
            break;
        case TermDispatch::GraphicsOptions::BrightBackgroundWhite:
            Log::Comment(L"Testing graphics 'Bright Background Color White'");
            _pTest->_wAttribute = 0;
            _pTest->_wExpectedAttribute = BACKGROUND_INTENSITY | BACKGROUND_BLUE | BACKGROUND_GREEN | BACKGROUND_RED;
            _pTest->_fExpectedBackground = true;
            break;
        default:
            VERIFY_FAIL(L"Test not implemented yet!");
            break;
        }

        VERIFY_IS_TRUE(_pDispatch->SetGraphicsRendition(rgOptions, cOptions));
    }

    TEST_METHOD(GraphicsPersistBrightnessTests)
    {
        Log::Comment(L"Starting test...");

        _pTest->PrepData(); // default color from here is gray on black, FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED

        _pTest->_fPrivateSetLegacyAttributesResult = TRUE;

        TermDispatch::GraphicsOptions rgOptions[16];
        size_t cOptions = 1;

        Log::Comment(L"Test 1: Basic brightness test");
        Log::Comment(L"Reseting graphics options");
        rgOptions[0] = TermDispatch::GraphicsOptions::Off;
        _pTest->_wExpectedAttribute = FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED;
        _pTest->_fExpectedForeground = true;
        _pTest->_fExpectedBackground = true;
        _pTest->_fExpectedMeta = true;
        VERIFY_IS_TRUE(_pDispatch->SetGraphicsRendition(rgOptions, cOptions));

        Log::Comment(L"Testing graphics 'Foreground Color Blue'");
        rgOptions[0] = TermDispatch::GraphicsOptions::ForegroundBlue;
        _pTest->_wExpectedAttribute = FOREGROUND_BLUE;
        _pTest->_fExpectedForeground = true;
        VERIFY_IS_TRUE(_pDispatch->SetGraphicsRendition(rgOptions, cOptions));

        Log::Comment(L"Enabling brightness");
        rgOptions[0] = TermDispatch::GraphicsOptions::BoldBright;
        _pTest->_wExpectedAttribute = FOREGROUND_BLUE | FOREGROUND_INTENSITY;
        _pTest->_fExpectedForeground = true;
        VERIFY_IS_TRUE(_pDispatch->SetGraphicsRendition(rgOptions, cOptions));

        Log::Comment(L"Testing graphics 'Foreground Color Green, with brightness'");
        rgOptions[0] = TermDispatch::GraphicsOptions::ForegroundGreen;
        _pTest->_wExpectedAttribute = FOREGROUND_GREEN | FOREGROUND_INTENSITY;
        _pTest->_fExpectedForeground = true;
        VERIFY_IS_TRUE(_pDispatch->SetGraphicsRendition(rgOptions, cOptions));

        Log::Comment(L"Test 2: Disable brightness, use a bright color, next normal call remains not bright");
        Log::Comment(L"Reseting graphics options");
        rgOptions[0] = TermDispatch::GraphicsOptions::Off;
        _pTest->_wExpectedAttribute = FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED;
        _pTest->_fExpectedForeground = true;
        _pTest->_fExpectedBackground = true;
        _pTest->_fExpectedMeta = true;
        VERIFY_IS_TRUE(_pDispatch->SetGraphicsRendition(rgOptions, cOptions));

        Log::Comment(L"Testing graphics 'Foreground Color Bright Blue'");
        rgOptions[0] = TermDispatch::GraphicsOptions::BrightForegroundBlue;
        _pTest->_wExpectedAttribute = FOREGROUND_BLUE | FOREGROUND_INTENSITY;
        _pTest->_fExpectedForeground = true;
        VERIFY_IS_TRUE(_pDispatch->SetGraphicsRendition(rgOptions, cOptions));

        Log::Comment(L"Testing graphics 'Foreground Color Blue', brightness of 9x series doesn't persist");
        rgOptions[0] = TermDispatch::GraphicsOptions::ForegroundBlue;
        _pTest->_wExpectedAttribute = FOREGROUND_BLUE;
        _pTest->_fExpectedForeground = true;
        VERIFY_IS_TRUE(_pDispatch->SetGraphicsRendition(rgOptions, cOptions));

        Log::Comment(L"Test 3: Enable brightness, use a bright color, brightness persists to next normal call");
        Log::Comment(L"Reseting graphics options");
        rgOptions[0] = TermDispatch::GraphicsOptions::Off;
        _pTest->_wExpectedAttribute = FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED;
        _pTest->_fExpectedForeground = true;
        _pTest->_fExpectedBackground = true;
        _pTest->_fExpectedMeta = true;
        VERIFY_IS_TRUE(_pDispatch->SetGraphicsRendition(rgOptions, cOptions));

        Log::Comment(L"Testing graphics 'Foreground Color Blue'");
        rgOptions[0] = TermDispatch::GraphicsOptions::ForegroundBlue;
        _pTest->_wExpectedAttribute = FOREGROUND_BLUE;
        _pTest->_fExpectedForeground = true;
        VERIFY_IS_TRUE(_pDispatch->SetGraphicsRendition(rgOptions, cOptions));

        Log::Comment(L"Enabling brightness");
        rgOptions[0] = TermDispatch::GraphicsOptions::BoldBright;
        _pTest->_wExpectedAttribute = FOREGROUND_BLUE | FOREGROUND_INTENSITY;
        _pTest->_fExpectedForeground = true;
        VERIFY_IS_TRUE(_pDispatch->SetGraphicsRendition(rgOptions, cOptions));

        Log::Comment(L"Testing graphics 'Foreground Color Bright Blue'");
        rgOptions[0] = TermDispatch::GraphicsOptions::BrightForegroundBlue;
        _pTest->_wExpectedAttribute = FOREGROUND_BLUE | FOREGROUND_INTENSITY;
        _pTest->_fExpectedForeground = true;
        VERIFY_IS_TRUE(_pDispatch->SetGraphicsRendition(rgOptions, cOptions));

        Log::Comment(L"Testing graphics 'Foreground Color Blue, with brightness', brightness of 9x series doesn't affect brightness");
        rgOptions[0] = TermDispatch::GraphicsOptions::ForegroundBlue;
        _pTest->_wExpectedAttribute = FOREGROUND_BLUE | FOREGROUND_INTENSITY;
        _pTest->_fExpectedForeground = true;
        VERIFY_IS_TRUE(_pDispatch->SetGraphicsRendition(rgOptions, cOptions));

        Log::Comment(L"Testing graphics 'Foreground Color Green, with brightness'");
        rgOptions[0] = TermDispatch::GraphicsOptions::ForegroundGreen;
        _pTest->_wExpectedAttribute = FOREGROUND_GREEN | FOREGROUND_INTENSITY;
        _pTest->_fExpectedForeground = true;
        VERIFY_IS_TRUE(_pDispatch->SetGraphicsRendition(rgOptions, cOptions));
    }

    TEST_METHOD(DeviceStatusReportTests)
    {
        Log::Comment(L"Starting test...");

        Log::Comment(L"Test 1: Verify failure when using bad status.");
        _pTest->PrepData();
        VERIFY_IS_FALSE(_pDispatch->DeviceStatusReport((TermDispatch::AnsiStatusType) -1));
    }

    TEST_METHOD(DeviceStatus_CursorPositionReportTests)
    {
        Log::Comment(L"Starting test...");

        Log::Comment(L"Test 1: Verify normal cursor response position.");
        _pTest->PrepData(CursorX::XCENTER, CursorY::YCENTER);

        // start with the cursor position in the buffer.
        COORD coordCursorExpected = _pTest->_coordCursorPos;

        // to get to VT, we have to adjust it to its position relative to the viewport.
        coordCursorExpected.X -= _pTest->_srViewport.Left;
        coordCursorExpected.Y -= _pTest->_srViewport.Top;

        // Then note that VT is 1,1 based for the top left, so add 1. (The rest of the console uses 0,0 for array index bases.)
        coordCursorExpected.X++;
        coordCursorExpected.Y++;

        VERIFY_IS_TRUE(_pDispatch->DeviceStatusReport(TermDispatch::AnsiStatusType::CPR_CursorPositionReport));

        wchar_t pwszBuffer[50];

        swprintf_s(pwszBuffer, ARRAYSIZE(pwszBuffer), L"\x1b[%d;%dR", coordCursorExpected.Y, coordCursorExpected.X);
        _pTest->ValidateInputEvent(pwszBuffer);
    }

    TEST_METHOD(DeviceAttributesTests)
    {
        Log::Comment(L"Starting test...");

        Log::Comment(L"Test 1: Verify normal response.");
        _pTest->PrepData();
        VERIFY_IS_TRUE(_pDispatch->DeviceAttributes());

        PCWSTR pwszExpectedResponse = L"\x1b[?1;0c";
        _pTest->ValidateInputEvent(pwszExpectedResponse);

        Log::Comment(L"Test 2: Verify failure when WriteConsoleInput doesn't work.");
        _pTest->PrepData();
        _pTest->_fPrivatePrependConsoleInputResult = FALSE;

        VERIFY_IS_FALSE(_pDispatch->DeviceAttributes());
    }

    TEST_METHOD(ScrollTest)
    {
        BEGIN_TEST_METHOD_PROPERTIES()
            TEST_METHOD_PROPERTY(L"Data:uiDirection", L"{0, 1}") // These values align with the ScrollDirection enum class to try all the directions.
            TEST_METHOD_PROPERTY(L"Data:uiMagnitude", L"{1, 2, 5}") // These values align with the ScrollDirection enum class to try all the directions.
        END_TEST_METHOD_PROPERTIES()

        Log::Comment(L"Starting test...");

        // Used to switch between the various function options.
        typedef bool(AdaptDispatch::*ScrollFunc)(const unsigned int);
        ScrollFunc scrollFunc = nullptr;

        // Modify variables based on directionality of this test
        ScrollDirection direction;
        unsigned int dir;
        VERIFY_SUCCEEDED_RETURN(TestData::TryGetValue(L"uiDirection", dir));
        direction = (ScrollDirection)dir;
        unsigned int uiMagnitude;
        VERIFY_SUCCEEDED_RETURN(TestData::TryGetValue(L"uiMagnitude", uiMagnitude));
        SHORT sMagnitude = (SHORT)uiMagnitude;

        switch (direction)
        {
        case ScrollDirection::UP:
            Log::Comment(L"Testing up direction.");
            scrollFunc = &AdaptDispatch::ScrollUp;
            break;
        case ScrollDirection::DOWN:
            Log::Comment(L"Testing down direction.");
            scrollFunc = &AdaptDispatch::ScrollDown;
            break;
        }
        Log::Comment(NoThrowString().Format(L"Scrolling by %d lines", uiMagnitude));
        if (scrollFunc == nullptr)
        {
            VERIFY_FAIL();
            return;
        }

        // place the cursor in the center.
        _pTest->PrepData(CursorX::XCENTER, CursorY::YCENTER);

        // Save the cursor position. It shouldn't move for the rest of the test.
        COORD coordCursorExpected = _pTest->_coordCursorPos;

        // Fill the entire buffer with Qs. Blue on Green.
        WCHAR const wchOuterBuffer = 'Q';
        WORD const wAttrOuterBuffer = FOREGROUND_BLUE | BACKGROUND_GREEN;
        SMALL_RECT srOuterBuffer;
        srOuterBuffer.Top = 0;
        srOuterBuffer.Left = 0;
        srOuterBuffer.Bottom = _pTest->_coordBufferSize.Y;
        srOuterBuffer.Right= _pTest->_coordBufferSize.X;
        _pTest->FillRectangle(srOuterBuffer, wchOuterBuffer, wAttrOuterBuffer);

        // Fill the viewport with Rs. Red on Blue.
        WCHAR const wchViewport = 'R';
        WORD const wAttrViewport = FOREGROUND_RED | BACKGROUND_BLUE;
        SMALL_RECT srViewport = _pTest->_srViewport;
        _pTest->FillRectangle(srViewport, wchViewport, wAttrViewport);

        // Add some characters to see if they moved.
        // change the color too so we can make sure that it's fine

        WORD const wAttrTestText = FOREGROUND_GREEN;
        PWSTR const pwszTestText = L"ABCDE"; // Text is written at y=34, moves to y=33
        size_t cchTestText = wcslen(pwszTestText);
        SMALL_RECT srTestText;
        srTestText.Top = _pTest->_coordCursorPos.Y;
        srTestText.Bottom = srTestText.Top + 1;
        srTestText.Left = _pTest->_coordCursorPos.X;
        srTestText.Right = srTestText.Left + (SHORT)cchTestText;
        _pTest->InsertString(_pTest->_coordCursorPos, pwszTestText, wAttrTestText);

        //Scroll Up one line
        VERIFY_IS_TRUE((_pDispatch->*(scrollFunc))(sMagnitude), L"Verify Scroll call was sucessful.");

        // verify cursor didn't move
        VERIFY_ARE_EQUAL(coordCursorExpected, _pTest->_coordCursorPos, L"Verify cursor didn't move from insert operation.");

        // Verify the field of Qs didn't change outside the viewport.
        VERIFY_IS_TRUE(_pTest->ValidateRectangleContains(srOuterBuffer, wchOuterBuffer, wAttrOuterBuffer, srViewport),
                       L"Field of Qs outside viewport should remain unchanged.");

        // Okay, this part get confusing. These change depending on the direction of the test.
        // direction    InViewport      Outside
        // UP           Bottom Line     Top minus One
        // DOWN         Top Line        Bottom plus One
        const bool fScrollUp = (direction == ScrollDirection::UP);
        SMALL_RECT srInViewport = srViewport;
        srInViewport.Top = (fScrollUp)? (srViewport.Bottom - sMagnitude) : (srViewport.Top);
        srInViewport.Bottom = srInViewport.Top + sMagnitude;
        WCHAR const wchInViewport = ' ';
        WORD const wAttrInViewport = _pTest->_wAttribute;

        // Verify the bottom line is now empty
        VERIFY_IS_TRUE(_pTest->ValidateRectangleContains(srInViewport, wchInViewport, wAttrInViewport),
                       L"InViewport line(s) should now be blank, with default buffer attributes");

        SMALL_RECT srOutside = srViewport;
        srOutside.Top = (fScrollUp)? (srViewport.Top - sMagnitude) : (srViewport.Bottom);
        srOutside.Bottom = srOutside.Top + sMagnitude;
        WCHAR const wchOutside = wchOuterBuffer;
        WORD const wAttrOutside = wAttrOuterBuffer;

        // Verify the line above the viewport is unchanged
        VERIFY_IS_TRUE(_pTest->ValidateRectangleContains(srOutside, wchOutside, wAttrOutside),
                       L"Line(s) above the viewport is unchanged");

        // Verify that the line where the ABCDE is now wchViewport
        COORD coordTestText;
        PWSTR const pwszNewTestText = L"RRRRR";
        coordTestText.X = srTestText.Left;
        coordTestText.Y = srTestText.Top;
        VERIFY_IS_TRUE(_pTest->ValidateString(coordTestText, pwszNewTestText, wAttrViewport), L"Contents of viewport should have shifted to where the string used to be.");

        // Verify that the line above/below the ABCDE now has the ABCDE
        coordTestText.X = srTestText.Left;
        coordTestText.Y = (fScrollUp)? (srTestText.Top - sMagnitude) : (srTestText.Top + sMagnitude);
        VERIFY_IS_TRUE(_pTest->ValidateString(coordTestText, pwszTestText, wAttrTestText), L"String should have moved up/down by given magnitude.");

    }

    TEST_METHOD(CursorKeysModeTest)
    {

        Log::Comment(L"Starting test...");

        // success cases
        // set numeric mode = true
        Log::Comment(L"Test 1: application mode = false");
        _pTest->_fPrivateSetCursorKeysModeResult = TRUE;
        _pTest->_fCursorKeysApplicationMode = false;

        VERIFY_IS_TRUE(_pDispatch->SetCursorKeysMode(false));

        // set numeric mode = false
        Log::Comment(L"Test 2: application mode = true");
        _pTest->_fPrivateSetCursorKeysModeResult = TRUE;
        _pTest->_fCursorKeysApplicationMode = true;

        VERIFY_IS_TRUE(_pDispatch->SetCursorKeysMode(true));

    }

    TEST_METHOD(KeypadModeTest)
    {

        Log::Comment(L"Starting test...");

        // success cases
        // set numeric mode = true
        Log::Comment(L"Test 1: application mode = false");
        _pTest->_fPrivateSetKeypadModeResult = TRUE;
        _pTest->_fKeypadApplicationMode = false;

        VERIFY_IS_TRUE(_pDispatch->SetKeypadMode(false));

        // set numeric mode = false
        Log::Comment(L"Test 2: application mode = true");
        _pTest->_fPrivateSetKeypadModeResult = TRUE;
        _pTest->_fKeypadApplicationMode = true;

        VERIFY_IS_TRUE(_pDispatch->SetKeypadMode(true));

    }

    TEST_METHOD(AllowBlinkingTest)
    {

        Log::Comment(L"Starting test...");

        // success cases
        // set numeric mode = true
        Log::Comment(L"Test 1: enable blinking = true");
        _pTest->_fPrivateAllowCursorBlinkingResult = TRUE;
        _pTest->_fEnable = true;

        VERIFY_IS_TRUE(_pDispatch->EnableCursorBlinking(true));

        // set numeric mode = false
        Log::Comment(L"Test 2: enable blinking = false");
        _pTest->_fPrivateAllowCursorBlinkingResult = TRUE;
        _pTest->_fEnable = false;

        VERIFY_IS_TRUE(_pDispatch->EnableCursorBlinking(false));

    }

    TEST_METHOD(ScrollMarginsTest)
    {
        Log::Comment(L"Starting test...");

        SMALL_RECT srTestMargins = {0};
        _pTest->_srViewport.Bottom = 8;
        _pTest->_fGetConsoleScreenBufferInfoExResult = TRUE;

        Log::Comment(L"Test 1: Verify having both values is valid.");
        _pTest->_SetMarginsHelper(&srTestMargins, 2, 6);
        _pTest->_fPrivateSetScrollingRegionResult = TRUE;
        VERIFY_IS_TRUE(_pDispatch->SetTopBottomScrollingMargins(srTestMargins.Top, srTestMargins.Bottom, true));

        Log::Comment(L"Test 2: Verify having only top is valid.");

        _pTest->_SetMarginsHelper(&srTestMargins, 7, 0);
        _pTest->_srExpectedScrollRegion.Bottom = _pTest->_srViewport.Bottom - 1; // We expect the bottom to be the bottom of the viewport, exclusive.
        _pTest->_fPrivateSetScrollingRegionResult = TRUE;
        VERIFY_IS_TRUE(_pDispatch->SetTopBottomScrollingMargins(srTestMargins.Top, srTestMargins.Bottom, true));

        Log::Comment(L"Test 3: Verify having only bottom is valid.");

        _pTest->_SetMarginsHelper(&srTestMargins, 0, 7);
        _pTest->_fPrivateSetScrollingRegionResult = TRUE;
        VERIFY_IS_TRUE(_pDispatch->SetTopBottomScrollingMargins(srTestMargins.Top, srTestMargins.Bottom, true));

        Log::Comment(L"Test 4: Verify having no values is valid.");

        _pTest->_SetMarginsHelper(&srTestMargins, 0, 0);
        _pTest->_fPrivateSetScrollingRegionResult = TRUE;
        VERIFY_IS_TRUE(_pDispatch->SetTopBottomScrollingMargins(srTestMargins.Top, srTestMargins.Bottom, true));

        Log::Comment(L"Test 5: Verify having both values, but bad bounds is invalid.");

        _pTest->_SetMarginsHelper(&srTestMargins, 7, 3);
        _pTest->_fPrivateSetScrollingRegionResult = TRUE;
        VERIFY_IS_FALSE(_pDispatch->SetTopBottomScrollingMargins(srTestMargins.Top, srTestMargins.Bottom, true));


        Log::Comment(L"Test 6: Verify Setting margins to (0, height) clears them");
        // First set,
        _pTest->_fPrivateSetScrollingRegionResult = TRUE;
        _pTest->_SetMarginsHelper(&srTestMargins, 2, 6);
        VERIFY_IS_TRUE(_pDispatch->SetTopBottomScrollingMargins(srTestMargins.Top, srTestMargins.Bottom, true));
        // Then clear
        _pTest->_srExpectedScrollRegion.Top = 0;
        _pTest->_srExpectedScrollRegion.Bottom = 0;
        _pTest->_SetMarginsHelper(&srTestMargins, 0, 7);
        VERIFY_IS_TRUE(_pDispatch->SetTopBottomScrollingMargins(srTestMargins.Top, srTestMargins.Bottom, true));


        Log::Comment(L"Test 7: Verify Setting margins to (1, height) clears them");
        // First set,
        _pTest->_fPrivateSetScrollingRegionResult = TRUE;
        _pTest->_SetMarginsHelper(&srTestMargins, 2, 6);
        VERIFY_IS_TRUE(_pDispatch->SetTopBottomScrollingMargins(srTestMargins.Top, srTestMargins.Bottom, true));
        // Then clear
        _pTest->_srExpectedScrollRegion.Top = 0;
        _pTest->_srExpectedScrollRegion.Bottom = 0;
        _pTest->_SetMarginsHelper(&srTestMargins, 0, 7);
        VERIFY_IS_TRUE(_pDispatch->SetTopBottomScrollingMargins(srTestMargins.Top, srTestMargins.Bottom, true));

    }


    TEST_METHOD(TabSetClearTests)
    {
        Log::Comment(L"Starting test...");

        _pTest->_fPrivateHorizontalTabSetResult = TRUE;
        VERIFY_IS_TRUE(_pDispatch->HorizontalTabSet());

        _pTest->_sExpectedNumTabs = 16;

        _pTest->_fPrivateForwardTabResult = TRUE;
        VERIFY_IS_TRUE(_pDispatch->ForwardTab(16));

        _pTest->_fPrivateBackwardsTabResult = TRUE;
        VERIFY_IS_TRUE(_pDispatch->BackwardsTab(16));

        _pTest->_fPrivateTabClearResult = TRUE;
        _pTest->_fExpectedClearAll = true;
        VERIFY_IS_TRUE(_pDispatch->TabClear(TermDispatch::TabClearType::ClearAllColumns));

        _pTest->_fExpectedClearAll = false;
        VERIFY_IS_TRUE(_pDispatch->TabClear(TermDispatch::TabClearType::ClearCurrentColumn));

    }

    TEST_METHOD(SetConsoleTitleTest)
    {

        Log::Comment(L"Starting test...");

        Log::Comment(L"Test 1: set title to be non-null");
        _pTest->_fSetConsoleTitleWResult = TRUE;
        wchar_t* pwchTestString = L"Foo bar";
        _pTest->_pwchExpectedWindowTitle = pwchTestString;
        _pTest->_sCchExpectedTitleLength = 8;

        VERIFY_IS_TRUE(_pDispatch->SetWindowTitle(pwchTestString, 8));

        Log::Comment(L"Test 2: set title to be null");
        _pTest->_fSetConsoleTitleWResult = FALSE;
        _pTest->_pwchExpectedWindowTitle = nullptr;

        VERIFY_IS_TRUE(_pDispatch->SetWindowTitle(nullptr, 8));

    }

    TEST_METHOD(TestMouseModes)
    {
        Log::Comment(L"Starting test...");

        Log::Comment(L"Test 1: Test Default Mouse Mode");
        _pTest->_fExpectedMouseEnabled = true;
        _pTest->_fPrivateEnableVT200MouseModeResult = TRUE;
        VERIFY_IS_TRUE(_pDispatch->EnableVT200MouseMode(true));
        _pTest->_fExpectedMouseEnabled = false;
        VERIFY_IS_TRUE(_pDispatch->EnableVT200MouseMode(false));

        Log::Comment(L"Test 2: Test UTF-8 Extended Mouse Mode");
        _pTest->_fExpectedMouseEnabled = true;
        _pTest->_fPrivateEnableUTF8ExtendedMouseModeResult = TRUE;
        VERIFY_IS_TRUE(_pDispatch->EnableUTF8ExtendedMouseMode(true));
        _pTest->_fExpectedMouseEnabled = false;
        VERIFY_IS_TRUE(_pDispatch->EnableUTF8ExtendedMouseMode(false));

        Log::Comment(L"Test 3: Test SGR Extended Mouse Mode");
        _pTest->_fExpectedMouseEnabled = true;
        _pTest->_fPrivateEnableSGRExtendedMouseModeResult = TRUE;
        VERIFY_IS_TRUE(_pDispatch->EnableSGRExtendedMouseMode(true));
        _pTest->_fExpectedMouseEnabled = false;
        VERIFY_IS_TRUE(_pDispatch->EnableSGRExtendedMouseMode(false));

        Log::Comment(L"Test 4: Test Button-Event Mouse Mode");
        _pTest->_fExpectedMouseEnabled = true;
        _pTest->_fPrivateEnableButtonEventMouseModeResult = TRUE;
        VERIFY_IS_TRUE(_pDispatch->EnableButtonEventMouseMode(true));
        _pTest->_fExpectedMouseEnabled = false;
        VERIFY_IS_TRUE(_pDispatch->EnableButtonEventMouseMode(false));

        Log::Comment(L"Test 5: Test Any-Event Mouse Mode");
        _pTest->_fExpectedMouseEnabled = true;
        _pTest->_fPrivateEnableAnyEventMouseModeResult = TRUE;
        VERIFY_IS_TRUE(_pDispatch->EnableAnyEventMouseMode(true));
        _pTest->_fExpectedMouseEnabled = false;
        VERIFY_IS_TRUE(_pDispatch->EnableAnyEventMouseMode(false));

        Log::Comment(L"Test 6: Test Alt Scroll Mouse Mode");
        _pTest->_fExpectedAlternateScrollEnabled = true;
        _pTest->_fPrivateEnableAlternateScrollResult = TRUE;
        VERIFY_IS_TRUE(_pDispatch->EnableAlternateScroll(true));
        _pTest->_fExpectedAlternateScrollEnabled = false;
        VERIFY_IS_TRUE(_pDispatch->EnableAlternateScroll(false));
    }

    TEST_METHOD(Xterm256ColorTest)
    {
        Log::Comment(L"Starting test...");

        _pTest->PrepData(); // default color from here is gray on black, FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED


        TermDispatch::GraphicsOptions rgOptions[16];
        size_t cOptions = 3;

        _pTest->_fSetConsoleXtermTextAttributeResult = true;

        Log::Comment(L"Test 1: Change Foreground");
        rgOptions[0] = TermDispatch::GraphicsOptions::ForegroundExtended;
        rgOptions[1] = TermDispatch::GraphicsOptions::Xterm256Index;
        rgOptions[2] = (TermDispatch::GraphicsOptions)2; // Green
        _pTest->_wExpectedAttribute = FOREGROUND_GREEN;
        _pTest->_iExpectedXtermTableEntry = 2;
        _pTest->_fExpectedIsForeground = true;
        _pTest->_fUsingRgbColor = false;
        VERIFY_IS_TRUE(_pDispatch->SetGraphicsRendition(rgOptions, cOptions));

        Log::Comment(L"Test 2: Change Background");
        rgOptions[0] = TermDispatch::GraphicsOptions::BackgroundExtended;
        rgOptions[1] = TermDispatch::GraphicsOptions::Xterm256Index;
        rgOptions[2] = (TermDispatch::GraphicsOptions)9; // Bright Red
        _pTest->_wExpectedAttribute = FOREGROUND_GREEN | BACKGROUND_RED | BACKGROUND_INTENSITY;
        _pTest->_iExpectedXtermTableEntry = 9;
        _pTest->_fExpectedIsForeground = false;
        _pTest->_fUsingRgbColor = false;
        VERIFY_IS_TRUE(_pDispatch->SetGraphicsRendition(rgOptions, cOptions));



        Log::Comment(L"Test 3: Change Foreground to RGB color");
        rgOptions[0] = TermDispatch::GraphicsOptions::ForegroundExtended;
        rgOptions[1] = TermDispatch::GraphicsOptions::Xterm256Index;
        rgOptions[2] = (TermDispatch::GraphicsOptions)42; // Arbitrary Color
        _pTest->_iExpectedXtermTableEntry = 42;
        _pTest->_fExpectedIsForeground = true;
        _pTest->_fUsingRgbColor = true;
        VERIFY_IS_TRUE(_pDispatch->SetGraphicsRendition(rgOptions, cOptions));


        Log::Comment(L"Test 4: Change Background to RGB color");
        rgOptions[0] = TermDispatch::GraphicsOptions::BackgroundExtended;
        rgOptions[1] = TermDispatch::GraphicsOptions::Xterm256Index;
        rgOptions[2] = (TermDispatch::GraphicsOptions)142; // Arbitrary Color
        _pTest->_iExpectedXtermTableEntry = 142;
        _pTest->_fExpectedIsForeground = false;
        _pTest->_fUsingRgbColor = true;
        VERIFY_IS_TRUE(_pDispatch->SetGraphicsRendition(rgOptions, cOptions));

        Log::Comment(L"Test 5: Change Foreground to Legacy Attr while BG is RGB color");
        // Unfortunately this test isn't all that good, because the adapterTest adapter isn't smart enough
        //   to have it's own color table and translate the pre-existing RGB BG into a legacy BG.
        // Fortunately, the ft_api:RgbColorTests IS smart enough to test that.
        rgOptions[0] = TermDispatch::GraphicsOptions::ForegroundExtended;
        rgOptions[1] = TermDispatch::GraphicsOptions::Xterm256Index;
        rgOptions[2] = (TermDispatch::GraphicsOptions)9; // Bright Red
        _pTest->_wExpectedAttribute = FOREGROUND_RED | FOREGROUND_INTENSITY | BACKGROUND_RED | BACKGROUND_INTENSITY;
        _pTest->_iExpectedXtermTableEntry = 9;
        _pTest->_fExpectedIsForeground = true;
        _pTest->_fUsingRgbColor = false;
        VERIFY_IS_TRUE(_pDispatch->SetGraphicsRendition(rgOptions, cOptions));

    }


    TEST_METHOD(HardReset)
    {
        Log::Comment(L"Starting test...");

        _pTest->PrepData();

        ///////////////// Components of a EraseScrollback //////////////////////
        _pTest->_fExpectedWindowAbsolute = true;
        SMALL_RECT srRegion = { 0 };
        srRegion.Bottom = _pTest->_srViewport.Bottom - _pTest->_srViewport.Top - 1;
        srRegion.Right = _pTest->_srViewport.Right - _pTest->_srViewport.Left - 1;
        _pTest->_srExpectedConsoleWindow = srRegion;
        // The cursor will be moved to the same relative location in the new viewport with origin @ 0, 0
        const COORD coordRelativeCursor = { _pTest->_coordCursorPos.X - _pTest->_srViewport.Left,
                                            _pTest->_coordCursorPos.Y - _pTest->_srViewport.Top };

        // Cursor to 1,1
        _pTest->_coordExpectedCursorPos = {0, 0};
        _pTest->_fSetConsoleCursorPositionResult = true;
        _pTest->_fPrivateSetLegacyAttributesResult = true;
        _pTest->_fExpectedForeground = true;
        _pTest->_fExpectedBackground = true;
        _pTest->_fExpectedMeta = true;
        const COORD coordExpectedCursorPos = {0, 0};

        // Sets the SGR state to normal.
        _pTest->_wExpectedAttribute = FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED;

        VERIFY_IS_TRUE(_pDispatch->HardReset());
        VERIFY_ARE_EQUAL(_pTest->_coordCursorPos, coordExpectedCursorPos);
        VERIFY_ARE_EQUAL(_pTest->_fUsingRgbColor, false);

        Log::Comment(L"Test 2: Gracefully fail when getting console information fails.");
        _pTest->PrepData();
        _pTest->_fGetConsoleScreenBufferInfoExResult = false;

        VERIFY_IS_FALSE(_pDispatch->HardReset());

        Log::Comment(L"Test 3: Gracefully fail when filling the rectangle fails.");
        _pTest->PrepData();
        _pTest->_fFillConsoleOutputCharacterWResult = false;

        VERIFY_IS_FALSE(_pDispatch->HardReset());

        Log::Comment(L"Test 4: Gracefully fail when setting the window fails.");
        _pTest->PrepData();
        _pTest->_fSetConsoleWindowInfoResult = false;

        VERIFY_IS_FALSE(_pDispatch->HardReset());
    }

private:
    TestGetSet* _pTest;
    AdaptDispatch* _pDispatch;
    static const WORD s_wDefaultFill = FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED; // dark gray on black.
};
