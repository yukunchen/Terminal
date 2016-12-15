/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "_output.h"
#include "output.h"

#include "cursor.h"
#include "dbcs.h"
#include "getset.h"
#include "globals.h"
#include "misc.h"
#include "srvinit.h"
#include "window.hpp"
#include "userprivapi.hpp"

#pragma hdrstop


// NOTE: we use this to communicate with progman - see Q105446 for details.
typedef struct _PMIconData
{
    DWORD dwResSize;
    DWORD dwVer;
    BYTE iResource; // icon resource
} PMICONDATA, *LPPMICONDATA;

// This routine figures out what parameters to pass to CreateScreenBuffer based on the data from STARTUPINFO and the
// registry defaults, and then calls CreateScreenBuffer.
NTSTATUS DoCreateScreenBuffer()
{
    CHAR_INFO Fill;
    Fill.Attributes = g_ciConsoleInformation.GetFillAttribute();
    Fill.Char.UnicodeChar = UNICODE_SPACE;

    CHAR_INFO PopupFill;
    PopupFill.Attributes = g_ciConsoleInformation.GetPopupFillAttribute();
    PopupFill.Char.UnicodeChar = UNICODE_SPACE;

    FontInfo fiFont(g_ciConsoleInformation.GetFaceName(),
                    static_cast<BYTE>(g_ciConsoleInformation.GetFontFamily()),
                    g_ciConsoleInformation.GetFontWeight(),
                    g_ciConsoleInformation.GetFontSize(),
                    g_ciConsoleInformation.GetCodePage());

    // For East Asian version, we want to get the code page from the registry or shell32, so we can specify console
    // codepage by console.cpl or shell32. The default codepage is OEMCP.
    g_ciConsoleInformation.CP = g_ciConsoleInformation.GetCodePage();
    g_ciConsoleInformation.OutputCP = g_ciConsoleInformation.GetCodePage();

    g_ciConsoleInformation.Flags |= CONSOLE_USE_PRIVATE_FLAGS;

    NTSTATUS Status = SCREEN_INFORMATION::CreateInstance(g_ciConsoleInformation.GetWindowSize(),
                                                         &fiFont,
                                                         g_ciConsoleInformation.GetScreenBufferSize(),
                                                         Fill,
                                                         PopupFill,
                                                         g_ciConsoleInformation.GetCursorSize(),
                                                         &g_ciConsoleInformation.ScreenBuffers);

    // TODO: MSFT 9355013: This needs to be resolved. We increment it once with no handle to ensure it's never cleaned up
    // and one always exists for the renderer (and potentially other functions.)
    // It's currently a load-bearing piece of code. http://osgvsowi/9355013
    g_ciConsoleInformation.ScreenBuffers[0].Header.IncrementOriginalScreenBuffer();

    return Status;
}

/*
 * This routine tells win32k what process we want to use to masquerade as the
 * owner of conhost's window. If ProcessData is nullptr that means the root process
 * has exited so we need to find any old process to be the owner. If this console
 * has no processes attached to it -- it's only being kept alive by references
 * via IO handles -- then we'll just set the owner to conhost.exe itself.
 */
VOID SetConsoleWindowOwner(_In_ HWND hwnd, _Inout_opt_ ConsoleProcessHandle* pProcessData)
{
    ASSERT(g_ciConsoleInformation.IsConsoleLocked());

    DWORD dwProcessId;
    DWORD dwThreadId;
    if (nullptr != pProcessData)
    {
        dwProcessId = pProcessData->dwProcessId;
        dwThreadId = pProcessData->dwThreadId;
    }
    else
    {
        // Find a process to own the console window. If there are none then let's use conhost's.
        pProcessData = g_ciConsoleInformation.ProcessHandleList.GetFirstProcess();
        if (pProcessData != nullptr)
        {
            dwProcessId = pProcessData->dwProcessId;
            dwThreadId = pProcessData->dwThreadId;
            pProcessData->fRootProcess = true;
        }
        else
        {
            dwProcessId = GetCurrentProcessId();
            dwThreadId = GetCurrentThreadId();
        }
    }

    CONSOLEWINDOWOWNER ConsoleOwner;
    ConsoleOwner.hwnd = hwnd;
    ConsoleOwner.ProcessId = dwProcessId;
    ConsoleOwner.ThreadId = dwThreadId;

    UserPrivApi::s_ConsoleControl(UserPrivApi::CONSOLECONTROL::ConsoleSetWindowOwner, &ConsoleOwner, sizeof(ConsoleOwner));
}

