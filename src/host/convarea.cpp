/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "_output.h"

#include "dbcs.h"

#include "..\interactivity\inc\ServiceLocator.hpp"

#pragma hdrstop

// Attributes flags:
#define COMMON_LVB_GRID_SINGLEFLAG 0x2000   // DBCS: Grid attribute: use for ime cursor.

SHORT CalcWideCharToColumn(_In_ PCHAR_INFO Buffer, _In_ size_t NumberOfChars);

void ConsoleImeViewInfo(_In_ ConversionAreaInfo* ConvAreaInfo, _In_ COORD coordConView);
void ConsoleImeWindowInfo(_In_ ConversionAreaInfo* ConvAreaInfo, _In_ SMALL_RECT rcViewCaWindow);
NTSTATUS ConsoleImeResizeScreenBuffer(_In_ PSCREEN_INFORMATION ScreenInfo, _In_ COORD NewScreenSize, _In_ ConversionAreaInfo* ConvAreaInfo);
bool InsertConvertedString(_In_ LPCWSTR lpStr);
void StreamWriteToScreenBufferIME(_In_reads_(StringLength) PWCHAR String,
                                  _In_ USHORT StringLength,
                                  _In_ PSCREEN_INFORMATION ScreenInfo,
                                  _In_reads_(StringLength) PCHAR StringA);

bool IsValidSmallRect(_In_ PSMALL_RECT const Rect)
{
    return (Rect->Right >= Rect->Left && Rect->Bottom >= Rect->Top);
}

void WriteConvRegionToScreen(_In_ const SCREEN_INFORMATION * const pScreenInfo,
                             _In_ const SMALL_RECT srConvRegion)
{
    CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    if (!pScreenInfo->IsActiveScreenBuffer())
    {
        return;
    }

    ConsoleImeInfo* const pIme = &gci.ConsoleIme;

    for (unsigned int i = 0; i < pIme->ConvAreaCompStr.size(); ++i)
    {
        ConversionAreaInfo* const pConvAreaInfo = pIme->ConvAreaCompStr[i];

        if (!pConvAreaInfo->IsHidden())
        {
            const SMALL_RECT currentViewport = pScreenInfo->GetBufferViewport();
            // Do clipping region
            SMALL_RECT Region;
            Region.Left = currentViewport.Left + pConvAreaInfo->CaInfo.rcViewCaWindow.Left + pConvAreaInfo->CaInfo.coordConView.X;
            Region.Right = Region.Left + (pConvAreaInfo->CaInfo.rcViewCaWindow.Right - pConvAreaInfo->CaInfo.rcViewCaWindow.Left);
            Region.Top = currentViewport.Top + pConvAreaInfo->CaInfo.rcViewCaWindow.Top + pConvAreaInfo->CaInfo.coordConView.Y;
            Region.Bottom = Region.Top + (pConvAreaInfo->CaInfo.rcViewCaWindow.Bottom - pConvAreaInfo->CaInfo.rcViewCaWindow.Top);

            SMALL_RECT ClippedRegion;
            ClippedRegion.Left = max(Region.Left, currentViewport.Left);
            ClippedRegion.Top = max(Region.Top, currentViewport.Top);
            ClippedRegion.Right = min(Region.Right, currentViewport.Right);
            ClippedRegion.Bottom = min(Region.Bottom, currentViewport.Bottom);

            if (IsValidSmallRect(&ClippedRegion))
            {
                Region = ClippedRegion;
                ClippedRegion.Left = max(Region.Left, srConvRegion.Left);
                ClippedRegion.Top = max(Region.Top, srConvRegion.Top);
                ClippedRegion.Right = min(Region.Right, srConvRegion.Right);
                ClippedRegion.Bottom = min(Region.Bottom, srConvRegion.Bottom);
                if (IsValidSmallRect(&ClippedRegion))
                {
                    // if we have a renderer, we need to update.
                    // we've already confirmed (above with an early return) that we're on conversion areas that are a part of the active (visible/rendered) screen
                    // so send invalidates to those regions such that we're queried for data on the next frame and repainted.
                    if (ServiceLocator::LocateGlobals().pRender != nullptr)
                    {
                        // convert inclusive rectangle to exclusive rectangle
                        SMALL_RECT srExclusive = ClippedRegion;
                        srExclusive.Right++;
                        srExclusive.Bottom++;

                        ServiceLocator::LocateGlobals().pRender->TriggerRedraw(&srExclusive);
                    }
                }
            }
        }
    }
}

