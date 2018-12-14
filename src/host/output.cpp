/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "_output.h"
#include "output.h"
#include "handle.h"

#include "getset.h"
#include "misc.h"

#include "../interactivity/inc/ServiceLocator.hpp"
#include "../types/inc/Viewport.hpp"
#include "../types/inc/convert.hpp"

#pragma hdrstop
using namespace Microsoft::Console::Types;

// This routine figures out what parameters to pass to CreateScreenBuffer based on the data from STARTUPINFO and the
// registry defaults, and then calls CreateScreenBuffer.
[[nodiscard]]
NTSTATUS DoCreateScreenBuffer()
{
    CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();

    FontInfo fiFont(gci.GetFaceName(),
                    static_cast<BYTE>(gci.GetFontFamily()),
                    gci.GetFontWeight(),
                    gci.GetFontSize(),
                    gci.GetCodePage());

    // For East Asian version, we want to get the code page from the registry or shell32, so we can specify console
    // codepage by console.cpl or shell32. The default codepage is OEMCP.
    gci.CP = gci.GetCodePage();
    gci.OutputCP = gci.GetCodePage();

    gci.Flags |= CONSOLE_USE_PRIVATE_FLAGS;

    NTSTATUS Status = SCREEN_INFORMATION::CreateInstance(gci.GetWindowSize(),
                                                         fiFont,
                                                         gci.GetScreenBufferSize(),
                                                         gci.GetDefaultAttributes(),
                                                         TextAttribute{ gci.GetPopupFillAttribute() },
                                                         gci.GetCursorSize(),
                                                         &gci.ScreenBuffers);

    // TODO: MSFT 9355013: This needs to be resolved. We increment it once with no handle to ensure it's never cleaned up
    // and one always exists for the renderer (and potentially other functions.)
    // It's currently a load-bearing piece of code. http://osgvsowi/9355013
    if (NT_SUCCESS(Status))
    {
        gci.ScreenBuffers[0].IncrementOriginalScreenBuffer();
    }

    return Status;
}

// Routine Description:
// - This routine copies a rectangular region from the screen buffer to the screen buffer.  no clipping is done.
// Arguments:
// - screenInfo - reference to screen info
// - sourceRect - rectangle in source buffer to copy
// - targetPoint - upper left coordinates of new location rectangle
static void _CopyRectangle(SCREEN_INFORMATION& screenInfo,
                           const Viewport source,
                           const COORD target)
{
    const auto bufferSize = screenInfo.GetBufferSize().Dimensions();

    const auto sourceFullRows = source.Width() == bufferSize.X;
    const auto targetFullRows = target.X == bufferSize.X;
    const auto verticalCopyOnly = source.Left() == 0 && target.X == 0;

    if (sourceFullRows && targetFullRows && verticalCopyOnly)
    {
        const auto delta = target.Y - source.Top();

        screenInfo.GetTextBuffer().ScrollRows(source.Top(), source.Height(), gsl::narrow<SHORT>(delta));
    }
    else
    {
        const auto cells = screenInfo.ReadRect(source);
        screenInfo.WriteRect(cells, target);
    }
}

// Routine Description:
// - This routine reads a sequence of attributes from the screen buffer.
// Arguments:
// - screenInfo - reference to screen buffer information.
// - coordRead - Screen buffer coordinate to begin reading from.
// - amountToRead - the number of elements to read
// Return Value:
// - vector of attribute data
std::vector<WORD> ReadOutputAttributes(const SCREEN_INFORMATION& screenInfo,
                                       const COORD coordRead,
                                       const size_t amountToRead)
{
    // Short circuit. If nothing to read, leave early.
    if (amountToRead == 0)
    {
        return {};
    }

    // Short circuit, if reading out of bounds, leave early.
    if (!screenInfo.GetBufferSize().IsInBounds(coordRead))
    {
        return {};
    }

    // Get iterator to the position we should start reading at.
    auto it = screenInfo.GetCellDataAt(coordRead);
    // Count up the number of cells we've attempted to read.
    ULONG amountRead = 0;
    // Prepare the return value string.
    std::vector<WORD> retVal;
    // Reserve the number of cells. If we have >U+FFFF, it will auto-grow later and that's OK.
    retVal.reserve(amountToRead);

    // While we haven't read enough cells yet and the iterator is still valid (hasn't reached end of buffer)
    while (amountRead < amountToRead && it)
    {
        // If the first thing we read is trailing, pad with a space.
        // OR If the last thing we read is leading, pad with a space.
        if ((amountRead == 0 && it->DbcsAttr().IsTrailing()) ||
            (amountRead == (amountToRead - 1) && it->DbcsAttr().IsLeading()))
        {
            retVal.push_back(it->TextAttr().GetLegacyAttributes());
        }
        else
        {
            retVal.push_back(it->TextAttr().GetLegacyAttributes() | it->DbcsAttr().GeneratePublicApiAttributeFormat());
        }

        amountRead++;
        it++;
    }

    return retVal;
}