// Routine Description:
// - This routine copies a rectangular region from the screen buffer. no clipping is done.
// Arguments:
// - ScreenInfo - pointer to screen info
// - SourcePoint - upper left coordinates of source rectangle
// - Target - pointer to target buffer
// - TargetSize - dimensions of target buffer
// - TargetRect - rectangle in source buffer to copy
// Return Value:
// - <none>
NTSTATUS ReadRectFromScreenBuffer(_In_ const SCREEN_INFORMATION * const pScreenInfo,
                                  _In_ COORD const coordSourcePoint,
                                  _Inout_ PCHAR_INFO pciTarget,
                                  _In_ COORD const coordTargetSize,
                                  _In_ PSMALL_RECT const psrTargetRect,
                                  _Inout_opt_ TextAttribute* const pTextAttributes)
{
    DBGOUTPUT(("ReadRectFromScreenBuffer\n"));

    const bool fOutputTextAttributes = pTextAttributes != nullptr;
    short const sXSize = (short)(psrTargetRect->Right - psrTargetRect->Left + 1);
    short const sYSize = (short)(psrTargetRect->Bottom - psrTargetRect->Top + 1);
    ASSERT(sYSize > 0);
    const int ScreenBufferWidth = pScreenInfo->GetScreenBufferSize().X;

    ROW* pRow = pScreenInfo->TextInfo->GetRowByOffset(coordSourcePoint.Y);
    NTSTATUS Status = STATUS_SUCCESS;
    for (short iRow = 0; iRow < sYSize; iRow++)
    {
        PCHAR_INFO pciTargetPtr = (PCHAR_INFO)((PBYTE)pciTarget + SCREEN_BUFFER_POINTER(psrTargetRect->Left,
                                                                                        psrTargetRect->Top + iRow,
                                                                                        coordTargetSize.X,
                                                                                        sizeof(CHAR_INFO)));

        TextAttribute* pTargetAttributes = nullptr;
        if (fOutputTextAttributes)
        {
            pTargetAttributes = (TextAttribute*)(pTextAttributes + SCREEN_BUFFER_POINTER(psrTargetRect->Left,
                                                                                         psrTargetRect->Top + iRow,
                                                                                         coordTargetSize.X,
                                                                                         1));
        }

        // copy the chars and attrs from their respective arrays
        PWCHAR pwChar = &pRow->CharRow.Chars[coordSourcePoint.X];

        // Unpack the attributes into an array so we can iterate over them.
        TextAttribute* rgUnpackedRowAttributes = new TextAttribute[ScreenBufferWidth];
        Status = NT_TESTNULL(rgUnpackedRowAttributes);
        if (NT_SUCCESS(Status))
        {
            pRow->AttrRow.UnpackAttrs(rgUnpackedRowAttributes, ScreenBufferWidth);

            PBYTE pbAttrP = &pRow->CharRow.KAttrs[coordSourcePoint.X];

            for (short iCol = 0; iCol < sXSize; pciTargetPtr++)
            {
                BYTE bAttrR = *pbAttrP++;
                TextAttribute textAttr = rgUnpackedRowAttributes[coordSourcePoint.X + iCol];

                if (iCol == 0 && bAttrR & CHAR_ROW::ATTR_TRAILING_BYTE)
                {
                    pciTargetPtr->Char.UnicodeChar = UNICODE_SPACE;
                    bAttrR = 0;
                }
                else if (iCol + 1 >= sXSize && bAttrR & CHAR_ROW::ATTR_LEADING_BYTE)
                {
                    pciTargetPtr->Char.UnicodeChar = UNICODE_SPACE;
                    bAttrR = 0;
                }
                else
                {
                    pciTargetPtr->Char.UnicodeChar = *pwChar;
                }

                pwChar++;
                pciTargetPtr->Attributes = (bAttrR & CHAR_ROW::ATTR_DBCSSBCS_BYTE) << 8;

                // Always copy the legacy attributes to the CHAR_INFO.
                pciTargetPtr->Attributes |= g_ciConsoleInformation.GenerateLegacyAttributes(textAttr);

                if (fOutputTextAttributes)
                {
                    pTargetAttributes->SetFrom(textAttr);
                    pTargetAttributes++;
                }

                iCol += 1;

            }
            delete[] rgUnpackedRowAttributes;
        }
        else
        {
            break;
        }

        pRow = pScreenInfo->TextInfo->GetNextRowNoWrap(pRow);
    }
    return Status;
}

// Routine Description:
// - This routine copies a rectangular region from the screen buffer to the screen buffer.  no clipping is done.
// Arguments:
// - ScreenInfo - pointer to screen info
// - SourceRect - rectangle in source buffer to copy
// - TargetPoint - upper left coordinates of new location rectangle
// Return Value:
// - <none>
NTSTATUS _CopyRectangle(_In_ PSCREEN_INFORMATION pScreenInfo, _In_ const SMALL_RECT * const psrSource, _In_ const COORD coordTarget)
{
    DBGOUTPUT(("_CopyRectangle\n"));

    COORD SourcePoint;
    SourcePoint.X = psrSource->Left;
    SourcePoint.Y = psrSource->Top;

    SMALL_RECT Target;
    Target.Left = 0;
    Target.Top = 0;
    Target.Right = psrSource->Right - psrSource->Left;
    Target.Bottom = psrSource->Bottom - psrSource->Top;

    COORD Size;
    Size.X = Target.Right;
    Size.Y = Target.Bottom;
    Size.X++;
    Size.Y++;

    CHAR_INFO* rgciScrollBuffer = new CHAR_INFO[Size.X * Size.Y];
    NTSTATUS Status = NT_TESTNULL(rgciScrollBuffer);
    if (NT_SUCCESS(Status))
    {
        TextAttribute* rTextAttrs = new TextAttribute[Size.X * Size.Y];
        Status = NT_TESTNULL(rTextAttrs);
        if (NT_SUCCESS(Status))
        {
            Status = ReadRectFromScreenBuffer(pScreenInfo, SourcePoint, rgciScrollBuffer, Size, &Target, rTextAttrs);
            if (NT_SUCCESS(Status))
            {
                Status = WriteRectToScreenBuffer((PBYTE)rgciScrollBuffer, Size, &Target, pScreenInfo, coordTarget, rTextAttrs);  // ScrollBuffer won't need conversion
            }
            delete[] rTextAttrs;
        }
        delete[] rgciScrollBuffer;
    }
    return Status;
}