#define LOCAL_BUFFER_SIZE 100
NTSTATUS WriteUndetermineChars(_In_reads_(NumChars) LPWSTR lpString, _In_ PBYTE lpAtr, _In_reads_(CONIME_ATTRCOLOR_SIZE) PWORD lpAtrIdx, _In_ DWORD NumChars)
{
    CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    ConsoleImeInfo* const ConsoleIme = &gci.ConsoleIme;
    PSCREEN_INFORMATION const ScreenInfo = gci.CurrentScreenBuffer;

    COORD Position = ScreenInfo->TextInfo->GetCursor()->GetPosition();
    COORD WindowOrigin;
    {
        const SMALL_RECT currentViewport = ScreenInfo->GetBufferViewport();

        if ((currentViewport.Left <= Position.X && Position.X <= currentViewport.Right) &&
            (currentViewport.Top <= Position.Y && Position.Y <= currentViewport.Bottom))
        {
            Position.X = ScreenInfo->TextInfo->GetCursor()->GetPosition().X - currentViewport.Left;
            Position.Y = ScreenInfo->TextInfo->GetCursor()->GetPosition().Y - currentViewport.Top;
        }
        else
        {
            WindowOrigin.X = 0;
            WindowOrigin.Y = (SHORT)(Position.Y - currentViewport.Bottom);
            ScreenInfo->SetViewportOrigin(FALSE, WindowOrigin);
        }
    }

    SHORT PosY = Position.Y;

    #pragma prefast(suppress:__WARNING_W2A_BEST_FIT, "WC_NO_BEST_FIT_CHARS doesn't work in many codepages. Retain old behavior.")
    ULONG NumStr = WideCharToMultiByte(CP_ACP,
                                       0,
                                       lpString,
                                       NumChars,
                                       nullptr,
                                       0,
                                       nullptr,
                                       nullptr);

    int const WholeLen = (int)Position.X + (int)NumStr;
    int const WholeRow = WholeLen / ScreenInfo->GetScreenWindowSizeX();

    if ((PosY + WholeRow) > (ScreenInfo->GetScreenWindowSizeY() - 1))
    {
        PosY = (SHORT)(ScreenInfo->GetScreenWindowSizeY() - 1 - WholeRow);
        if (PosY < 0)
        {
            PosY = ScreenInfo->GetBufferViewport().Top;
        }
    }

    BOOL UndetAreaUp = FALSE;
    if (PosY != Position.Y)
    {
        Position.Y = PosY;
        UndetAreaUp = TRUE;
    }

    DWORD ConvAreaIndex = 0;

    DWORD const BufferSize = NumChars;
    NumChars = 0;

    ConversionAreaInfo* ConvAreaInfo;
    for (ConvAreaIndex = 0; NumChars < BufferSize; ConvAreaIndex++)
    {
        if (ConvAreaIndex + 1 > ConsoleIme->ConvAreaCompStr.size())
        {
            NTSTATUS Status;

            Status = gci.ConsoleIme.AddConversionArea();
            if (!NT_SUCCESS(Status))
            {
                return Status;
            }
        }
        ConvAreaInfo = ConsoleIme->ConvAreaCompStr[ConvAreaIndex];
        PSCREEN_INFORMATION const ConvScreenInfo = ConvAreaInfo->ScreenBuffer;
        ConvScreenInfo->TextInfo->GetCursor()->SetXPosition(Position.X);

        if (ConvAreaInfo->IsHidden() || (UndetAreaUp))
        {
            // This conversion area need positioning onto cursor position.
            COORD CursorPosition;
            CursorPosition.X = 0;
            CursorPosition.Y = (SHORT)(Position.Y + ConvAreaIndex);
            ConsoleImeViewInfo(ConvAreaInfo, CursorPosition);
        }

        SMALL_RECT Region;
        Region.Left = ConvScreenInfo->TextInfo->GetCursor()->GetPosition().X;
        Region.Top = 0;
        Region.Bottom = 0;

        while (NumChars < BufferSize)
        {
            ULONG i = 0;
            WCHAR LocalBuffer[LOCAL_BUFFER_SIZE];
            BYTE LocalBufferA[LOCAL_BUFFER_SIZE];

            PWCHAR LocalBufPtr = LocalBuffer;
            PBYTE LocalBufPtrA = LocalBufferA;

            WCHAR Char = 0;
            WORD Attr = 0;
            while (NumChars < BufferSize && i < LOCAL_BUFFER_SIZE && Position.X < ScreenInfo->GetScreenWindowSizeX())
            {
                Char = *lpString;
    #pragma prefast(suppress:__WARNING_INCORRECT_ANNOTATION, "Precarious but this is internal-only code so we can live with it")
                Attr = *lpAtr;
                if (Char >= (WCHAR)' ')
                {
                    if (IsCharFullWidth(Char))
                    {
                        if (i < (LOCAL_BUFFER_SIZE - 1) && Position.X < ScreenInfo->GetScreenWindowSizeX() - 1)
                        {
                            *LocalBufPtr++ = Char;
                            *LocalBufPtrA++ = CHAR_ROW::ATTR_LEADING_BYTE;
                            *LocalBufPtr++ = Char;
                            *LocalBufPtrA++ = CHAR_ROW::ATTR_TRAILING_BYTE;
                            Position.X += 2;
                            i += 2;
                        }
                        else
                        {
                            Position.X++;
                            break;
                        }
                    }
                    else
                    {
                        *LocalBufPtr++ = Char;
                        *LocalBufPtrA++ = 0;
                        Position.X++;
                        i++;
                    }
                }
                lpString++;
                lpAtr++;
                NumChars++;

                if (NumChars < BufferSize && Attr != *lpAtr)
                {
                    break;
                }
            }

            if (i != 0)
            {
                WORD wLegacyAttr = lpAtrIdx[Attr & 0x07];
                if (Attr & 0x10)
                {
                    wLegacyAttr |= (COMMON_LVB_GRID_SINGLEFLAG | COMMON_LVB_GRID_RVERTICAL);
                }
                else if (Attr & 0x20)
                {
                    wLegacyAttr |= (COMMON_LVB_GRID_SINGLEFLAG | COMMON_LVB_GRID_LVERTICAL);
                }
                TextAttribute taAttribute = TextAttribute(wLegacyAttr);
                ConvScreenInfo->SetAttributes(taAttribute);

                StreamWriteToScreenBufferIME(LocalBuffer, (USHORT) i, ConvScreenInfo, (PCHAR) LocalBufferA);

                ConvScreenInfo->TextInfo->GetCursor()->IncrementXPosition(i);

                if (NumChars == BufferSize ||
                    Position.X >= ScreenInfo->GetScreenWindowSizeX() ||
                    ((Char >= (WCHAR)' ' &&
                      IsCharFullWidth(Char) &&
                      Position.X >= ScreenInfo->GetScreenWindowSizeX() - 1)))
                {

                    Region.Right = (SHORT)(ConvScreenInfo->TextInfo->GetCursor()->GetPosition().X - 1);
                    ConsoleImeWindowInfo(ConvAreaInfo, Region);

                    ConvAreaInfo->SetHidden(false);

                    ConsoleImePaint(ConvAreaInfo);

                    Position.X = 0;
                    break;
                }

                if (NumChars == BufferSize)
                {
                    return STATUS_SUCCESS;
                }
                continue;

            }
            else if (NumChars == BufferSize)
            {
                return STATUS_SUCCESS;
            }

            if (Position.X >= ScreenInfo->GetScreenWindowSizeX())
            {
                Position.X = 0;
                break;
            }
        }

    }

    for (; ConvAreaIndex < ConsoleIme->ConvAreaCompStr.size(); ConvAreaIndex++)
    {
        ConvAreaInfo = ConsoleIme->ConvAreaCompStr[ConvAreaIndex];
        if (!ConvAreaInfo->IsHidden())
        {
            ConvAreaInfo->SetHidden(true);
            ConsoleImePaint(ConvAreaInfo);
        }
    }

    return STATUS_SUCCESS;
}

