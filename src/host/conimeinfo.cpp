/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"
#include "conimeinfo.h"

ConversionAreaBufferInfo::ConversionAreaBufferInfo(_In_ COORD const coordBufferSize) :
    coordCaBuffer(coordBufferSize),
    rcViewCaWindow({ 0 }),
    coordConView({ 0 })
{
}

ConversionAreaInfo::ConversionAreaInfo(_In_ COORD const coordBufferSize,
                                       _In_ SCREEN_INFORMATION* const pScreenInfo) :
    CaInfo(coordBufferSize),
    _fIsHidden(true),
    ScreenBuffer(pScreenInfo)
{

}

// Routine Description:
// - Instantiates a new instance of the ConversionAreaInfo class in a way that can return error codes.
// Arguments:
// - ppInfo - Pointer to a pointer that will receive the location of the newly created object.
// Return value:
// - NTSTATUS value. Normally STATUS_SUCCESSFUL if OK. Use appropriate checking macros.
NTSTATUS ConversionAreaInfo::s_CreateInstance(_Outptr_ ConversionAreaInfo** const ppInfo)
{
    NTSTATUS Status = STATUS_SUCCESS;
    *ppInfo = nullptr;

    if (g_ciConsoleInformation.CurrentScreenBuffer == nullptr)
    {
        Status = STATUS_UNSUCCESSFUL;
    }

    if (NT_SUCCESS(Status))
    {
        // Each conversion area represents one row of the current screen buffer.
        COORD coordCaBuffer = g_ciConsoleInformation.CurrentScreenBuffer->ScreenBufferSize;
        coordCaBuffer.Y = 1;

        COORD dwWindowSize;
        dwWindowSize.X = g_ciConsoleInformation.CurrentScreenBuffer->GetScreenWindowSizeX();
        dwWindowSize.Y = g_ciConsoleInformation.CurrentScreenBuffer->GetScreenWindowSizeY();

        CHAR_INFO Fill;
        Fill.Attributes = g_ciConsoleInformation.CurrentScreenBuffer->GetAttributes().GetLegacyAttributes();

        CHAR_INFO PopupFill;
        PopupFill.Attributes = g_ciConsoleInformation.CurrentScreenBuffer->GetPopupAttributes()->GetLegacyAttributes();

        const FontInfo* const pfiFont = g_ciConsoleInformation.CurrentScreenBuffer->TextInfo->GetCurrentFont();

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

            ConversionAreaInfo* pca = new ConversionAreaInfo(coordCaBuffer, pNewScreen);

            Status = NT_TESTNULL(pca);
            if (NT_SUCCESS(Status))
            {
                pNewScreen->ConvScreenInfo = pca;
                *ppInfo = pca;
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
    while (!ConvAreaCompStr.empty())
    {
        // No throw is guaranteed for these operations on a non-empty container.
        delete ConvAreaCompStr.back();
        ConvAreaCompStr.pop_back();
    }

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
    TextAttribute const Attributes = g_ciConsoleInformation.CurrentScreenBuffer->GetAttributes();

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
    ConversionAreaInfo* pca;

    NTSTATUS Status = ConversionAreaInfo::s_CreateInstance(&pca);
    if (NT_SUCCESS(Status))
    {
        try
        {
            ConvAreaCompStr.push_back(pca);
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