// Routine Description:
// - This routine reads a rectangular region from the screen buffer. The region is first clipped.
// Arguments:
// - ScreenInformation - Screen buffer to read from.
// - Buffer - Buffer to read into.
// - ReadRegion - Region to read.
// Return Value:
NTSTATUS ReadScreenBuffer(_In_ const SCREEN_INFORMATION * const pScreenInfo, _Inout_ PCHAR_INFO pciBuffer, _Inout_ PSMALL_RECT psrReadRegion)
{
    DBGOUTPUT(("ReadScreenBuffer\n"));

    // calculate dimensions of caller's buffer.  have to do this calculation before clipping.
    COORD TargetSize;
    TargetSize.X = (SHORT)(psrReadRegion->Right - psrReadRegion->Left + 1);
    TargetSize.Y = (SHORT)(psrReadRegion->Bottom - psrReadRegion->Top + 1);

    if (TargetSize.X <= 0 || TargetSize.Y <= 0)
    {
        psrReadRegion->Right = psrReadRegion->Left - 1;
        psrReadRegion->Bottom = psrReadRegion->Top - 1;
        return STATUS_SUCCESS;
    }

    // do clipping.
    const COORD coordScreenBufferSize = pScreenInfo->GetScreenBufferSize();
    if (psrReadRegion->Right > (SHORT)(coordScreenBufferSize.X - 1))
    {
        psrReadRegion->Right = (SHORT)(coordScreenBufferSize.X - 1);
    }
    if (psrReadRegion->Bottom > (SHORT)(coordScreenBufferSize.Y - 1))
    {
        psrReadRegion->Bottom = (SHORT)(coordScreenBufferSize.Y - 1);
    }

    COORD TargetPoint;
    if (psrReadRegion->Left < 0)
    {
        TargetPoint.X = -psrReadRegion->Left;
        psrReadRegion->Left = 0;
    }
    else
    {
        TargetPoint.X = 0;
    }

    if (psrReadRegion->Top < 0)
    {
        TargetPoint.Y = -psrReadRegion->Top;
        psrReadRegion->Top = 0;
    }
    else
    {
        TargetPoint.Y = 0;
    }

    COORD SourcePoint;
    SourcePoint.X = psrReadRegion->Left;
    SourcePoint.Y = psrReadRegion->Top;

    SMALL_RECT Target;
    Target.Left = TargetPoint.X;
    Target.Top = TargetPoint.Y;
    Target.Right = TargetPoint.X + (psrReadRegion->Right - psrReadRegion->Left);
    Target.Bottom = TargetPoint.Y + (psrReadRegion->Bottom - psrReadRegion->Top);

    return ReadRectFromScreenBuffer(pScreenInfo, SourcePoint, pciBuffer, TargetSize, &Target, nullptr);
}

// Routine Description:
// - This routine write a rectangular region to the screen buffer. The region is first clipped.
// - The region should contain Unicode or UnicodeOem chars.
// Arguments:
// - ScreenInformation - Screen buffer to write to.
// - Buffer - Buffer to write from.
// - ReadRegion - Region to write.
// Return Value:
NTSTATUS WriteScreenBuffer(_In_ PSCREEN_INFORMATION pScreenInfo, _In_ PCHAR_INFO pciBuffer, _Inout_ PSMALL_RECT psrWriteRegion)
{
    DBGOUTPUT(("WriteScreenBuffer\n"));

    // Calculate dimensions of caller's buffer; this calculation must be done before clipping.
    COORD SourceSize;
    SourceSize.X = (SHORT)(psrWriteRegion->Right - psrWriteRegion->Left + 1);
    SourceSize.Y = (SHORT)(psrWriteRegion->Bottom - psrWriteRegion->Top + 1);

    if (SourceSize.X <= 0 || SourceSize.Y <= 0)
    {
        return STATUS_SUCCESS;
    }

    // Ensure that the write region is within the constraints of the screen buffer.
    const COORD coordScreenBufferSize = pScreenInfo->GetScreenBufferSize();
    if (psrWriteRegion->Left >= coordScreenBufferSize.X || psrWriteRegion->Top >= coordScreenBufferSize.Y)
    {
        return STATUS_SUCCESS;
    }

    SMALL_RECT SourceRect;
    // Do clipping.
    if (psrWriteRegion->Right > (SHORT)(coordScreenBufferSize.X - 1))
    {
        psrWriteRegion->Right = (SHORT)(coordScreenBufferSize.X - 1);
    }
    SourceRect.Right = psrWriteRegion->Right - psrWriteRegion->Left;

    if (psrWriteRegion->Bottom > (SHORT)(coordScreenBufferSize.Y - 1))
    {
        psrWriteRegion->Bottom = (SHORT)(coordScreenBufferSize.Y - 1);
    }
    SourceRect.Bottom = psrWriteRegion->Bottom - psrWriteRegion->Top;

    if (psrWriteRegion->Left < 0)
    {
        SourceRect.Left = -psrWriteRegion->Left;
        psrWriteRegion->Left = 0;
    }
    else
    {
        SourceRect.Left = 0;
    }

    if (psrWriteRegion->Top < 0)
    {
        SourceRect.Top = -psrWriteRegion->Top;
        psrWriteRegion->Top = 0;
    }
    else
    {
        SourceRect.Top = 0;
    }

    if (SourceRect.Left > SourceRect.Right || SourceRect.Top > SourceRect.Bottom)
    {
        return STATUS_INVALID_PARAMETER;
    }

    COORD TargetPoint;
    TargetPoint.X = psrWriteRegion->Left;
    TargetPoint.Y = psrWriteRegion->Top;

    return WriteRectToScreenBuffer((PBYTE) pciBuffer, SourceSize, &SourceRect, pScreenInfo, TargetPoint, nullptr);
}

