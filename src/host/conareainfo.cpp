/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "conareainfo.h"

#include "_output.h"

#include "../interactivity/inc/ServiceLocator.hpp"

ConversionAreaBufferInfo::ConversionAreaBufferInfo(const COORD coordBufferSize) :
    coordCaBuffer(coordBufferSize),
    rcViewCaWindow({ 0 }),
    coordConView({ 0 })
{
}

ConversionAreaInfo::ConversionAreaInfo(const COORD bufferSize,
                                       const COORD windowSize,
                                       const CHAR_INFO fill,
                                       const CHAR_INFO popupFill,
                                       const FontInfo fontInfo) :
    CaInfo{ bufferSize },
    _fIsHidden{ true },
    ScreenBuffer{ nullptr }
{
    SCREEN_INFORMATION* pNewScreen = nullptr;
    // cursor has no height because it won't be rendered for conversion area
    NTSTATUS status = SCREEN_INFORMATION::CreateInstance(windowSize,
                                                         fontInfo,
                                                         bufferSize,
                                                         fill,
                                                         popupFill,
                                                         0,
                                                         &pNewScreen);
    if (NT_SUCCESS(status))
    {
        // Suppress painting notifications for modifying a conversion area cursor as they're not actually rendered.
        pNewScreen->GetTextBuffer().GetCursor().SetIsConversionArea(true);
        pNewScreen->ConvScreenInfo = this;
        ScreenBuffer = pNewScreen;
    }
    else
    {
        THROW_NTSTATUS(status);
    }
}

ConversionAreaInfo::~ConversionAreaInfo()
{
    if (ScreenBuffer != nullptr)
    {
        delete ScreenBuffer;
    }
}

ConversionAreaInfo::ConversionAreaInfo(ConversionAreaInfo&& other) :
    CaInfo(other.CaInfo),
    _fIsHidden(other._fIsHidden),
    ScreenBuffer(nullptr)
{
    std::swap(ScreenBuffer, other.ScreenBuffer);
}

// Routine Description:
// - Describes whether the conversion area should be drawn or should be hidden.
// Arguments:
// - <none>
// Return Value:
// - True if it should not be drawn. False if it should be drawn.
bool ConversionAreaInfo::IsHidden() const
{
    return _fIsHidden;
}

// Routine Description:
// - Sets a value describing whether the conversion area should be drawn or should be hidden.
// Arguments:
// - fIsHidden - True if it should not be drawn. False if it should be drawn.
// Return Value:
// - <none>
void ConversionAreaInfo::SetHidden(const bool fIsHidden)
{
    _fIsHidden = fIsHidden;
}

// Routine Description:
// - Clears out a conversion area
void ConversionAreaInfo::ClearArea()
{
    SetHidden(true);

    COORD Coord = { 0 };
    DWORD CharsToWrite = ScreenBuffer->GetScreenBufferSize().X;

    // TODO: This is dumb. There should be a "clear row" method.
    LOG_IF_FAILED(FillOutput(*ScreenBuffer,
                             (WCHAR)' ',
                             Coord,
                             CONSOLE_FALSE_UNICODE,    // faster than real unicode
                             &CharsToWrite));

    CharsToWrite = ScreenBuffer->GetScreenBufferSize().X;

    const CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();

    LOG_IF_FAILED(FillOutput(*ScreenBuffer,
                             gci.GetActiveOutputBuffer().GetAttributes().GetLegacyAttributes(),
                             Coord,
                             CONSOLE_ATTRIBUTE,
                             &CharsToWrite));

    Paint();
}

HRESULT ConversionAreaInfo::Resize(const COORD newSize)
{
    // attempt to resize underlying buffers
    RETURN_IF_NTSTATUS_FAILED(ScreenBuffer->ResizeScreenBuffer(newSize, FALSE));

    // store new size
    CaInfo.coordCaBuffer = newSize;

    // restrict viewport to buffer size.
    const COORD restriction = { newSize.X - 1i16, newSize.Y - 1i16 };
    CaInfo.rcViewCaWindow.Left = std::min(CaInfo.rcViewCaWindow.Left, restriction.X);
    CaInfo.rcViewCaWindow.Right = std::min(CaInfo.rcViewCaWindow.Right, restriction.X);
    CaInfo.rcViewCaWindow.Top = std::min(CaInfo.rcViewCaWindow.Top, restriction.Y);
    CaInfo.rcViewCaWindow.Bottom = std::min(CaInfo.rcViewCaWindow.Bottom, restriction.Y);

    return S_OK;
}

void ConversionAreaInfo::SetWindowInfo(const SMALL_RECT view)
{
    if (view.Left != CaInfo.rcViewCaWindow.Left ||
        view.Top != CaInfo.rcViewCaWindow.Top ||
        view.Right != CaInfo.rcViewCaWindow.Right ||
        view.Bottom != CaInfo.rcViewCaWindow.Bottom)
    {
        if (!IsHidden())
        {
            SetHidden(true);
            Paint();

            CaInfo.rcViewCaWindow = view;
            SetHidden(false);
            Paint();
        }
        else
        {
            CaInfo.rcViewCaWindow = view;
        }
    }
}

void ConversionAreaInfo::SetViewPos(const COORD pos)
{
    if (IsHidden())
    {
        CaInfo.coordConView = pos;
    }
    else
    {
        CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();

        SMALL_RECT OldRegion = CaInfo.rcViewCaWindow;;
        OldRegion.Left += CaInfo.coordConView.X;
        OldRegion.Right += CaInfo.coordConView.X;
        OldRegion.Top += CaInfo.coordConView.Y;
        OldRegion.Bottom += CaInfo.coordConView.Y;
        WriteToScreen(gci.GetActiveOutputBuffer(), OldRegion);

        CaInfo.coordConView = pos;

        SMALL_RECT NewRegion = CaInfo.rcViewCaWindow;
        NewRegion.Left += CaInfo.coordConView.X;
        NewRegion.Right += CaInfo.coordConView.X;
        NewRegion.Top += CaInfo.coordConView.Y;
        NewRegion.Bottom += CaInfo.coordConView.Y;
        WriteToScreen(gci.GetActiveOutputBuffer(), NewRegion);
    }
}

void ConversionAreaInfo::Paint() const
{
    CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    SCREEN_INFORMATION& ScreenInfo = gci.GetActiveOutputBuffer();
    const auto viewport = ScreenInfo.GetBufferViewport();

    SMALL_RECT WriteRegion;
    WriteRegion.Left = viewport.Left + CaInfo.coordConView.X + CaInfo.rcViewCaWindow.Left;
    WriteRegion.Right = WriteRegion.Left + (CaInfo.rcViewCaWindow.Right - CaInfo.rcViewCaWindow.Left);
    WriteRegion.Top = viewport.Top + CaInfo.coordConView.Y + CaInfo.rcViewCaWindow.Top;
    WriteRegion.Bottom = WriteRegion.Top + (CaInfo.rcViewCaWindow.Bottom - CaInfo.rcViewCaWindow.Top);

    if (!IsHidden())
    {
        WriteConvRegionToScreen(ScreenInfo, WriteRegion);
    }
    else
    {
        WriteToScreen(ScreenInfo, WriteRegion);
    }
}