NTSTATUS FillUndetermineChars(_In_ ConversionAreaInfo* ConvAreaInfo)
{
    const CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    ConvAreaInfo->SetHidden(true);

    COORD Coord = { 0 };
    DWORD CharsToWrite = ConvAreaInfo->ScreenBuffer->GetScreenBufferSize().X;

    FillOutput(ConvAreaInfo->ScreenBuffer, (WCHAR)' ', Coord, CONSOLE_FALSE_UNICODE,    // faster than real unicode
               &CharsToWrite);

    CharsToWrite = ConvAreaInfo->ScreenBuffer->GetScreenBufferSize().X;
    FillOutput(ConvAreaInfo->ScreenBuffer,
               gci.CurrentScreenBuffer->GetAttributes().GetLegacyAttributes(),
               Coord,
               CONSOLE_ATTRIBUTE,
               &CharsToWrite);
    ConsoleImePaint(ConvAreaInfo);
    return STATUS_SUCCESS;
}


NTSTATUS ConsoleImeCompStr(_In_ LPCONIME_UICOMPMESSAGE CompStr)
{
    CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    ConsoleImeInfo* const pIme = &gci.ConsoleIme;

    if (CompStr->dwCompStrLen == 0 || CompStr->dwResultStrLen != 0)
    {
        // Cursor turn ON.
        if (pIme->SavedCursorVisible)
        {
            pIme->SavedCursorVisible = FALSE;
            gci.CurrentScreenBuffer
                ->SetCursorInformation(gci.CurrentScreenBuffer->TextInfo->GetCursor()->GetSize(),
                                       TRUE);
        }

        // Determine string.
        for (unsigned int i = 0; i < pIme->ConvAreaCompStr.size(); ++i)
        {
            ConversionAreaInfo* const ConvAreaInfo = pIme->ConvAreaCompStr[i];
            if (ConvAreaInfo && !ConvAreaInfo->IsHidden())
            {
                FillUndetermineChars(ConvAreaInfo);
            }
        }

        if (CompStr->dwResultStrLen != 0)
        {
            #pragma prefast(suppress:26035, "CONIME_UICOMPMESSAGE structure impossible for PREfast to trace due to its structure.")
            if (!InsertConvertedString((LPCWSTR) ((PBYTE) CompStr + CompStr->dwResultStrOffset)))
            {
                return STATUS_INVALID_HANDLE;
            }
        }

        if (pIme->CompStrData)
        {
            delete[] pIme->CompStrData;
            pIme->CompStrData = nullptr;
        }
    }
    else
    {
        LPWSTR lpStr;
        PBYTE lpAtr;
        PWORD lpAtrIdx;

        // Cursor turn OFF.
        if (gci.CurrentScreenBuffer->TextInfo->GetCursor()->IsVisible())
        {
            pIme->SavedCursorVisible = TRUE;
            gci.CurrentScreenBuffer
                ->SetCursorInformation(gci.CurrentScreenBuffer->TextInfo->GetCursor()->GetSize(),
                                       FALSE);
        }

        // Composition string.
        for (unsigned int i = 0; i < pIme->ConvAreaCompStr.size(); ++i)
        {
            ConversionAreaInfo* const ConvAreaInfo = pIme->ConvAreaCompStr[i];
            if (ConvAreaInfo && !ConvAreaInfo->IsHidden())
            {
                FillUndetermineChars(ConvAreaInfo);
            }
        }

        lpStr = (LPWSTR) ((PBYTE) CompStr + CompStr->dwCompStrOffset);
        lpAtr = (PBYTE) CompStr + CompStr->dwCompAttrOffset;
        lpAtrIdx = (PWORD) CompStr->CompAttrColor;
        #pragma prefast(suppress:26035, "CONIME_UICOMPMESSAGE structure impossible for PREfast to trace due to its structure.")
        WriteUndetermineChars(lpStr, lpAtr, lpAtrIdx, CompStr->dwCompStrLen / sizeof(WCHAR));
    }

    return STATUS_SUCCESS;
}