// Routine Description:
// - This routine reads a sequence of unicode characters from the screen buffer
// Arguments:
// - screenInfo - reference to screen buffer information.
// - coordRead - Screen buffer coordinate to begin reading from.
// - amountToRead - the number of elements to read
// Return Value:
// - wstring
std::wstring ReadOutputStringW(const SCREEN_INFORMATION& screenInfo,
                               const COORD coordRead,
                               const size_t amountToRead)
{
    // Short circuit. If nothing to read, leave early.
    if (amountToRead == 0)
    {
        return {};
    }

    // Short circuit, if reading out of bounds, leave early.
    if (!screenInfo.GetBufferSize().IsInBounds(coordRead))
    {
        return {};
    }

    // Get iterator to the position we should start reading at.
    auto it = screenInfo.GetCellDataAt(coordRead);

    // Count up the number of cells we've attempted to read.
    ULONG amountRead = 0;

    // Prepare the return value string.
    std::wstring retVal;
    retVal.reserve(amountToRead); // Reserve the number of cells. If we have >U+FFFF, it will auto-grow later and that's OK.

    // While we haven't read enough cells yet and the iterator is still valid (hasn't reached end of buffer)
    while (amountRead < amountToRead && it)
    {
        // If the first thing we read is trailing, pad with a space.
        // OR If the last thing we read is leading, pad with a space.
        if ((amountRead == 0 && it->DbcsAttr().IsTrailing()) ||
            (amountRead == (amountToRead - 1) && it->DbcsAttr().IsLeading()))
        {
            retVal += UNICODE_SPACE;
        }
        else
        {
            // Otherwise, add anything that isn't a trailing cell. (Trailings are duplicate copies of the leading.)
            if (!it->DbcsAttr().IsTrailing())
            {
                retVal += it->Chars();
            }
        }

        amountRead++;
        it++;
    }

    return retVal;
}

// Routine Description:
// - This routine reads a sequence of ascii characters from the screen buffer
// Arguments:
// - screenInfo - reference to screen buffer information.
// - coordRead - Screen buffer coordinate to begin reading from.
// - amountToRead - the number of elements to read
// Return Value:
// - string of char data
std::string ReadOutputStringA(const SCREEN_INFORMATION& screenInfo,
                              const COORD coordRead,
                              const size_t amountToRead)
{
    const auto wstr = ReadOutputStringW(screenInfo, coordRead, amountToRead);

    const auto& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    return ConvertToA(gci.OutputCP, wstr);
}

void ScreenBufferSizeChange(const COORD coordNewSize)
{
    const CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();

    try
    {
        gci.pInputBuffer->Write(std::make_unique<WindowBufferSizeEvent>(coordNewSize));
    }
    catch (...)
    {
        LOG_HR(wil::ResultFromCaughtException());
    }
}

void ScrollScreen(SCREEN_INFORMATION& screenInfo,
                  const SMALL_RECT * const psrScroll,
                  _In_opt_ const SMALL_RECT * const psrMerge,
                  const COORD coordTarget)
{
    NTSTATUS status = STATUS_SUCCESS;

    if (screenInfo.IsActiveScreenBuffer())
    {
        IAccessibilityNotifier *pNotifier = ServiceLocator::LocateAccessibilityNotifier();
        status = NT_TESTNULL(pNotifier);

        if (NT_SUCCESS(status))
        {
            pNotifier->NotifyConsoleUpdateScrollEvent(coordTarget.X - psrScroll->Left, coordTarget.Y - psrScroll->Right);
        }
        IRenderer* const pRender = ServiceLocator::LocateGlobals().pRender;
        if (pRender != nullptr)
        {
            // psrScroll is the source rectangle which gets written with the same dimensions to the coordTarget position.
            // Therefore the final rectangle starts with the top left corner at coordTarget
            // and the size is the size of psrScroll.
            // NOTE: psrScroll is an INCLUSIVE rectangle, so we must add 1 when measuring width as R-L or B-T
            Viewport scrollRect = Viewport::FromInclusive(*psrScroll);
            const auto written = Viewport::FromDimensions(coordTarget, scrollRect.Width(), scrollRect.Height());

            pRender->TriggerRedraw(written);

            // psrMerge was just filled exactly where it's stated.
            if (psrMerge != nullptr)
            {
                // psrMerge is an inclusive rectangle. Make it exclusive to deal with the renderer.
                const auto merge = Viewport::FromInclusive(*psrMerge);

                pRender->TriggerRedraw(merge);
            }
        }
    }
}