// Routine Description:
// - This routine reads a string of characters or attributes from the screen buffer.
// Arguments:
// - ScreenInfo - Pointer to screen buffer information.
// - Buffer - Buffer to read into.
// - ReadCoord - Screen buffer coordinate to begin reading from.
// - StringType
//      CONSOLE_ASCII         - read a string of ASCII characters.
//      CONSOLE_REAL_UNICODE  - read a string of Real Unicode characters.
//      CONSOLE_FALSE_UNICODE - read a string of False Unicode characters.
//      CONSOLE_ATTRIBUTE     - read a string of attributes.
// - NumRecords - On input, the size of the buffer in elements.  On output, the number of elements read.
// Return Value:
NTSTATUS ReadOutputString(_In_ const SCREEN_INFORMATION * const pScreenInfo,
                          _Inout_ PVOID pvBuffer,
                          _In_ const COORD coordRead,
                          _In_ const ULONG ulStringType,
                          _Inout_ PULONG pcRecords)
{
    DBGOUTPUT(("ReadOutputString\n"));

    if (*pcRecords == 0)
    {
        return STATUS_SUCCESS;
    }

    ULONG NumRead = 0;
    SHORT X = coordRead.X;
    SHORT Y = coordRead.Y;
    const COORD coordScreenBufferSize = pScreenInfo->GetScreenBufferSize();
    if (X >= coordScreenBufferSize.X || X < 0 || Y >= coordScreenBufferSize.Y || Y < 0)
    {
        *pcRecords = 0;
        return STATUS_SUCCESS;
    }

    PWCHAR TransBuffer = nullptr;
    PWCHAR BufPtr;
    if (ulStringType == CONSOLE_ASCII)
    {
        TransBuffer = new WCHAR[*pcRecords];
        if (TransBuffer == nullptr)
        {
            return STATUS_NO_MEMORY;
        }
        BufPtr = TransBuffer;
    }
    else
    {
        BufPtr = (PWCHAR)pvBuffer;
    }

    PBYTE TransBufferA = nullptr;
    PBYTE BufPtrA = nullptr;

    {
        TransBufferA = new BYTE[*pcRecords];
        if (TransBufferA == nullptr)
        {
            if (TransBuffer != nullptr)
            {
                delete[] TransBuffer;
            }

            return STATUS_NO_MEMORY;
        }

        BufPtrA = TransBufferA;
    }

    {
        ROW* Row = pScreenInfo->TextInfo->GetRowByOffset(coordRead.Y);
        ASSERT(Row != nullptr);
        PWCHAR Char;
        SHORT j, k;
        PBYTE AttrP = nullptr;

        if ((ulStringType == CONSOLE_ASCII) || (ulStringType == CONSOLE_REAL_UNICODE) || (ulStringType == CONSOLE_FALSE_UNICODE))
        {
            while (NumRead < *pcRecords)
            {
                if (Row == nullptr)
                {
                    ASSERT(false);
                    break;
                }

                // copy the chars from its array
                Char = &Row->CharRow.Chars[X];
                AttrP = &Row->CharRow.KAttrs[X];

                if ((ULONG)(coordScreenBufferSize.X - X) >(*pcRecords - NumRead))
                {
                    memmove(BufPtr, Char, (*pcRecords - NumRead) * sizeof(WCHAR));
                    memmove(BufPtrA, AttrP, (*pcRecords - NumRead) * sizeof(CHAR));

                    NumRead += *pcRecords - NumRead;
                    break;
                }

                memmove(BufPtr, Char, (coordScreenBufferSize.X - X) * sizeof(WCHAR));
                BufPtr = (PWCHAR)((PBYTE)BufPtr + ((coordScreenBufferSize.X - X) * sizeof(WCHAR)));

                memmove(BufPtrA, AttrP, (coordScreenBufferSize.X - X) * sizeof(CHAR));
                BufPtrA = (PBYTE)((PBYTE)BufPtrA + ((coordScreenBufferSize.X - X) * sizeof(CHAR)));

                NumRead += coordScreenBufferSize.X - X;

                Row = pScreenInfo->TextInfo->GetNextRowNoWrap(Row);

                X = 0;
                Y++;
                if (Y >= coordScreenBufferSize.Y)
                {
                    break;
                }
            }

            if (NumRead)
            {
                if (ulStringType == CONSOLE_ASCII)
                {
                    Char = BufPtr = TransBuffer;
                }
                else
                {
                    Char = BufPtr = (PWCHAR)pvBuffer;
                }

                BufPtrA = TransBufferA;
#pragma prefast(suppress:__WARNING_BUFFER_OVERFLOW, "This code is fine")
                if (*BufPtrA & CHAR_ROW::ATTR_TRAILING_BYTE)
                {
                    j = k = (SHORT)(NumRead - 1);
                    BufPtr++;
                    *Char++ = UNICODE_SPACE;
                    BufPtrA++;
                    NumRead = 1;
                }
                else
                {
                    j = k = (SHORT)NumRead;
                    NumRead = 0;
                }

                while (j--)
                {
                    if (!(*BufPtrA & CHAR_ROW::ATTR_TRAILING_BYTE))
                    {
                        *Char++ = *BufPtr;
                        NumRead++;
                    }
                    BufPtr++;
                    BufPtrA++;
                }

                if (k && *(BufPtrA - 1) & CHAR_ROW::ATTR_LEADING_BYTE)
                {
                    *(Char - 1) = UNICODE_SPACE;
                }
            }
        }
        else if (ulStringType == CONSOLE_ATTRIBUTE)
        {
            SHORT CountOfAttr;
            TextAttributeRun* pAttrRun;
            PWORD TargetPtr = (PWORD)BufPtr;

            while (NumRead < *pcRecords)
            {
                if (Row == nullptr)
                {
                    ASSERT(false);
                    break;
                }

                // Copy the attrs from its array.
                AttrP = &Row->CharRow.KAttrs[X];

                UINT uiCountOfAttr;
                Row->AttrRow.FindAttrIndex(X, &pAttrRun, &uiCountOfAttr);

                // attempt safe cast. bail early if failed.
                if (FAILED(UIntToShort(uiCountOfAttr, &CountOfAttr)))
                {
                    ASSERT(false);
                    return STATUS_UNSUCCESSFUL;
                }

                k = 0;
                for (j = X; j < coordScreenBufferSize.X; TargetPtr++)
                {
                    const WORD wLegacyAttributes = pAttrRun->GetAttributes().GetLegacyAttributes();
                    if ((j == X) && (*AttrP & CHAR_ROW::ATTR_TRAILING_BYTE))
                    {
                        *TargetPtr = wLegacyAttributes;
                    }
                    else if (*AttrP & CHAR_ROW::ATTR_LEADING_BYTE)
                    {
                        if ((NumRead == *pcRecords - 1) || (j == coordScreenBufferSize.X - 1))
                        {
                            *TargetPtr = wLegacyAttributes;
                        }
                        else
                        {
                            *TargetPtr = wLegacyAttributes | (WCHAR)(*AttrP & CHAR_ROW::ATTR_DBCSSBCS_BYTE) << 8;
                        }
                    }
                    else
                    {
                        *TargetPtr = wLegacyAttributes | (WCHAR)(*AttrP & CHAR_ROW::ATTR_DBCSSBCS_BYTE) << 8;
                    }

                    NumRead++;
                    j += 1;
                    if (++k == CountOfAttr && j < coordScreenBufferSize.X)
                    {
                        pAttrRun++;
                        k = 0;
                        if (!SUCCEEDED(UIntToShort(pAttrRun->GetLength(), &CountOfAttr)))
                        {
                            ASSERT(false);
                            return STATUS_UNSUCCESSFUL;
                        }
                    }

                    if (NumRead == *pcRecords)
                    {
                        delete[] TransBufferA;
                        return STATUS_SUCCESS;
                    }

                    AttrP++;
                }

                Row = pScreenInfo->TextInfo->GetNextRowNoWrap(Row);

                X = 0;
                Y++;
                if (Y >= coordScreenBufferSize.Y)
                {
                    break;
                }
            }
        }
        else
        {
            *pcRecords = 0;
            delete[] TransBufferA;

            return STATUS_INVALID_PARAMETER;
        }
    }

    if (ulStringType == CONSOLE_ASCII)
    {
        UINT const Codepage = g_ciConsoleInformation.OutputCP;

        NumRead = ConvertToOem(Codepage, TransBuffer, NumRead, (LPSTR) pvBuffer, *pcRecords);

        delete[] TransBuffer;
    }

    delete[] TransBufferA;

    *pcRecords = NumRead;
    return STATUS_SUCCESS;
}