NTSTATUS ConsoleImeResizeCompStrView()
{
    CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    ConsoleImeInfo* const pIme = &gci.ConsoleIme;

    // Compositon string
    LPCONIME_UICOMPMESSAGE const CompStr = pIme->CompStrData;
    if (CompStr)
    {
        for (unsigned int i = 0; i < pIme->ConvAreaCompStr.size(); ++i)
        {
            ConversionAreaInfo* const ConvAreaInfo = pIme->ConvAreaCompStr[i];
            if (ConvAreaInfo && !ConvAreaInfo->IsHidden())
            {
                FillUndetermineChars(ConvAreaInfo);
            }
        }

        LPWSTR lpStr = (LPWSTR) ((PBYTE) CompStr + CompStr->dwCompStrOffset);
        PBYTE lpAtr = (PBYTE) CompStr + CompStr->dwCompAttrOffset;
        PWORD lpAtrIdx = (PWORD) CompStr->CompAttrColor;

        WriteUndetermineChars(lpStr, lpAtr, lpAtrIdx, CompStr->dwCompStrLen / sizeof(WCHAR));
    }

    return STATUS_SUCCESS;
}

NTSTATUS ConsoleImeResizeCompStrScreenBuffer(_In_ COORD const coordNewScreenSize)
{
    CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    ConsoleImeInfo* const pIme = &gci.ConsoleIme;

    // Composition string
    for (unsigned int i = 0; i < pIme->ConvAreaCompStr.size(); ++i)
    {
        ConversionAreaInfo* const ConvAreaInfo = pIme->ConvAreaCompStr[i];

        if (ConvAreaInfo)
        {
            if (!ConvAreaInfo->IsHidden())
            {
                ConvAreaInfo->SetHidden(true);
                ConsoleImePaint(ConvAreaInfo);
            }

            NTSTATUS Status = ConsoleImeResizeScreenBuffer(ConvAreaInfo->ScreenBuffer, coordNewScreenSize, ConvAreaInfo);
            if (!NT_SUCCESS(Status))
            {
                return Status;
            }
        }

    }

    return STATUS_SUCCESS;
}