// Routine Description:
// - This routine rotates the circular buffer as a shorthand for scrolling the entire screen
SHORT ScrollEntireScreen(SCREEN_INFORMATION& screenInfo, const SHORT sScrollValue)
{
    // store index of first row
    SHORT const RowIndex = screenInfo.GetTextBuffer().GetFirstRowIndex();

    // update screen buffer
    screenInfo.GetTextBuffer().SetFirstRowIndex((SHORT)((RowIndex + sScrollValue) % screenInfo.GetBufferSize().Height()));

    return RowIndex;
}

// Routine Description:
// - This routine is a special-purpose scroll for use by AdjustCursorPosition.
// Arguments:
// - screenInfo - reference to screen buffer info.
// Return Value:
// - true if we succeeded in scrolling the buffer, otherwise false (if we're out of memory)
bool StreamScrollRegion(SCREEN_INFORMATION& screenInfo)
{
    // Rotate the circular buffer around and wipe out the previous final line.
    bool fSuccess = screenInfo.GetTextBuffer().IncrementCircularBuffer();
    if (fSuccess)
    {
        // Trigger a graphical update if we're active.
        if (screenInfo.IsActiveScreenBuffer())
        {
            COORD coordDelta = { 0 };
            coordDelta.Y = -1;

            IAccessibilityNotifier *pNotifier = ServiceLocator::LocateAccessibilityNotifier();
            if (pNotifier)
            {
                // Notify accessibility that a scroll has occurred.
                pNotifier->NotifyConsoleUpdateScrollEvent(coordDelta.X, coordDelta.Y);
            }

            if (ServiceLocator::LocateGlobals().pRender != nullptr)
            {
                ServiceLocator::LocateGlobals().pRender->TriggerScroll(&coordDelta);
            }
        }
    }
    return fSuccess;
}