void ScreenBufferSizeChange(_In_ COORD const coordNewSize)
{
    INPUT_RECORD InputEvent;
    InputEvent.EventType = WINDOW_BUFFER_SIZE_EVENT;
    InputEvent.Event.WindowBufferSizeEvent.dwSize = coordNewSize;

    WriteInputBuffer(g_ciConsoleInformation.pInputBuffer, &InputEvent, 1);
}

void ScrollScreen(_Inout_ PSCREEN_INFORMATION pScreenInfo,
                  _In_ const SMALL_RECT * const psrScroll,
                  _In_opt_ const SMALL_RECT * const psrMerge,
                  _In_ const COORD coordTarget)
{
    if (pScreenInfo->IsActiveScreenBuffer())
    {
        // Notify accessibility that a scroll has occurred.
        NotifyWinEvent(EVENT_CONSOLE_UPDATE_SCROLL,
                       g_ciConsoleInformation.hWnd,
                       coordTarget.X - psrScroll->Left,
                       coordTarget.Y - psrScroll->Right);


        if (g_pRender != nullptr)
        {
            // psrScroll is the source rectangle which gets written with the same dimensions to the coordTarget position.
            // Therefore the final rectangle starts with the top left corner at coordTarget
            // and the size is the size of psrScroll.
            // NOTE: psrScroll is an INCLUSIVE rectangle, so we must add 1 when measuring width as R-L or B-T
            SMALL_RECT rcWritten = *psrScroll;
            rcWritten.Left = coordTarget.X;
            rcWritten.Top = coordTarget.Y;
            rcWritten.Right = rcWritten.Left + (psrScroll->Right - psrScroll->Left + 1);
            rcWritten.Bottom = rcWritten.Top + (psrScroll->Bottom - psrScroll->Top + 1);

            g_pRender->TriggerRedraw(&rcWritten);

            // psrMerge was just filled exactly where it's stated.
            if (psrMerge != nullptr)
            {
                // psrMerge is an inclusive rectangle. Make it exclusive to deal with the renderer.
                SMALL_RECT rcMerge = *psrMerge;
                rcMerge.Bottom++;
                rcMerge.Right++;

                g_pRender->TriggerRedraw(&rcMerge);
            }
        }
    }
}