SHORT CalcWideCharToColumn(_In_reads_(NumberOfChars) PCHAR_INFO Buffer, _In_ size_t NumberOfChars)
{
    SHORT Column = 0;

    while (NumberOfChars--)
    {
        if (IsCharFullWidth(Buffer->Char.UnicodeChar))
        {
            Column += 2;
        }
        else
        {
            Column++;
        }

        Buffer++;
    }

    return Column;
}


void ConsoleImePaint(_In_ ConversionAreaInfo* pConvAreaInfo)
{
    const CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    if (pConvAreaInfo == nullptr)
    {
        return;
    }

    PSCREEN_INFORMATION const ScreenInfo = gci.CurrentScreenBuffer;
    if (ScreenInfo == nullptr)
    {
        return;
    }

    SMALL_RECT WriteRegion;
    WriteRegion.Left = ScreenInfo->GetBufferViewport().Left + pConvAreaInfo->CaInfo.coordConView.X + pConvAreaInfo->CaInfo.rcViewCaWindow.Left;
    WriteRegion.Right = WriteRegion.Left + (pConvAreaInfo->CaInfo.rcViewCaWindow.Right - pConvAreaInfo->CaInfo.rcViewCaWindow.Left);
    WriteRegion.Top = ScreenInfo->GetBufferViewport().Top + pConvAreaInfo->CaInfo.coordConView.Y + pConvAreaInfo->CaInfo.rcViewCaWindow.Top;
    WriteRegion.Bottom = WriteRegion.Top + (pConvAreaInfo->CaInfo.rcViewCaWindow.Bottom - pConvAreaInfo->CaInfo.rcViewCaWindow.Top);

    if (!pConvAreaInfo->IsHidden())
    {
        WriteConvRegionToScreen(ScreenInfo, WriteRegion);
    }
    else
    {
        WriteToScreen(ScreenInfo, WriteRegion);
    }
}