// Routine Description:
// - This routine copies ScrollRectangle to DestinationOrigin then fills in ScrollRectangle with Fill.
// - The scroll region is copied to a third buffer, the scroll region is filled, then the original contents of the scroll region are copied to the destination.
// Arguments:
// - screenInfo - reference to screen buffer info.
// - ScrollRectangle - Region to copy
// - ClipRectangle - Optional pointer to clip region.
// - DestinationOrigin - Upper left corner of target region.
// - fillCharGiven - Character to fill source region with.
// - fillAttrsGiven - attribute to fill source region with.
// NOTE: Throws exceptions
void ScrollRegion(SCREEN_INFORMATION& screenInfo,
                  const SMALL_RECT scrollRectGiven,
                  const std::optional<SMALL_RECT> clipRectGiven,
                  const COORD destinationOriginGiven,
                  const wchar_t fillCharGiven,
                  const TextAttribute fillAttrsGiven)
{
    auto fillWithChar = fillCharGiven;
    auto fillWithAttrs = fillAttrsGiven;
    auto scrollRect = scrollRectGiven;
    auto originCoordinate = destinationOriginGiven;

    // here's how we clip:

    // Clip source rectangle to screen buffer => S
    // Create target rectangle based on S => T
    // Clip T to ClipRegion => T
    // Create S2 based on clipped T => S2
    // Clip S to ClipRegion => S3

    // S2 is the region we copy to T
    // S3 is the region to fill

    if (fillWithChar == UNICODE_NULL && fillWithAttrs.IsLegacy() && fillWithAttrs.GetLegacyAttributes() == 0)
    {
        fillWithChar = UNICODE_SPACE;
        fillWithAttrs = screenInfo.GetAttributes();
    }

    const auto bufferSize = screenInfo.GetBufferSize().Dimensions();
    const COORD bufferLimits{ bufferSize.X - 1i16, bufferSize.Y - 1i16 };

    // clip the source rectangle to the screen buffer
    if (scrollRect.Left < 0)
    {
        originCoordinate.X += -scrollRect.Left;
        scrollRect.Left = 0;
    }
    if (scrollRect.Top < 0)
    {
        originCoordinate.Y += -scrollRect.Top;
        scrollRect.Top = 0;
    }

    if (scrollRect.Right >= bufferSize.X)
    {
        scrollRect.Right = bufferLimits.X;
    }
    if (scrollRect.Bottom >= bufferSize.Y)
    {
        scrollRect.Bottom = bufferLimits.Y;
    }

    // if source rectangle doesn't intersect screen buffer, return.
    if (scrollRect.Bottom < scrollRect.Top || scrollRect.Right < scrollRect.Left)
    {
        return;
    }

    // Account for the scroll margins set by DECSTBM
    auto marginRect = screenInfo.GetAbsoluteScrollMargins().ToInclusive();
    if (marginRect.Bottom > marginRect.Top)
    {
        if (scrollRect.Top < marginRect.Top)
        {
            scrollRect.Top = marginRect.Top;
        }
        if (scrollRect.Bottom >= marginRect.Bottom)
        {
            scrollRect.Bottom = marginRect.Bottom;
        }
    }

    // clip the target rectangle
    // if a cliprectangle was provided, clip it to the screen buffer.
    // if not, set the cliprectangle to the screen buffer region.
    auto clipRect = clipRectGiven.value_or(SMALL_RECT{ 0, 0, bufferSize.X - 1i16, bufferSize.Y - 1i16 });

    // clip the cliprectangle.
    clipRect.Left = std::max(clipRect.Left, 0i16);
    clipRect.Top = std::max(clipRect.Top, 0i16);
    clipRect.Right = std::min(clipRect.Right, bufferLimits.X);
    clipRect.Bottom = std::min(clipRect.Bottom, bufferLimits.Y);

    // Account for the scroll margins set by DECSTBM
    if (marginRect.Bottom > marginRect.Top)
    {
        if (clipRect.Top < marginRect.Top)
        {
            clipRect.Top = marginRect.Top;
        }
        if (clipRect.Bottom >= marginRect.Bottom)
        {
            clipRect.Bottom = marginRect.Bottom;
        }
    }
    // Create target rectangle based on S => T
    // Clip T to ClipRegion => T
    // Create S2 based on clipped T => S2

    auto scrollRect2 = scrollRect;

    SMALL_RECT targetRectangle;
    targetRectangle.Left = originCoordinate.X;
    targetRectangle.Top = originCoordinate.Y;
    targetRectangle.Right = (SHORT)(originCoordinate.X + (scrollRect2.Right - scrollRect2.Left + 1) - 1);
    targetRectangle.Bottom = (SHORT)(originCoordinate.Y + (scrollRect2.Bottom - scrollRect2.Top + 1) - 1);

    if (targetRectangle.Left < clipRect.Left)
    {
        scrollRect2.Left += clipRect.Left - targetRectangle.Left;
        targetRectangle.Left = clipRect.Left;
    }
    if (targetRectangle.Top < clipRect.Top)
    {
        scrollRect2.Top += clipRect.Top - targetRectangle.Top;
        targetRectangle.Top = clipRect.Top;
    }
    if (targetRectangle.Right > clipRect.Right)
    {
        scrollRect2.Right -= targetRectangle.Right - clipRect.Right;
        targetRectangle.Right = clipRect.Right;
    }
    if (targetRectangle.Bottom > clipRect.Bottom)
    {
        scrollRect2.Bottom -= targetRectangle.Bottom - clipRect.Bottom;
        targetRectangle.Bottom = clipRect.Bottom;
    }

    // clip scroll rect to clipregion => S3
    SMALL_RECT scrollRect3 = scrollRect;
    scrollRect3.Left = std::max(scrollRect3.Left, clipRect.Left);
    scrollRect3.Top = std::max(scrollRect3.Top, clipRect.Top);
    scrollRect3.Right = std::min(scrollRect3.Right, clipRect.Right);
    scrollRect3.Bottom = std::min(scrollRect3.Bottom, clipRect.Bottom);

    // if scroll rect doesn't intersect clip region, return.
    if (scrollRect3.Bottom < scrollRect3.Top || scrollRect3.Right < scrollRect3.Left)
    {
        return;
    }

    // if target rectangle doesn't intersect screen buffer, skip scrolling part.
    if (!(targetRectangle.Bottom < targetRectangle.Top || targetRectangle.Right < targetRectangle.Left))
    {
        // if we can, don't use intermediate scroll region buffer.  do this
        // by figuring out fill rectangle.  NOTE: this code will only work
        // if _CopyRectangle copies from low memory to high memory (otherwise
        // we would overwrite the scroll region before reading it).

        if (scrollRect2.Right == targetRectangle.Right &&
            scrollRect2.Left == targetRectangle.Left && scrollRect2.Top > targetRectangle.Top && scrollRect2.Top < targetRectangle.Bottom)
        {
            const COORD targetPoint{ targetRectangle.Left, targetRectangle.Top };

            if (scrollRect2.Right == (SHORT)(bufferSize.X - 1) &&
                scrollRect2.Left == 0 && scrollRect2.Bottom == (SHORT)(bufferSize.Y - 1) && scrollRect2.Top == 1)
            {
                ScrollEntireScreen(screenInfo, (SHORT)(scrollRect2.Top - targetRectangle.Top));
            }
            else
            {
                _CopyRectangle(screenInfo, Viewport::FromInclusive(scrollRect2), targetPoint);
            }

            SMALL_RECT fillRect;
            fillRect.Left = targetRectangle.Left;
            fillRect.Right = targetRectangle.Right;
            fillRect.Top = (SHORT)(targetRectangle.Bottom + 1);
            fillRect.Bottom = scrollRect.Bottom;
            if (fillRect.Top < clipRect.Top)
            {
                fillRect.Top = clipRect.Top;
            }

            if (fillRect.Bottom > clipRect.Bottom)
            {
                fillRect.Bottom = clipRect.Bottom;
            }
            screenInfo.WriteRect(OutputCellIterator(fillWithChar, fillWithAttrs), Viewport::FromInclusive(fillRect));

            ScrollScreen(screenInfo, &scrollRect2, &fillRect, targetPoint);
        }

        // if no overlap, don't need intermediate copy
        else if (scrollRect3.Right < targetRectangle.Left ||
                 scrollRect3.Left > targetRectangle.Right ||
                 scrollRect3.Top > targetRectangle.Bottom ||
                 scrollRect3.Bottom < targetRectangle.Top)
        {
            const COORD TargetPoint{ targetRectangle.Left, targetRectangle.Top };
            _CopyRectangle(screenInfo, Viewport::FromInclusive(scrollRect2), TargetPoint);

            screenInfo.WriteRect(OutputCellIterator(fillWithChar, fillWithAttrs), Viewport::FromInclusive(scrollRect3));

            ScrollScreen(screenInfo, &scrollRect2, &scrollRect3, TargetPoint);
        }

        // for the case where the source and target rectangles overlap, we copy the source rectangle, fill it, then copy it to the target.
        else
        {
            // Backup cells from screen
            const auto cells = screenInfo.ReadRect(Viewport::FromInclusive(scrollRect2));

            // Fill cells in screen
            screenInfo.WriteRect(OutputCellIterator(fillWithChar, fillWithAttrs), Viewport::FromInclusive(scrollRect3));

            // Replaced backed up cells at target
            const COORD targetPoint{ targetRectangle.Left, targetRectangle.Top };
            screenInfo.WriteRect(cells, targetPoint);

            // update regions on screen.
            ScrollScreen(screenInfo, &scrollRect2, &scrollRect3, targetPoint);
        }
    }
    else
    {
        // Do fill.
        screenInfo.WriteRect(OutputCellIterator(fillWithChar, fillWithAttrs), Viewport::FromInclusive(scrollRect3));
    }
}