// Routine Description:
// - This routine rotates the circular buffer as a shorthand for scrolling the entire screen
SHORT ScrollEntireScreen(_Inout_ PSCREEN_INFORMATION pScreenInfo, _In_ const SHORT sScrollValue)
{
    // store index of first row
    SHORT const RowIndex = pScreenInfo->TextInfo->GetFirstRowIndex();

    // update screen buffer
    pScreenInfo->TextInfo->SetFirstRowIndex((SHORT)((RowIndex + sScrollValue) % pScreenInfo->GetScreenBufferSize().Y));

    return RowIndex;
}

// Routine Description:
// - This routine is a special-purpose scroll for use by AdjustCursorPosition.
// Arguments:
// - ScreenInfo - pointer to screen buffer info.
// Return Value:
// - true if we succeeded in scrolling the buffer, otherwise false (if we're out of memory)
bool StreamScrollRegion(_Inout_ PSCREEN_INFORMATION pScreenInfo)
{
    // Rotate the circular buffer around and wipe out the previous final line.
    bool fSuccess = pScreenInfo->TextInfo->IncrementCircularBuffer();
    if (fSuccess)
    {
        // Trigger a graphical update if we're active.
        if (pScreenInfo->IsActiveScreenBuffer())
        {
            COORD coordDelta = { 0 };
            coordDelta.Y = -1;

            // Notify accessibility that a scroll has occurred.
            NotifyWinEvent(EVENT_CONSOLE_UPDATE_SCROLL, g_ciConsoleInformation.hWnd, coordDelta.X, coordDelta.Y);

            if (g_pRender != nullptr)
            {
                g_pRender->TriggerScroll(&coordDelta);
            }
        }
    }
    return fSuccess;
}