void ConsoleImeViewInfo(_In_ ConversionAreaInfo* ConvAreaInfo, _In_ COORD coordConView)
{
    const CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();

    if (ConvAreaInfo->IsHidden())
    {
        SMALL_RECT NewRegion;
        ConvAreaInfo->CaInfo.coordConView = coordConView;
        NewRegion = ConvAreaInfo->CaInfo.rcViewCaWindow;
        NewRegion.Left += ConvAreaInfo->CaInfo.coordConView.X;
        NewRegion.Right += ConvAreaInfo->CaInfo.coordConView.X;
        NewRegion.Top += ConvAreaInfo->CaInfo.coordConView.Y;
        NewRegion.Bottom += ConvAreaInfo->CaInfo.coordConView.Y;
    }
    else
    {
        SMALL_RECT OldRegion, NewRegion;
        OldRegion = ConvAreaInfo->CaInfo.rcViewCaWindow;
        OldRegion.Left += ConvAreaInfo->CaInfo.coordConView.X;
        OldRegion.Right += ConvAreaInfo->CaInfo.coordConView.X;
        OldRegion.Top += ConvAreaInfo->CaInfo.coordConView.Y;
        OldRegion.Bottom += ConvAreaInfo->CaInfo.coordConView.Y;
        ConvAreaInfo->CaInfo.coordConView = coordConView;

        WriteToScreen(gci.CurrentScreenBuffer, OldRegion);

        NewRegion = ConvAreaInfo->CaInfo.rcViewCaWindow;
        NewRegion.Left += ConvAreaInfo->CaInfo.coordConView.X;
        NewRegion.Right += ConvAreaInfo->CaInfo.coordConView.X;
        NewRegion.Top += ConvAreaInfo->CaInfo.coordConView.Y;
        NewRegion.Bottom += ConvAreaInfo->CaInfo.coordConView.Y;
        WriteToScreen(gci.CurrentScreenBuffer, NewRegion);
    }
}

void ConsoleImeWindowInfo(_In_ ConversionAreaInfo* ConvAreaInfo, _In_ SMALL_RECT rcViewCaWindow)
{
    if (rcViewCaWindow.Left != ConvAreaInfo->CaInfo.rcViewCaWindow.Left ||
        rcViewCaWindow.Top != ConvAreaInfo->CaInfo.rcViewCaWindow.Top ||
        rcViewCaWindow.Right != ConvAreaInfo->CaInfo.rcViewCaWindow.Right ||
        rcViewCaWindow.Bottom != ConvAreaInfo->CaInfo.rcViewCaWindow.Bottom)
    {
        if (!ConvAreaInfo->IsHidden())
        {
            ConvAreaInfo->SetHidden(true);
            ConsoleImePaint(ConvAreaInfo);

            ConvAreaInfo->CaInfo.rcViewCaWindow = rcViewCaWindow;
            ConvAreaInfo->SetHidden(false);
            ConsoleImePaint(ConvAreaInfo);
        }
        else
        {
            ConvAreaInfo->CaInfo.rcViewCaWindow = rcViewCaWindow;
        }
    }
}

NTSTATUS ConsoleImeResizeScreenBuffer(_In_ PSCREEN_INFORMATION ScreenInfo, _In_ COORD NewScreenSize, _In_ ConversionAreaInfo* ConvAreaInfo)
{
    NTSTATUS Status = ScreenInfo->ResizeScreenBuffer(NewScreenSize, FALSE);
    if (NT_SUCCESS(Status))
    {
        ConvAreaInfo->CaInfo.coordCaBuffer = NewScreenSize;
        if (ConvAreaInfo->CaInfo.rcViewCaWindow.Left > NewScreenSize.X - 1)
        {
            ConvAreaInfo->CaInfo.rcViewCaWindow.Left = NewScreenSize.X - 1;
        }

        if (ConvAreaInfo->CaInfo.rcViewCaWindow.Right > NewScreenSize.X - 1)
        {
            ConvAreaInfo->CaInfo.rcViewCaWindow.Right = NewScreenSize.X - 1;
        }

        if (ConvAreaInfo->CaInfo.rcViewCaWindow.Top > NewScreenSize.Y - 1)
        {
            ConvAreaInfo->CaInfo.rcViewCaWindow.Top = NewScreenSize.Y - 1;
        }

        if (ConvAreaInfo->CaInfo.rcViewCaWindow.Bottom > NewScreenSize.Y - 1)
        {
            ConvAreaInfo->CaInfo.rcViewCaWindow.Bottom = NewScreenSize.Y - 1;
        }
    }

    return Status;
}

