/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "_output.h"

#include "dbcs.h"
#include "Ucs2CharRow.hpp"

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
                                  _In_ const size_t StringLength,
                                  _In_ PSCREEN_INFORMATION ScreenInfo,
                                  _In_reads_(StringLength) DbcsAttribute* const pDbcsAttributes);

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
        const std::unique_ptr<ConversionAreaInfo>& ConvAreaInfo = pIme->ConvAreaCompStr[i];

        if (!ConvAreaInfo->IsHidden())
        {
            const SMALL_RECT currentViewport = pScreenInfo->GetBufferViewport();
            // Do clipping region
            SMALL_RECT Region;
            Region.Left = currentViewport.Left + ConvAreaInfo->CaInfo.rcViewCaWindow.Left + ConvAreaInfo->CaInfo.coordConView.X;
            Region.Right = Region.Left + (ConvAreaInfo->CaInfo.rcViewCaWindow.Right - ConvAreaInfo->CaInfo.rcViewCaWindow.Left);
            Region.Top = currentViewport.Top + ConvAreaInfo->CaInfo.rcViewCaWindow.Top + ConvAreaInfo->CaInfo.coordConView.Y;
            Region.Bottom = Region.Top + (ConvAreaInfo->CaInfo.rcViewCaWindow.Bottom - ConvAreaInfo->CaInfo.rcViewCaWindow.Top);

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
[[nodiscard]]
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
            LOG_IF_FAILED(ScreenInfo->SetViewportOrigin(FALSE, WindowOrigin));
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
        ConvAreaInfo = ConsoleIme->ConvAreaCompStr[ConvAreaIndex].get();
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

            size_t currentBufferIndex = 0;
            WCHAR LocalBuffer[LOCAL_BUFFER_SIZE];
            DbcsAttribute dbcsAttributes[LOCAL_BUFFER_SIZE];

            WCHAR Char = 0;
            WORD Attr = 0;
            while (NumChars < BufferSize &&
                   currentBufferIndex < LOCAL_BUFFER_SIZE &&
                   Position.X < ScreenInfo->GetScreenWindowSizeX())
            {
                Char = *lpString;
    #pragma prefast(suppress:__WARNING_INCORRECT_ANNOTATION, "Precarious but this is internal-only code so we can live with it")
                Attr = *lpAtr;
                if (Char >= (WCHAR)' ')
                {
                    if (IsCharFullWidth(Char))
                    {
                        if (currentBufferIndex < (LOCAL_BUFFER_SIZE - 1)
                            && Position.X < ScreenInfo->GetScreenWindowSizeX() - 1)
                        {
                            // leading byte
                            LocalBuffer[currentBufferIndex] = Char;
                            dbcsAttributes[currentBufferIndex].SetLeading();
                            ++currentBufferIndex;
                            // trailing byte
                            LocalBuffer[currentBufferIndex] = Char;
                            dbcsAttributes[currentBufferIndex].SetTrailing();
                            ++currentBufferIndex;

                            Position.X += 2;
                        }
                        else
                        {
                            Position.X++;
                            break;
                        }
                    }
                    else
                    {
                        LocalBuffer[currentBufferIndex] = Char;
                        dbcsAttributes[currentBufferIndex].SetSingle();
                        ++currentBufferIndex;
                        Position.X++;
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

            if (currentBufferIndex != 0)
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

                StreamWriteToScreenBufferIME(LocalBuffer, currentBufferIndex, ConvScreenInfo, dbcsAttributes);

                ConvScreenInfo->TextInfo->GetCursor()->IncrementXPosition(static_cast<int>(currentBufferIndex));

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
        ConvAreaInfo = ConsoleIme->ConvAreaCompStr[ConvAreaIndex].get();
        if (!ConvAreaInfo->IsHidden())
        {
            ConvAreaInfo->SetHidden(true);
            ConsoleImePaint(ConvAreaInfo);
        }
    }

    return STATUS_SUCCESS;
}

[[nodiscard]]
NTSTATUS FillUndetermineChars(_In_ ConversionAreaInfo* const ConvAreaInfo)
{
    const CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    ConvAreaInfo->SetHidden(true);

    COORD Coord = { 0 };
    DWORD CharsToWrite = ConvAreaInfo->ScreenBuffer->GetScreenBufferSize().X;

    LOG_IF_FAILED(FillOutput(ConvAreaInfo->ScreenBuffer,
                             (WCHAR)' ',
                             Coord,
                             CONSOLE_FALSE_UNICODE,    // faster than real unicode
                             &CharsToWrite));

    CharsToWrite = ConvAreaInfo->ScreenBuffer->GetScreenBufferSize().X;
    LOG_IF_FAILED(FillOutput(ConvAreaInfo->ScreenBuffer,
                             gci.CurrentScreenBuffer->GetAttributes().GetLegacyAttributes(),
                             Coord,
                             CONSOLE_ATTRIBUTE,
                             &CharsToWrite));
    ConsoleImePaint(ConvAreaInfo);
    return STATUS_SUCCESS;
}


[[nodiscard]]
NTSTATUS ConsoleImeCompStr(_In_ LPCONIME_UICOMPMESSAGE CompStr)
{
    CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    Cursor* pCursor = gci.CurrentScreenBuffer->TextInfo->GetCursor();
    ConsoleImeInfo* const pIme = &gci.ConsoleIme;

    if (CompStr->dwCompStrLen == 0 || CompStr->dwResultStrLen != 0)
    {
        // Cursor turn ON.
        if (pIme->SavedCursorVisible)
        {
            pIme->SavedCursorVisible = FALSE;

            gci.CurrentScreenBuffer->SetCursorInformation(
                pCursor->GetSize(),
                TRUE,
                pCursor->GetColor(),
                pCursor->GetType()
            );

        }

        // Determine string.
        for (unsigned int i = 0; i < pIme->ConvAreaCompStr.size(); ++i)
        {
            const std::unique_ptr<ConversionAreaInfo>& ConvAreaInfo = pIme->ConvAreaCompStr[i];
            if (ConvAreaInfo.get() && !ConvAreaInfo->IsHidden())
            {
                LOG_IF_FAILED(FillUndetermineChars(ConvAreaInfo.get()));
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
        if (pCursor->IsVisible())
        {
            pIme->SavedCursorVisible = TRUE;

            gci.CurrentScreenBuffer->SetCursorInformation(
                pCursor->GetSize(),
                FALSE,
                pCursor->GetColor(),
                pCursor->GetType()
            );

        }

        // Composition string.
        for (unsigned int i = 0; i < pIme->ConvAreaCompStr.size(); ++i)
        {
            const std::unique_ptr<ConversionAreaInfo>& ConvAreaInfo = pIme->ConvAreaCompStr[i];
            if (ConvAreaInfo.get() && !ConvAreaInfo->IsHidden())
            {
                LOG_IF_FAILED(FillUndetermineChars(ConvAreaInfo.get()));
            }
        }

        lpStr = (LPWSTR) ((PBYTE) CompStr + CompStr->dwCompStrOffset);
        lpAtr = (PBYTE) CompStr + CompStr->dwCompAttrOffset;
        lpAtrIdx = (PWORD) CompStr->CompAttrColor;
        #pragma prefast(suppress:26035, "CONIME_UICOMPMESSAGE structure impossible for PREfast to trace due to its structure.")
        LOG_IF_FAILED(WriteUndetermineChars(lpStr, lpAtr, lpAtrIdx, CompStr->dwCompStrLen / sizeof(WCHAR)));
    }

    return STATUS_SUCCESS;
}

[[nodiscard]]
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
            const std::unique_ptr<ConversionAreaInfo>& ConvAreaInfo = pIme->ConvAreaCompStr[i];
            if (ConvAreaInfo.get() && !ConvAreaInfo->IsHidden())
            {
                LOG_IF_FAILED(FillUndetermineChars(ConvAreaInfo.get()));
            }
        }

        LPWSTR lpStr = (LPWSTR) ((PBYTE) CompStr + CompStr->dwCompStrOffset);
        PBYTE lpAtr = (PBYTE) CompStr + CompStr->dwCompAttrOffset;
        PWORD lpAtrIdx = (PWORD) CompStr->CompAttrColor;

        LOG_IF_FAILED(WriteUndetermineChars(lpStr, lpAtr, lpAtrIdx, CompStr->dwCompStrLen / sizeof(WCHAR)));
    }

    return STATUS_SUCCESS;
}

[[nodiscard]]
NTSTATUS ConsoleImeResizeCompStrScreenBuffer(_In_ COORD const coordNewScreenSize)
{
    CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    ConsoleImeInfo* const pIme = &gci.ConsoleIme;

    // Composition string
    for (unsigned int i = 0; i < pIme->ConvAreaCompStr.size(); ++i)
    {
        std::unique_ptr<ConversionAreaInfo>& ConvAreaInfo = pIme->ConvAreaCompStr[i];

        if (ConvAreaInfo.get())
        {
            if (!ConvAreaInfo->IsHidden())
            {
                ConvAreaInfo->SetHidden(true);
                ConsoleImePaint(ConvAreaInfo.get());
            }

            NTSTATUS Status = ConsoleImeResizeScreenBuffer(ConvAreaInfo->ScreenBuffer, coordNewScreenSize, ConvAreaInfo.get());
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


void ConsoleImePaint(_In_ const ConversionAreaInfo* const pConvAreaInfo)
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

void ConsoleImeViewInfo(_Inout_ ConversionAreaInfo* const ConvAreaInfo, _In_ COORD coordConView)
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

void ConsoleImeWindowInfo(_Inout_ ConversionAreaInfo* const ConvAreaInfo, _In_ SMALL_RECT rcViewCaWindow)
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

[[nodiscard]]
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
[[nodiscard]]
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
                    LOG_IF_FAILED(ConsoleImeCompStr(gci.ConsoleIme.CompStrData));
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
                                  _In_ const size_t StringLength,
                                  _In_ PSCREEN_INFORMATION ScreenInfo,
                                  _In_reads_(StringLength) DbcsAttribute* const pDbcsAttributes)
{
    DBGOUTPUT(("StreamWriteToScreenBuffer\n"));

    COORD TargetPoint = ScreenInfo->TextInfo->GetCursor()->GetPosition();

    ROW& Row = ScreenInfo->TextInfo->GetRowByOffset(TargetPoint.Y);
    DBGOUTPUT(("&Row = 0x%p, TargetPoint = (0x%x,0x%x)\n", &Row, TargetPoint.X, TargetPoint.Y));

    // copy chars
    CleanupDbcsEdgesForWrite(StringLength, TargetPoint, ScreenInfo);

    const COORD coordScreenBufferSize = ScreenInfo->GetScreenBufferSize();

    USHORT ScreenEndOfString;
    if (SUCCEEDED(UShortSub(coordScreenBufferSize.X, TargetPoint.X, &ScreenEndOfString)) &&
        ScreenEndOfString &&
        StringLength > ScreenEndOfString)
    {

        if (TargetPoint.Y == coordScreenBufferSize.Y - 1 &&
            TargetPoint.X + static_cast<USHORT>(StringLength) >= coordScreenBufferSize.X &&
            pDbcsAttributes[ScreenEndOfString - 1].IsLeading())
        {
            *(String + ScreenEndOfString - 1) = UNICODE_SPACE;
            pDbcsAttributes[ScreenEndOfString - 1].SetSingle();
            if (StringLength > static_cast<size_t>(ScreenEndOfString) - 1)
            {
                *(String + ScreenEndOfString) = UNICODE_SPACE;
                pDbcsAttributes[ScreenEndOfString].SetSingle();
            }
        }
    }

    try
    {
        ICharRow& iCharRow = Row.GetCharRow();
        // we only support ucs2 encoded char rows
        FAIL_FAST_IF_MSG(iCharRow.GetSupportedEncoding() != ICharRow::SupportedEncoding::Ucs2,
                        "only support UCS2 char rows currently");

        Ucs2CharRow& charRow = static_cast<Ucs2CharRow&>(iCharRow);
        OverwriteColumns(String,
                        String + StringLength,
                        pDbcsAttributes,
                        std::next(charRow.begin(), TargetPoint.X));
    }
    CATCH_LOG();

    // see if attr string is different.  if so, allocate a new attr buffer and merge the two strings.
    TextAttributeRun* pExistingHead;
    Row.GetAttrRow().FindAttrIndex(0, &pExistingHead, nullptr);

    if (Row.GetAttrRow()._cList != 1 || !(pExistingHead->GetAttributes().IsEqual(ScreenInfo->GetAttributes())))
    {
        TextAttributeRun InsertedRun;

        const WORD wScreenAttributes = ScreenInfo->GetAttributes().GetLegacyAttributes();
        const bool fRVerticalSet = AreAllFlagsSet(wScreenAttributes, COMMON_LVB_GRID_SINGLEFLAG | COMMON_LVB_GRID_RVERTICAL);
        const bool fLVerticalSet = AreAllFlagsSet(wScreenAttributes, COMMON_LVB_GRID_SINGLEFLAG | COMMON_LVB_GRID_LVERTICAL);

        if (fLVerticalSet || fRVerticalSet)
        {
            const int iFlag = fRVerticalSet? COMMON_LVB_GRID_RVERTICAL : COMMON_LVB_GRID_LVERTICAL;
            for (size_t i = 0; i < StringLength; i++)
            {
                InsertedRun.SetLength(1);
                const bool IsLeadingOrTrailing = fRVerticalSet ? pDbcsAttributes[i].IsLeading() : pDbcsAttributes[i].IsTrailing();
                if (IsLeadingOrTrailing)
                {
                    InsertedRun.SetAttributesFromLegacy(wScreenAttributes & ~(COMMON_LVB_GRID_SINGLEFLAG | iFlag));
                }
                else
                {
                    InsertedRun.SetAttributesFromLegacy(wScreenAttributes & ~COMMON_LVB_GRID_SINGLEFLAG);
                }

                // Each time around the loop, take our new 1-length attribute with the appropriate line attributes (underlines, etc.)
                // and insert it into the existing Run-Length-Encoded attribute list.
                LOG_IF_FAILED(Row.GetAttrRow().InsertAttrRuns(&InsertedRun,
                                                              1,
                                                              TargetPoint.X + i,
                                                              (SHORT)(TargetPoint.X + i),
                                                              coordScreenBufferSize.X));
            }
        }
        else
        {
            InsertedRun.SetLength(StringLength);
            InsertedRun.SetAttributesFromLegacy(wScreenAttributes);
            LOG_IF_FAILED(Row.GetAttrRow().InsertAttrRuns(&InsertedRun,
                                                          1,
                                                          TargetPoint.X,
                                                          (SHORT)(TargetPoint.X + StringLength - 1),
                                                          coordScreenBufferSize.X));
        }
    }

    int tempInt;
    HRESULT hr = SizeTToInt(TargetPoint.X + StringLength - 1, &tempInt);
    if (FAILED(hr))
    {
        return;
    }
    short tempShort;
    hr = IntToShort(tempInt, &tempShort);
    if (FAILED(hr))
    {
        return;
    }

    ScreenInfo->ResetTextFlags(TargetPoint.X, TargetPoint.Y, tempShort, TargetPoint.Y);
}