// Routine Description:
// - This routine copies ScrollRectangle to DestinationOrigin then fills in ScrollRectangle with Fill.
// - The scroll region is copied to a third buffer, the scroll region is filled, then the original contents of the scroll region are copied to the destination.
// Arguments:
// - ScreenInfo - pointer to screen buffer info.
// - ScrollRectangle - Region to copy
// - ClipRectangle - Optional pointer to clip region.
// - DestinationOrigin - Upper left corner of target region.
// - Fill - Character and attribute to fill source region with.
// Return Value:
NTSTATUS ScrollRegion(_Inout_ PSCREEN_INFORMATION pScreenInfo,
                      _Inout_ PSMALL_RECT psrScroll,
                      _In_opt_ PSMALL_RECT psrClip,
                      _In_ COORD coordDestinationOrigin,
                      _In_ CHAR_INFO ciFill)
{
    // here's how we clip:

    // Clip source rectangle to screen buffer => S
    // Create target rectangle based on S => T
    // Clip T to ClipRegion => T
    // Create S2 based on clipped T => S2
    // Clip S to ClipRegion => S3

    // S2 is the region we copy to T
    // S3 is the region to fill

    if (ciFill.Char.UnicodeChar == '\0' && ciFill.Attributes == 0)
    {
        ciFill.Char.UnicodeChar = L' ';
        ciFill.Attributes = pScreenInfo->GetAttributes().GetLegacyAttributes();
    }

    // clip the source rectangle to the screen buffer
    if (psrScroll->Left < 0)
    {
        coordDestinationOrigin.X += -psrScroll->Left;
        psrScroll->Left = 0;
    }
    if (psrScroll->Top < 0)
    {
        coordDestinationOrigin.Y += -psrScroll->Top;
        psrScroll->Top = 0;
    }

    const COORD coordScreenBufferSize = pScreenInfo->GetScreenBufferSize();
    if (psrScroll->Right >= coordScreenBufferSize.X)
    {
        psrScroll->Right = (SHORT)(coordScreenBufferSize.X - 1);
    }
    if (psrScroll->Bottom >= coordScreenBufferSize.Y)
    {
        psrScroll->Bottom = (SHORT)(coordScreenBufferSize.Y - 1);
    }

    // if source rectangle doesn't intersect screen buffer, return.
    if (psrScroll->Bottom < psrScroll->Top || psrScroll->Right < psrScroll->Left)
    {
        return STATUS_SUCCESS;
    }

    // Account for the scroll margins set by DECSTBM
    SMALL_RECT srScrollMargins = pScreenInfo->GetScrollMargins();
    SMALL_RECT srBufferViewport = pScreenInfo->GetBufferViewport();

    // The margins are in viewport relative coordinates. Adjust for that.
    srScrollMargins.Top += srBufferViewport.Top;
    srScrollMargins.Bottom += srBufferViewport.Top;
    srScrollMargins.Left += srBufferViewport.Left;
    srScrollMargins.Right += srBufferViewport.Left;

    if (srScrollMargins.Bottom > srScrollMargins.Top)
    {
        if (psrScroll->Top < srScrollMargins.Top)
        {
            psrScroll->Top = srScrollMargins.Top;
        }
        if (psrScroll->Bottom >= srScrollMargins.Bottom)
        {
            psrScroll->Bottom = srScrollMargins.Bottom;
        }
    }
    SMALL_RECT OurClipRectangle;
    // clip the target rectangle
    // if a cliprectangle was provided, clip it to the screen buffer.
    // if not, set the cliprectangle to the screen buffer region.
    if (psrClip)
    {
        // clip the cliprectangle.
        if (psrClip->Left < 0)
        {
            psrClip->Left = 0;
        }
        if (psrClip->Top < 0)
        {
            psrClip->Top = 0;
        }
        if (psrClip->Right >= coordScreenBufferSize.X)
        {
            psrClip->Right = (SHORT)(coordScreenBufferSize.X - 1);
        }
        if (psrClip->Bottom >= coordScreenBufferSize.Y)
        {
            psrClip->Bottom = (SHORT)(coordScreenBufferSize.Y - 1);
        }
    }
    else
    {
        OurClipRectangle.Left = 0;
        OurClipRectangle.Top = 0;
        OurClipRectangle.Right = (SHORT)(coordScreenBufferSize.X - 1);
        OurClipRectangle.Bottom = (SHORT)(coordScreenBufferSize.Y - 1);
        psrClip = &OurClipRectangle;
    }
    // Account for the scroll margins set by DECSTBM
    if (srScrollMargins.Bottom > srScrollMargins.Top)
    {
        if (psrClip->Top < srScrollMargins.Top)
        {
            psrClip->Top = srScrollMargins.Top;
        }
        if (psrClip->Bottom >= srScrollMargins.Bottom)
        {
            psrClip->Bottom = srScrollMargins.Bottom;
        }
    }
    // Create target rectangle based on S => T
    // Clip T to ClipRegion => T
    // Create S2 based on clipped T => S2

    SMALL_RECT ScrollRectangle2 = *psrScroll;

    SMALL_RECT TargetRectangle;
    TargetRectangle.Left = coordDestinationOrigin.X;
    TargetRectangle.Top = coordDestinationOrigin.Y;
    TargetRectangle.Right = (SHORT)(coordDestinationOrigin.X + (ScrollRectangle2.Right - ScrollRectangle2.Left + 1) - 1);
    TargetRectangle.Bottom = (SHORT)(coordDestinationOrigin.Y + (ScrollRectangle2.Bottom - ScrollRectangle2.Top + 1) - 1);

    if (TargetRectangle.Left < psrClip->Left)
    {
        ScrollRectangle2.Left += psrClip->Left - TargetRectangle.Left;
        TargetRectangle.Left = psrClip->Left;
    }
    if (TargetRectangle.Top < psrClip->Top)
    {
        ScrollRectangle2.Top += psrClip->Top - TargetRectangle.Top;
        TargetRectangle.Top = psrClip->Top;
    }
    if (TargetRectangle.Right > psrClip->Right)
    {
        ScrollRectangle2.Right -= TargetRectangle.Right - psrClip->Right;
        TargetRectangle.Right = psrClip->Right;
    }
    if (TargetRectangle.Bottom > psrClip->Bottom)
    {
        ScrollRectangle2.Bottom -= TargetRectangle.Bottom - psrClip->Bottom;
        TargetRectangle.Bottom = psrClip->Bottom;
    }

    // clip scroll rect to clipregion => S3
    SMALL_RECT ScrollRectangle3 = *psrScroll;
    if (ScrollRectangle3.Left < psrClip->Left)
    {
        ScrollRectangle3.Left = psrClip->Left;
    }
    if (ScrollRectangle3.Top < psrClip->Top)
    {
        ScrollRectangle3.Top = psrClip->Top;
    }
    if (ScrollRectangle3.Right > psrClip->Right)
    {
        ScrollRectangle3.Right = psrClip->Right;
    }
    if (ScrollRectangle3.Bottom > psrClip->Bottom)
    {
        ScrollRectangle3.Bottom = psrClip->Bottom;
    }

    // if scroll rect doesn't intersect clip region, return.
    if (ScrollRectangle3.Bottom < ScrollRectangle3.Top || ScrollRectangle3.Right < ScrollRectangle3.Left)
    {
        return STATUS_SUCCESS;
    }

    NTSTATUS Status = STATUS_SUCCESS;
    // if target rectangle doesn't intersect screen buffer, skip scrolling part.
    if (!(TargetRectangle.Bottom < TargetRectangle.Top || TargetRectangle.Right < TargetRectangle.Left))
    {
        // if we can, don't use intermediate scroll region buffer.  do this
        // by figuring out fill rectangle.  NOTE: this code will only work
        // if _CopyRectangle copies from low memory to high memory (otherwise
        // we would overwrite the scroll region before reading it).

        if (ScrollRectangle2.Right == TargetRectangle.Right &&
            ScrollRectangle2.Left == TargetRectangle.Left && ScrollRectangle2.Top > TargetRectangle.Top && ScrollRectangle2.Top < TargetRectangle.Bottom)
        {
            COORD TargetPoint;
            TargetPoint.X = TargetRectangle.Left;
            TargetPoint.Y = TargetRectangle.Top;

            if (ScrollRectangle2.Right == (SHORT)(coordScreenBufferSize.X - 1) &&
                ScrollRectangle2.Left == 0 && ScrollRectangle2.Bottom == (SHORT)(coordScreenBufferSize.Y - 1) && ScrollRectangle2.Top == 1)
            {
                ScrollEntireScreen(pScreenInfo, (SHORT)(ScrollRectangle2.Top - TargetRectangle.Top));
            }
            else
            {
                Status = _CopyRectangle(pScreenInfo, &ScrollRectangle2, TargetPoint);
            }
            if (NT_SUCCESS(Status))
            {
                SMALL_RECT FillRect;
                FillRect.Left = TargetRectangle.Left;
                FillRect.Right = TargetRectangle.Right;
                FillRect.Top = (SHORT)(TargetRectangle.Bottom + 1);
                FillRect.Bottom = psrScroll->Bottom;
                if (FillRect.Top < psrClip->Top)
                {
                    FillRect.Top = psrClip->Top;
                }

                if (FillRect.Bottom > psrClip->Bottom)
                {
                    FillRect.Bottom = psrClip->Bottom;
                }

                FillRectangle(&ciFill, pScreenInfo, &FillRect);

                ScrollScreen(pScreenInfo, &ScrollRectangle2, &FillRect, TargetPoint);
            }

        }

        // if no overlap, don't need intermediate copy
        else if (ScrollRectangle3.Right < TargetRectangle.Left ||
                 ScrollRectangle3.Left > TargetRectangle.Right ||
                 ScrollRectangle3.Top > TargetRectangle.Bottom ||
                 ScrollRectangle3.Bottom < TargetRectangle.Top)
        {
            COORD TargetPoint;
            TargetPoint.X = TargetRectangle.Left;
            TargetPoint.Y = TargetRectangle.Top;
            Status = _CopyRectangle(pScreenInfo, &ScrollRectangle2, TargetPoint);
            if (NT_SUCCESS(Status))
            {
                FillRectangle(&ciFill, pScreenInfo, &ScrollRectangle3);

                ScrollScreen(pScreenInfo, &ScrollRectangle2, &ScrollRectangle3, TargetPoint);
            }
        }

        // for the case where the source and target rectangles overlap, we copy the source rectangle, fill it, then copy it to the target.
        else
        {
            COORD Size;
            Size.X = (SHORT)(ScrollRectangle2.Right - ScrollRectangle2.Left + 1);
            Size.Y = (SHORT)(ScrollRectangle2.Bottom - ScrollRectangle2.Top + 1);

            CHAR_INFO* const rgciScrollBuffer = new CHAR_INFO[Size.X * Size.Y];
            Status = NT_TESTNULL(rgciScrollBuffer);
            if (SUCCEEDED(Status))
            {
                TextAttribute* rTextAttrs = new TextAttribute[Size.X * Size.Y];
                Status = NT_TESTNULL(rTextAttrs);
                if (SUCCEEDED(Status))
                {
                    SMALL_RECT TargetRect;
                    TargetRect.Left = 0;
                    TargetRect.Top = 0;
                    TargetRect.Right = ScrollRectangle2.Right - ScrollRectangle2.Left;
                    TargetRect.Bottom = ScrollRectangle2.Bottom - ScrollRectangle2.Top;

                    COORD SourcePoint;
                    SourcePoint.X = ScrollRectangle2.Left;
                    SourcePoint.Y = ScrollRectangle2.Top;

                    Status = ReadRectFromScreenBuffer(pScreenInfo, SourcePoint, rgciScrollBuffer, Size, &TargetRect, rTextAttrs);
                    if (NT_SUCCESS(Status))
                    {
                        FillRectangle(&ciFill, pScreenInfo, &ScrollRectangle3);

                        SMALL_RECT SourceRectangle;
                        SourceRectangle.Top = 0;
                        SourceRectangle.Left = 0;
                        SourceRectangle.Right = (SHORT)(Size.X - 1);
                        SourceRectangle.Bottom = (SHORT)(Size.Y - 1);

                        COORD TargetPoint;
                        TargetPoint.X = TargetRectangle.Left;
                        TargetPoint.Y = TargetRectangle.Top;

                        Status = WriteRectToScreenBuffer((PBYTE)rgciScrollBuffer, Size, &SourceRectangle, pScreenInfo, TargetPoint, rTextAttrs);
                        if (NT_SUCCESS(Status))
                        {
                            // update regions on screen.
                            ScrollScreen(pScreenInfo, &ScrollRectangle2, &ScrollRectangle3, TargetPoint);
                        }
                    }

                    delete[] rTextAttrs;
                }

                delete[] rgciScrollBuffer;
            }
        }
    }
    else
    {
        // Do fill.
        FillRectangle(&ciFill, pScreenInfo, &ScrollRectangle3);

        WriteToScreen(pScreenInfo, ScrollRectangle3);
    }
    return Status;
}