void SetActiveScreenBuffer(SCREEN_INFORMATION& screenInfo)
{
    CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    gci.pCurrentScreenBuffer = &screenInfo;

    // initialize cursor
    screenInfo.GetTextBuffer().GetCursor().SetIsOn(false);

    // set font
    screenInfo.RefreshFontWithRenderer();

    // Empty input buffer.
    gci.pInputBuffer->FlushAllButKeys();

    // Set window size.
    screenInfo.PostUpdateWindowSize();

    gci.ConsoleIme.RefreshAreaAttributes();

    // Write data to screen.
    WriteToScreen(screenInfo, screenInfo.GetViewport());
}

// TODO: MSFT 9450717 This should join the ProcessList class when CtrlEvents become moved into the server. https://osgvsowi/9450717
void CloseConsoleProcessState()
{
    const CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    // If there are no connected processes, sending control events is pointless as there's no one do send them to. In
    // this case we'll just exit conhost.

    // N.B. We can get into this state when a process has a reference to the console but hasn't connected. For example,
    //      when it's created suspended and never resumed.
    if (gci.ProcessHandleList.IsEmpty())
    {
        ServiceLocator::RundownAndExit(STATUS_SUCCESS);
    }

    HandleCtrlEvent(CTRL_CLOSE_EVENT);

    // Jiggle the handle: (see MSFT:19419231)
    // When we call this function, we'll only actually close the console once
    //      we're totally unlocked. If our caller has the console locked, great,
    //      we'll displatch the ctrl event once they unlock. However, if they're
    //      not running under lock (eg PtySignalInputThread::_GetData), then the
    //      ctrl event will never actually get dispatched.
    // So, lock and unlock here, to make sure the ctrl event gets handled.
    LockConsole();
    auto Unlock = wil::scope_exit([&] { UnlockConsole(); });
}