// Routine Description:
// - This routine handle WM_COPYDATA message.
// Arguments:
// - Console - Pointer to console information structure.
// - wParam -
// - lParam -
// Return Value:
NTSTATUS ImeControl(_In_ PCOPYDATASTRUCT pCopyDataStruct)
{
    CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    if (pCopyDataStruct == nullptr)
    {
        // fail safe.
        return STATUS_SUCCESS;
    }

    switch ((LONG) pCopyDataStruct->dwData)
    {
        case CI_CONIMECOMPOSITION:
            if (pCopyDataStruct->cbData >= sizeof(CONIME_UICOMPMESSAGE))
            {
                LPCONIME_UICOMPMESSAGE CompStr;

                CompStr = (LPCONIME_UICOMPMESSAGE) pCopyDataStruct->lpData;
                if (CompStr && CompStr->dwSize == pCopyDataStruct->cbData)
                {
                    if (gci.ConsoleIme.CompStrData)
                    {
                        delete[] gci.ConsoleIme.CompStrData;
                    }

                    gci.ConsoleIme.CompStrData = (LPCONIME_UICOMPMESSAGE) new BYTE[CompStr->dwSize];
                    if (gci.ConsoleIme.CompStrData == nullptr)
                    {
                        break;
                    }

                    memmove(gci.ConsoleIme.CompStrData, CompStr, CompStr->dwSize);
                    ConsoleImeCompStr(gci.ConsoleIme.CompStrData);
                }
            }
            break;
        case CI_ONSTARTCOMPOSITION:
            gci.pInputBuffer->fInComposition = true;
            break;
        case CI_ONENDCOMPOSITION:
            gci.pInputBuffer->fInComposition = false;
            break;
    }

    return STATUS_SUCCESS;
}

bool InsertConvertedString(_In_ LPCWSTR lpStr)
{
    const CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    bool fResult = false;

    if (gci.CurrentScreenBuffer->TextInfo->GetCursor()->IsOn())
    {
        gci.CurrentScreenBuffer->TextInfo->GetCursor()
            ->TimerRoutine(gci.CurrentScreenBuffer);
    }

    const DWORD dwControlKeyState = GetControlKeyState(0);
    try
    {
        std::deque<std::unique_ptr<IInputEvent>> inEvents;
        KeyEvent keyEvent{ TRUE, // keydown
                           1, // repeatCount
                           0, // virtualKeyCode
                           0, // virtualScanCode
                           0, // charData
                           dwControlKeyState }; // activeModifierKeys

        while (*lpStr)
        {
            keyEvent.SetCharData(*lpStr);
            inEvents.push_back(std::make_unique<KeyEvent>(keyEvent));

            ++lpStr;
        }

        gci.pInputBuffer->Write(inEvents);

        fResult = true;
    }
    catch (...)
    {
        LOG_HR(wil::ResultFromCaughtException());
    }

    return fResult;
}