NTSTATUS SetActiveScreenBuffer(_Inout_ PSCREEN_INFORMATION pScreenInfo)
{
    g_ciConsoleInformation.CurrentScreenBuffer = pScreenInfo;

    // initialize cursor
    pScreenInfo->TextInfo->GetCursor()->SetIsOn(FALSE);

    // set font
    pScreenInfo->RefreshFontWithRenderer();

    // Empty input buffer.
    FlushAllButKeys();
    SetScreenColors(pScreenInfo, pScreenInfo->GetAttributes().GetLegacyAttributes(), pScreenInfo->GetPopupAttributes()->GetLegacyAttributes(), FALSE);

    // Set window size.
    pScreenInfo->PostUpdateWindowSize();

    g_ciConsoleInformation.ConsoleIme.RefreshAreaAttributes();

    // Write data to screen.
    WriteToScreen(pScreenInfo, pScreenInfo->GetBufferViewport());

    return STATUS_SUCCESS;
}

// TODO: MSFT 9450717 This should join the ProcessList class when CtrlEvents become moved into the server. https://osgvsowi/9450717
void CloseConsoleProcessState()
{
    // If there are no connected processes, sending control events is pointless as there's no one do send them to. In
    // this case we'll just exit conhost.

    // N.B. We can get into this state when a process has a reference to the console but hasn't connected. For example,
    //      when it's created suspended and never resumed.
    if (g_ciConsoleInformation.ProcessHandleList.IsEmpty())
    {
        ExitProcess(STATUS_SUCCESS);
    }

    HandleCtrlEvent(CTRL_CLOSE_EVENT);
}
