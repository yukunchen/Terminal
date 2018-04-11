/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "conimeinfo.h"

#include "..\interactivity\inc\ServiceLocator.hpp"

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

ConversionAreaInfo::~ConversionAreaInfo()
{
    if (ScreenBuffer != nullptr)
    {
        delete ScreenBuffer;
    }
}

ConsoleImeInfo::ConsoleImeInfo() :
    CompStrData(nullptr),
    SavedCursorVisible(FALSE)
{

}

ConsoleImeInfo::~ConsoleImeInfo()
{
    if (CompStrData != nullptr)
    {
        delete[] CompStrData;
    }
}

// Routine Description:
// - Copies default attribute (color) data from the active screen buffer into the conversion area buffers
// Arguments:
// - <none> (Uses global current screen buffer)
// Return Value:
// - <none>
void ConsoleImeInfo::RefreshAreaAttributes()
{
    const CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    TextAttribute const Attributes = gci.GetActiveOutputBuffer().GetAttributes();

    for (unsigned int i = 0; i < ConvAreaCompStr.size(); ++i)
    {
        ConvAreaCompStr[i]->ScreenBuffer->SetAttributes(Attributes);
    }
}

// Routine Description:
// - Adds another conversion area to the current list of conversion areas (lines) available for IME candidate text
// Arguments:
// - <none>
// Return Value:
// - Status successful or appropriate NTSTATUS response.
[[nodiscard]]
NTSTATUS ConsoleImeInfo::AddConversionArea()
{
    const CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();

    COORD bufferSize = gci.GetActiveOutputBuffer().GetScreenBufferSize();
    bufferSize.Y = 1;

    COORD windowSize;
    windowSize.X = gci.GetActiveOutputBuffer().GetScreenWindowSizeX();
    windowSize.Y = gci.GetActiveOutputBuffer().GetScreenWindowSizeY();

    CHAR_INFO fill;
    fill.Attributes = gci.GetActiveOutputBuffer().GetAttributes().GetLegacyAttributes();

    CHAR_INFO popupFill;
    popupFill.Attributes = gci.GetActiveOutputBuffer().GetPopupAttributes()->GetLegacyAttributes();

    const FontInfo& fontInfo = gci.GetActiveOutputBuffer().GetTextBuffer().GetCurrentFont();

    try
    {
        std::unique_ptr<ConversionAreaInfo> convAreaInfo = std::make_unique<ConversionAreaInfo>(bufferSize,
                                                                                                windowSize,
                                                                                                fill,
                                                                                                popupFill,
                                                                                                fontInfo);
        THROW_HR_IF(E_OUTOFMEMORY, convAreaInfo == nullptr);

        ConvAreaCompStr.push_back(std::move(convAreaInfo));
    }
    catch (...)
    {
        return NTSTATUS_FROM_HRESULT(wil::ResultFromCaughtException());
    }

    RefreshAreaAttributes();

    return STATUS_SUCCESS;
}