void StreamWriteToScreenBufferIME(_In_reads_(StringLength) PWCHAR String,
                                  _In_ USHORT StringLength,
                                  _In_ PSCREEN_INFORMATION ScreenInfo,
                                  _In_reads_(StringLength) PCHAR StringA)
{
    DBGOUTPUT(("StreamWriteToScreenBuffer\n"));

    COORD TargetPoint = ScreenInfo->TextInfo->GetCursor()->GetPosition();

    ROW* const Row = ScreenInfo->TextInfo->GetRowByOffset(TargetPoint.Y);
    ASSERT(Row != nullptr);
    DBGOUTPUT(("Row = 0x%p, TargetPoint = (0x%x,0x%x)\n", Row, TargetPoint.X, TargetPoint.Y));

    // copy chars
    BisectWrite(StringLength, TargetPoint, ScreenInfo);

    const COORD coordScreenBufferSize = ScreenInfo->GetScreenBufferSize();

    USHORT ScreenEndOfString;
    if (SUCCEEDED(UShortSub(coordScreenBufferSize.X, TargetPoint.X, &ScreenEndOfString)) &&
        ScreenEndOfString &&
        StringLength > ScreenEndOfString)
    {

        if (TargetPoint.Y == coordScreenBufferSize.Y - 1 &&
            TargetPoint.X + StringLength >= coordScreenBufferSize.X && *(StringA + ScreenEndOfString - 1) & CHAR_ROW::ATTR_LEADING_BYTE)
        {
            *(String + ScreenEndOfString - 1) = UNICODE_SPACE;
            *(StringA + ScreenEndOfString - 1) = 0;
            if (StringLength > ScreenEndOfString - 1)
            {
                *(String + ScreenEndOfString) = UNICODE_SPACE;
                *(StringA + ScreenEndOfString) = 0;
            }
        }
    }
    memmove(&Row->CharRow.Chars[TargetPoint.X], String, StringLength * sizeof(WCHAR));
    memmove(&Row->CharRow.KAttrs[TargetPoint.X], StringA, StringLength * sizeof(CHAR));

    // recalculate first and last non-space char
    if (TargetPoint.X < Row->CharRow.Left)
    {
        // CharRow.Left is leftmost bound of chars in Chars array (array will be full width) i.e. type is COORD
        PWCHAR LastChar = &Row->CharRow.Chars[coordScreenBufferSize.X - 1];
        PWCHAR Char;

        for (Char = &Row->CharRow.Chars[TargetPoint.X]; Char < LastChar && *Char == (WCHAR)' '; Char++)
        {
            /* do nothing */ ;
        }
        Row->CharRow.Left = (SHORT)(Char - Row->CharRow.Chars);
    }

    if ((TargetPoint.X + StringLength) >= Row->CharRow.Right)
    {
        PWCHAR FirstChar = Row->CharRow.Chars;
        PWCHAR Char;

        for (Char = &Row->CharRow.Chars[TargetPoint.X + StringLength - 1]; *Char == (WCHAR)' ' && Char >= FirstChar; Char--)
        {
            /* do nothing */ ;
        }

        Row->CharRow.Right = (SHORT)(Char + 1 - FirstChar);
    }

    // see if attr string is different.  if so, allocate a new attr buffer and merge the two strings.
    TextAttributeRun* pExistingHead;
    Row->AttrRow.FindAttrIndex(0, &pExistingHead, nullptr);

    if (Row->AttrRow._cList != 1 || !(pExistingHead->GetAttributes().IsEqual(ScreenInfo->GetAttributes())))
    {
        TextAttributeRun InsertedRun;

        const WORD wScreenAttributes = ScreenInfo->GetAttributes().GetLegacyAttributes();
        const bool fRVerticalSet = AreAllFlagsSet(wScreenAttributes, COMMON_LVB_GRID_SINGLEFLAG | COMMON_LVB_GRID_RVERTICAL);
        const bool fLVerticalSet = AreAllFlagsSet(wScreenAttributes, COMMON_LVB_GRID_SINGLEFLAG | COMMON_LVB_GRID_LVERTICAL);

        if (fLVerticalSet || fRVerticalSet)
        {
            const byte LeadOrTrailByte = fRVerticalSet? CHAR_ROW::ATTR_LEADING_BYTE : CHAR_ROW::ATTR_TRAILING_BYTE;
            const int iFlag = fRVerticalSet? COMMON_LVB_GRID_RVERTICAL : COMMON_LVB_GRID_LVERTICAL;
            for (short i = 0; i < StringLength; i++)
            {
                InsertedRun.SetLength(1);
                if (*(StringA + i) & LeadOrTrailByte)
                {
                    InsertedRun.SetAttributesFromLegacy(wScreenAttributes & ~(COMMON_LVB_GRID_SINGLEFLAG | iFlag));
                }
                else
                {
                    InsertedRun.SetAttributesFromLegacy(wScreenAttributes & ~COMMON_LVB_GRID_SINGLEFLAG);
                }

                // Each time around the loop, take our new 1-length attribute with the appropriate line attributes (underlines, etc.)
                // and insert it into the existing Run-Length-Encoded attribute list.
                Row->AttrRow.InsertAttrRuns(&InsertedRun,
                                            1,
                                            TargetPoint.X + i,
                                            (SHORT)(TargetPoint.X + i + 1),
                                            coordScreenBufferSize.X);
            }
        }
        else
        {
            InsertedRun.SetLength(StringLength);
            InsertedRun.SetAttributesFromLegacy(wScreenAttributes);
            Row->AttrRow.InsertAttrRuns(&InsertedRun,
                                        1,
                                        TargetPoint.X,
                                        (SHORT)(TargetPoint.X + StringLength - 1),
                                        coordScreenBufferSize.X);
        }
    }

    ScreenInfo->ResetTextFlags(TargetPoint.X, TargetPoint.Y, TargetPoint.X + StringLength - 1, TargetPoint.Y);
}
