/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "conimeinfo.h"

#include "..\interactivity\inc\ServiceLocator.hpp"

ConversionAreaBufferInfo::ConversionAreaBufferInfo(_In_ COORD const coordBufferSize) :
    coordCaBuffer(coordBufferSize),
    rcViewCaWindow({ 0 }),
    coordConView({ 0 })
{
}

ConversionAreaInfo::ConversionAreaInfo(_In_ COORD const coordBufferSize,
                                       _In_ SCREEN_INFORMATION* const pScreenInfo,
                                       _In_ ConstructorGuard /*guard*/) :
    CaInfo(coordBufferSize),
    _fIsHidden(true),
    ScreenBuffer(pScreenInfo)
{

}

// Routine Description:
// - Instantiates a new instance of the ConversionAreaInfo class in a way that can return error codes.
// Arguments:
// - convAreaInfo - reference to the unique_ptr that will hold the newly created object.
// Return value:
// - NTSTATUS value. Normally STATUS_SUCCESSFUL if OK. Use appropriate checking macros.
NTSTATUS ConversionAreaInfo::s_CreateInstance(_Inout_ std::unique_ptr<ConversionAreaInfo>& convAreaInfo)
{
    const CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    NTSTATUS Status = STATUS_SUCCESS;

    if (gci.CurrentScreenBuffer == nullptr)
    {
        Status = STATUS_UNSUCCESSFUL;
    }

    if (NT_SUCCESS(Status))
    {
        // Each conversion area represents one row of the current screen buffer.
        COORD coordCaBuffer = gci.CurrentScreenBuffer->GetScreenBufferSize();
        coordCaBuffer.Y = 1;

        COORD dwWindowSize;
        dwWindowSize.X = gci.CurrentScreenBuffer->GetScreenWindowSizeX();
        dwWindowSize.Y = gci.CurrentScreenBuffer->GetScreenWindowSizeY();

        CHAR_INFO Fill;
        Fill.Attributes = gci.CurrentScreenBuffer->GetAttributes().GetLegacyAttributes();

        CHAR_INFO PopupFill;
        PopupFill.Attributes = gci.CurrentScreenBuffer->GetPopupAttributes()->GetLegacyAttributes();

        const FontInfo* const pfiFont = gci.CurrentScreenBuffer->TextInfo->GetCurrentFont();

        SCREEN_INFORMATION* pNewScreen;
        Status = SCREEN_INFORMATION::CreateInstance(dwWindowSize,
                                                    pfiFont,
                                                    coordCaBuffer,
                                                    Fill,
                                                    PopupFill,
                                                    0, // cursor has no height because it won't be rendered for conversion areas.
                                                    &pNewScreen);

        if (NT_SUCCESS(Status))
        {
            // Suppress painting notifications for modifying a conversion area cursor as they're not actually rendered.
            pNewScreen->TextInfo->GetCursor()->SetIsConversionArea(TRUE);

            convAreaInfo = std::make_unique<ConversionAreaInfo>(coordCaBuffer, pNewScreen, ConstructorGuard{});

            Status = NT_TESTNULL(convAreaInfo.get());
            if (NT_SUCCESS(Status))
            {
                pNewScreen->ConvScreenInfo = convAreaInfo.get();
            }
        }

        if (!NT_SUCCESS(Status))
        {
            delete pNewScreen;
        }
    }

    return Status;
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
void ConversionAreaInfo::SetHidden(_In_ bool const fIsHidden)
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
    TextAttribute const Attributes = gci.CurrentScreenBuffer->GetAttributes();

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
NTSTATUS ConsoleImeInfo::AddConversionArea()
{
    std::unique_ptr<ConversionAreaInfo> convAreaInfo;
    NTSTATUS Status = ConversionAreaInfo::s_CreateInstance(convAreaInfo);
    if (NT_SUCCESS(Status))
    {
        try
        {
            ConvAreaCompStr.push_back(std::move(convAreaInfo));
        }
        catch (std::bad_alloc)
        {
            Status = STATUS_NO_MEMORY;
        }
        catch (...)
        {
            Status = wil::ResultFromCaughtException();
        }

        if (NT_SUCCESS(Status))
        {
            RefreshAreaAttributes();
        }
    }

    return Status;
}
