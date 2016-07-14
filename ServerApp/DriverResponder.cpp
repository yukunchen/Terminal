#include "stdafx.h"
#include "DriverResponder.h"

// DriverResponder* g_Responder;

DriverResponder::DriverResponder()
{
    // g_Responder = this;
    _pConsoleHost = new ConsoleHost();
}


DriverResponder::~DriverResponder()
{
    if (_pConsoleHost != nullptr)
    {
        delete _pConsoleHost;
    }
}

// bool g_hasInput = false;

void DriverResponder::NotifyInput()
{
    // g_hasInput = true;
    _pService->NotifyInputReadWait();
}

// class BogusInputObject : public IConsoleInputObject
// {
//     DWORD Foo = 0xB0B0B0B0;
// };

// class BogusOutputObject : public IConsoleOutputObject
// {
//     DWORD Bar = 0xF0F0F0F0;
// };

// BogusInputObject* pCurrentInput;
// BogusOutputObject* pCurrentOutput;


// thread* g_pThread;

NTSTATUS DriverResponder::CreateInitialObjects(_Out_ IConsoleInputObject** const ppInputObject,
                                                 _Out_ IConsoleOutputObject** const ppOutputObject)
{
    // BogusInputObject* const pNewInput = new BogusInputObject();
    // BogusOutputObject* const pNewOutput = new BogusOutputObject();

    NTSTATUS Status = _pConsoleHost->CreateIOBuffers(ppInputObject, ppOutputObject);
    _pConsoleHost->CreateWindowThread();
    // *ppInputObject = pNewInput;
    // *ppOutputObject = pNewOutput;

    // pCurrentInput = *ppInputObject;
    // pCurrentOutput = *ppOutputObject;

    // g_pThread = new thread(GoWindow);

    return S_OK;
}

NTSTATUS DriverResponder::GetConsoleInputCodePageImpl(_Out_ ULONG* const pCodePage)
{
    *pCodePage = 437;
    return 0;
}

NTSTATUS DriverResponder::GetConsoleOutputCodePageImpl(_Out_ ULONG* const pCodePage)
{
    *pCodePage = 437;
    return 0;
}

NTSTATUS DriverResponder::GetConsoleInputModeImpl(_In_ IConsoleInputObject* const pInContext,
                                                    _Out_ ULONG* const pMode)
{
    UNREFERENCED_PARAMETER(pInContext);
    *pMode = 0;
    return 0;
}

NTSTATUS DriverResponder::GetConsoleOutputModeImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                     _Out_ ULONG* const pMode)
{
    UNREFERENCED_PARAMETER(pOutContext);
    *pMode = 0;
    return 0;
}

NTSTATUS DriverResponder::SetConsoleInputModeImpl(IConsoleInputObject* const pInContext, ULONG const Mode)
{
    UNREFERENCED_PARAMETER(pInContext);
    return 0;
}

NTSTATUS DriverResponder::SetConsoleOutputModeImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                     _In_ ULONG const Mode)
{
    UNREFERENCED_PARAMETER(pOutContext);
    return 0;
}

NTSTATUS DriverResponder::GetNumberOfConsoleInputEventsImpl(_In_ IConsoleInputObject* const pInContext,
                                                              _Out_ ULONG* const pEvents)
{
    UNREFERENCED_PARAMETER(pInContext);
    *pEvents = 0;
    return 0;
}


NTSTATUS DriverResponder::FillConsoleOutputAttributeImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                           _In_ WORD const Attribute,
                                                           _In_ DWORD const LengthToWrite,
                                                           _In_ COORD const StartingCoordinate,
                                                           _Out_ DWORD* const pCellsModified)
{
    UNREFERENCED_PARAMETER(pOutContext);
    UNREFERENCED_PARAMETER(Attribute);
    UNREFERENCED_PARAMETER(StartingCoordinate);
    *pCellsModified = LengthToWrite;
    return 0;
}

NTSTATUS DriverResponder::FillConsoleOutputCharacterAImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                            _In_ char const Character,
                                                            _In_ DWORD const LengthToWrite,
                                                            _In_ COORD const StartingCoordinate,
                                                            _Out_ DWORD* const pCellsModified)
{
    UNREFERENCED_PARAMETER(pOutContext);
    UNREFERENCED_PARAMETER(Character);
    UNREFERENCED_PARAMETER(StartingCoordinate);
    *pCellsModified = LengthToWrite;
    return 0;
}

NTSTATUS DriverResponder::FillConsoleOutputCharacterWImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                            _In_ wchar_t const Character,
                                                            _In_ DWORD const LengthToWrite,
                                                            _In_ COORD const StartingCoordinate,
                                                            _Out_ DWORD* const pCellsModified)
{
    UNREFERENCED_PARAMETER(pOutContext);
    UNREFERENCED_PARAMETER(Character);
    UNREFERENCED_PARAMETER(StartingCoordinate);
    *pCellsModified = LengthToWrite;
    return 0;
}

NTSTATUS DriverResponder::GenerateConsoleCtrlEventImpl(_In_ ULONG const ProcessGroupFilter,
                                                         _In_ ULONG const ControlEvent)
{
    UNREFERENCED_PARAMETER(ProcessGroupFilter);
    UNREFERENCED_PARAMETER(ControlEvent);
    return 0;
}

NTSTATUS DriverResponder::SetConsoleActiveScreenBufferImpl(_In_ HANDLE const NewOutContext)
{
    UNREFERENCED_PARAMETER(NewOutContext);
    return 0;
}

NTSTATUS DriverResponder::FlushConsoleInputBuffer(_In_ IConsoleInputObject* const pInContext)
{
    UNREFERENCED_PARAMETER(pInContext);
    return 0;
}

NTSTATUS DriverResponder::SetConsoleInputCodePageImpl(_In_ ULONG const CodePage)
{
    UNREFERENCED_PARAMETER(CodePage);
    return 0;
}

NTSTATUS DriverResponder::SetConsoleOutputCodePageImpl(_In_ ULONG const CodePage)
{
    UNREFERENCED_PARAMETER(CodePage);
    return 0;
}

NTSTATUS DriverResponder::GetConsoleCursorInfoImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                     _Out_ ULONG* const pCursorSize,
                                                     _Out_ BOOLEAN* const pIsVisible)
{
    UNREFERENCED_PARAMETER(pOutContext);
    *pCursorSize = 60;
    *pIsVisible = TRUE;
    return 0;
}

NTSTATUS DriverResponder::SetConsoleCursorInfoImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                     _In_ ULONG const CursorSize,
                                                     _In_ BOOLEAN const IsVisible)
{
    UNREFERENCED_PARAMETER(pOutContext);
    UNREFERENCED_PARAMETER(CursorSize);
    UNREFERENCED_PARAMETER(IsVisible);
    return 0;
}

NTSTATUS DriverResponder::GetConsoleScreenBufferInfoExImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                             _Out_ CONSOLE_SCREEN_BUFFER_INFOEX* const pScreenBufferInfoEx)
{
    UNREFERENCED_PARAMETER(pOutContext);
    pScreenBufferInfoEx->cbSize = sizeof(CONSOLE_SCREEN_BUFFER_INFOEX);
    pScreenBufferInfoEx->bFullscreenSupported = FALSE;
    pScreenBufferInfoEx->dwCursorPosition.X = 0;
    pScreenBufferInfoEx->dwCursorPosition.Y = 0;
    pScreenBufferInfoEx->dwMaximumWindowSize.X = 120;
    pScreenBufferInfoEx->dwMaximumWindowSize.Y = 30;
    pScreenBufferInfoEx->dwSize.X = 120;
    pScreenBufferInfoEx->dwSize.Y = 30;
    pScreenBufferInfoEx->srWindow.Left = 0;
    pScreenBufferInfoEx->srWindow.Top = 0;
    pScreenBufferInfoEx->srWindow.Right = 120;
    pScreenBufferInfoEx->srWindow.Bottom = 30;
    pScreenBufferInfoEx->wAttributes = 7;
    pScreenBufferInfoEx->wPopupAttributes = 9;
    pScreenBufferInfoEx->ColorTable[0] = RGB(0x0000, 0x0000, 0x0000);
    pScreenBufferInfoEx->ColorTable[1] = RGB(0x0000, 0x0000, 0x0080);
    pScreenBufferInfoEx->ColorTable[2] = RGB(0x0000, 0x0080, 0x0000);
    pScreenBufferInfoEx->ColorTable[3] = RGB(0x0000, 0x0080, 0x0080);
    pScreenBufferInfoEx->ColorTable[4] = RGB(0x0080, 0x0000, 0x0000);
    pScreenBufferInfoEx->ColorTable[5] = RGB(0x0080, 0x0000, 0x0080);
    pScreenBufferInfoEx->ColorTable[6] = RGB(0x0080, 0x0080, 0x0000);
    pScreenBufferInfoEx->ColorTable[7] = RGB(0x00C0, 0x00C0, 0x00C0);
    pScreenBufferInfoEx->ColorTable[8] = RGB(0x0080, 0x0080, 0x0080);
    pScreenBufferInfoEx->ColorTable[9] = RGB(0x0000, 0x0000, 0x00FF);
    pScreenBufferInfoEx->ColorTable[10] = RGB(0x0000, 0x00FF, 0x0000);
    pScreenBufferInfoEx->ColorTable[11] = RGB(0x0000, 0x00FF, 0x00FF);
    pScreenBufferInfoEx->ColorTable[12] = RGB(0x00FF, 0x0000, 0x0000);
    pScreenBufferInfoEx->ColorTable[13] = RGB(0x00FF, 0x0000, 0x00FF);
    pScreenBufferInfoEx->ColorTable[14] = RGB(0x00FF, 0x00FF, 0x0000);
    pScreenBufferInfoEx->ColorTable[15] = RGB(0x00FF, 0x00FF, 0x00FF);
    return 0;
}

NTSTATUS DriverResponder::SetConsoleScreenBufferInfoExImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                             _In_ const CONSOLE_SCREEN_BUFFER_INFOEX* const pScreenBufferInfoEx)
{
    UNREFERENCED_PARAMETER(pOutContext);
    UNREFERENCED_PARAMETER(pScreenBufferInfoEx);
    return 0;
}

NTSTATUS DriverResponder::SetConsoleScreenBufferSizeImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                           _In_ const COORD* const pSize)
{
    UNREFERENCED_PARAMETER(pOutContext);
    UNREFERENCED_PARAMETER(pSize);
    return 0;
}

NTSTATUS DriverResponder::SetConsoleCursorPositionImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                         _In_ const COORD* const pCursorPosition)
{
    UNREFERENCED_PARAMETER(pOutContext);
    UNREFERENCED_PARAMETER(pCursorPosition);
    return 0;
}

NTSTATUS DriverResponder::GetLargestConsoleWindowSizeImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                            _Out_ COORD* const pSize)
{
    UNREFERENCED_PARAMETER(pOutContext);
    pSize->X = 120;
    pSize->Y = 30;
    return 0;
}

NTSTATUS DriverResponder::ScrollConsoleScreenBufferAImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                           _In_ const SMALL_RECT* const pSourceRectangle,
                                                           _In_ const COORD* const pTargetOrigin,
                                                           _In_opt_ const SMALL_RECT* const pTargetClipRectangle,
                                                           _In_ const CHAR_INFO* const pFill)
{
    UNREFERENCED_PARAMETER(pOutContext);
    UNREFERENCED_PARAMETER(pSourceRectangle);
    UNREFERENCED_PARAMETER(pTargetOrigin);
    UNREFERENCED_PARAMETER(pTargetClipRectangle);
    UNREFERENCED_PARAMETER(pFill);
    return 0;
}

NTSTATUS DriverResponder::ScrollConsoleScreenBufferWImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                           _In_ const SMALL_RECT* const pSourceRectangle,
                                                           _In_ const COORD* const pTargetOrigin,
                                                           _In_opt_ const SMALL_RECT* const pTargetClipRectangle,
                                                           _In_ const CHAR_INFO* const pFill)
{
    UNREFERENCED_PARAMETER(pOutContext);
    UNREFERENCED_PARAMETER(pSourceRectangle);
    UNREFERENCED_PARAMETER(pTargetOrigin);
    UNREFERENCED_PARAMETER(pTargetClipRectangle);
    UNREFERENCED_PARAMETER(pFill);
    return 0;
}

NTSTATUS DriverResponder::SetConsoleTextAttributeImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                        _In_ WORD const Attribute)
{
    UNREFERENCED_PARAMETER(pOutContext);
    UNREFERENCED_PARAMETER(Attribute);
    return 0;
}

NTSTATUS DriverResponder::SetConsoleWindowInfoImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                     _In_ BOOLEAN const IsAbsoluteRectangle,
                                                     _In_ const SMALL_RECT* const pWindowRectangle)
{
    UNREFERENCED_PARAMETER(pOutContext);
    UNREFERENCED_PARAMETER(IsAbsoluteRectangle);
    UNREFERENCED_PARAMETER(pWindowRectangle);
    return 0;
}

NTSTATUS DriverResponder::ReadConsoleOutputAttributeImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                           _In_ const COORD* const pSourceOrigin,
                                                           _Out_writes_to_(AttributeBufferLength, *pAttributeBufferWritten) WORD* const pAttributeBuffer,
                                                           _In_ ULONG const AttributeBufferLength,
                                                           _Out_ ULONG* const pAttributeBufferWritten)
{
    UNREFERENCED_PARAMETER(pOutContext);
    UNREFERENCED_PARAMETER(pSourceOrigin);
    UNREFERENCED_PARAMETER(pAttributeBuffer);
    UNREFERENCED_PARAMETER(AttributeBufferLength);
    *pAttributeBufferWritten = 0;
    return 0;
}

NTSTATUS DriverResponder::ReadConsoleOutputCharacterAImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                            _In_ const COORD* const pSourceOrigin,
                                                            _Out_writes_to_(TextBufferLength, *pTextBufferWritten) char* const pTextBuffer,
                                                            _In_ ULONG const TextBufferLength,
                                                            _Out_ ULONG* const pTextBufferWritten)
{
    UNREFERENCED_PARAMETER(pOutContext);
    UNREFERENCED_PARAMETER(pSourceOrigin);
    UNREFERENCED_PARAMETER(pTextBuffer);
    UNREFERENCED_PARAMETER(TextBufferLength);
    *pTextBufferWritten = 0;
    return 0;
}

NTSTATUS DriverResponder::ReadConsoleOutputCharacterWImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                            _In_ const COORD* const pSourceOrigin,
                                                            _Out_writes_to_(TextBufferLength, *pTextBufferWritten) wchar_t* const pTextBuffer,
                                                            _In_ ULONG const TextBufferLength,
                                                            _Out_ ULONG* const pTextBufferWritten)
{
    UNREFERENCED_PARAMETER(pOutContext);
    UNREFERENCED_PARAMETER(pSourceOrigin);
    UNREFERENCED_PARAMETER(pTextBuffer);
    UNREFERENCED_PARAMETER(TextBufferLength);
    *pTextBufferWritten = 0;
    return 0;
}


NTSTATUS DriverResponder::WriteConsoleOutputAImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                    _In_reads_(pTextBufferSize->X * pTextBufferSize->Y) const CHAR_INFO* const pTextBuffer,
                                                    _In_ const COORD* const pTextBufferSize,
                                                    _In_ const COORD* const pTextBufferSourceOrigin,
                                                    _In_ const SMALL_RECT* const pTargetRectangle,
                                                    _Out_ SMALL_RECT* const pAffectedRectangle)
{
    UNREFERENCED_PARAMETER(pOutContext);
    UNREFERENCED_PARAMETER(pTextBuffer);
    UNREFERENCED_PARAMETER(pTextBufferSize);
    UNREFERENCED_PARAMETER(pTextBufferSourceOrigin);
    UNREFERENCED_PARAMETER(pTargetRectangle);
    *pAffectedRectangle = { 0 };
    return 0;
}

NTSTATUS DriverResponder::WriteConsoleOutputWImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                    _In_reads_(pTextBufferSize->X * pTextBufferSize->Y) const CHAR_INFO* const pTextBuffer,
                                                    _In_ const COORD* const pTextBufferSize,
                                                    _In_ const COORD* const pTextBufferSourceOrigin,
                                                    _In_ const SMALL_RECT* const pTargetRectangle,
                                                    _Out_ SMALL_RECT* const pAffectedRectangle)
{
    UNREFERENCED_PARAMETER(pOutContext);
    UNREFERENCED_PARAMETER(pTextBuffer);
    UNREFERENCED_PARAMETER(pTextBufferSize);
    UNREFERENCED_PARAMETER(pTextBufferSourceOrigin);
    UNREFERENCED_PARAMETER(pTargetRectangle);
    *pAffectedRectangle = { 0 };
    return 0;
}

NTSTATUS DriverResponder::WriteConsoleOutputAttributeImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                            _In_reads_(AttributeBufferLength) const WORD* const pAttributeBuffer,
                                                            _In_ ULONG const AttributeBufferLength,
                                                            _In_ const COORD* const pTargetOrigin,
                                                            _Out_ ULONG* const pAttributeBufferRead)
{
    UNREFERENCED_PARAMETER(pOutContext);
    UNREFERENCED_PARAMETER(pAttributeBuffer);
    UNREFERENCED_PARAMETER(pTargetOrigin);
    *pAttributeBufferRead = AttributeBufferLength;
    return 0;
}

NTSTATUS DriverResponder::WriteConsoleOutputCharacterAImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                             _In_reads_(TextBufferLength) const char* const pTextBuffer,
                                                             _In_ ULONG const TextBufferLength,
                                                             _In_ const COORD* const pTargetOrigin,
                                                             _Out_ ULONG* const pTextBufferRead)
{
    UNREFERENCED_PARAMETER(pOutContext);
    UNREFERENCED_PARAMETER(pTextBuffer);
    UNREFERENCED_PARAMETER(pTargetOrigin);
    *pTextBufferRead = TextBufferLength;
    return 0;
}

NTSTATUS DriverResponder::WriteConsoleOutputCharacterWImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                             _In_reads_(TextBufferLength) const wchar_t* const pTextBuffer,
                                                             _In_ ULONG const TextBufferLength,
                                                             _In_ const COORD* const pTargetOrigin,
                                                             _Out_ ULONG* const pTextBufferRead)
{
    UNREFERENCED_PARAMETER(pOutContext);
    UNREFERENCED_PARAMETER(pTextBuffer);
    UNREFERENCED_PARAMETER(pTargetOrigin);
    *pTextBufferRead = TextBufferLength;
    return 0;
}

NTSTATUS DriverResponder::ReadConsoleOutputA(_In_ IConsoleOutputObject* const pOutContext,
                                               _Out_writes_(pTextBufferSize->X * pTextBufferSize->Y) CHAR_INFO* const pTextBuffer,
                                               _In_ const COORD* const pTextBufferSize,
                                               _In_ const COORD* const pTextBufferTargetOrigin,
                                               _In_ const SMALL_RECT* const pSourceRectangle,
                                               _Out_ SMALL_RECT* const pReadRectangle)
{
    UNREFERENCED_PARAMETER(pOutContext);
    UNREFERENCED_PARAMETER(pTextBuffer);
    UNREFERENCED_PARAMETER(pTextBufferSize);
    UNREFERENCED_PARAMETER(pTextBufferTargetOrigin);
    UNREFERENCED_PARAMETER(pSourceRectangle);
    *pReadRectangle = { 0, 0, -1, -1 }; // because inclusive rects, an empty has right and bottom at -1
    return 0;
}

NTSTATUS DriverResponder::ReadConsoleOutputW(_In_ IConsoleOutputObject* const pOutContext,
                                               _Out_writes_(pTextBufferSize->X * pTextBufferSize->Y) CHAR_INFO* const pTextBuffer,
                                               _In_ const COORD* const pTextBufferSize,
                                               _In_ const COORD* const pTextBufferTargetOrigin,
                                               _In_ const SMALL_RECT* const pSourceRectangle,
                                               _Out_ SMALL_RECT* const pReadRectangle)
{
    UNREFERENCED_PARAMETER(pOutContext);
    UNREFERENCED_PARAMETER(pTextBuffer);
    UNREFERENCED_PARAMETER(pTextBufferSize);
    UNREFERENCED_PARAMETER(pTextBufferTargetOrigin);
    UNREFERENCED_PARAMETER(pSourceRectangle);
    *pReadRectangle = { 0, 0, -1, -1 }; // because inclusive rects, an empty has right and bottom at -1
    return 0;
}

NTSTATUS DriverResponder::GetConsoleTitleAImpl(_Out_writes_(TextBufferSize) char* const pTextBuffer,
                                                 _In_ ULONG const TextBufferSize)
{
    if (TextBufferSize > 0)
    {
        *pTextBuffer = '\0';
    }
    return 0;
}

NTSTATUS DriverResponder::GetConsoleTitleWImpl(_Out_writes_(TextBufferSize) wchar_t* const pTextBuffer,
                                                 _In_ ULONG const TextBufferSize)
{
    if (TextBufferSize > 0)
    {
        *pTextBuffer = '\0';
    }
    return 0;
}

NTSTATUS DriverResponder::GetConsoleOriginalTitleAImpl(_Out_writes_(TextBufferSize) char* const pTextBuffer,
                                                         _In_ ULONG const TextBufferSize)
{
    if (TextBufferSize > 0)
    {
        *pTextBuffer = '\0';
    }
    return 0;
}

NTSTATUS DriverResponder::GetConsoleOriginalTitleWImpl(_Out_writes_(TextBufferSize) char* const pTextBuffer,
                                                         _In_ ULONG const TextBufferSize)
{
    if (TextBufferSize > 0)
    {
        *pTextBuffer = '\0';
    }
    return 0;
}

NTSTATUS DriverResponder::SetConsoleTitleAImpl(_In_reads_(TextBufferSize) char* const pTextBuffer,
                                                 _In_ ULONG const TextBufferSize)
{
    UNREFERENCED_PARAMETER(pTextBuffer);
    UNREFERENCED_PARAMETER(TextBufferSize);
    return 0;
}

NTSTATUS DriverResponder::SetConsoleTitleWImpl(_In_reads_(TextBufferSize) wchar_t* const pTextBuffer,
                                                 _In_ ULONG const TextBufferSize)
{
    UNREFERENCED_PARAMETER(pTextBuffer);
    UNREFERENCED_PARAMETER(TextBufferSize);
    return 0;
}

NTSTATUS DriverResponder::GetNumberOfConsoleMouseButtonsImpl(_Out_ ULONG* const pButtons)
{
    *pButtons = 2;
    return 0;
}

NTSTATUS DriverResponder::GetConsoleFontSizeImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                   _In_ DWORD const FontIndex,
                                                   _Out_ COORD* const pFontSize)
{
    UNREFERENCED_PARAMETER(pOutContext);
    UNREFERENCED_PARAMETER(FontIndex);
    pFontSize->X = 8;
    pFontSize->Y = 12;
    return 0;
}

// driver will pare down for non-Ex method
NTSTATUS DriverResponder::GetCurrentConsoleFontExImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                        _In_ BOOLEAN const IsForMaximumWindowSize,
                                                        _Out_ CONSOLE_FONT_INFOEX* const pConsoleFontInfoEx)
{


    UNREFERENCED_PARAMETER(pOutContext);
    UNREFERENCED_PARAMETER(IsForMaximumWindowSize);
    pConsoleFontInfoEx->cbSize = sizeof(CONSOLE_FONT_INFOEX);
    pConsoleFontInfoEx->dwFontSize.X = 8;
    pConsoleFontInfoEx->dwFontSize.Y = 12;
    pConsoleFontInfoEx->FaceName[0] = L'\0';
    pConsoleFontInfoEx->FontFamily = 0;
    pConsoleFontInfoEx->FontWeight = 0;
    pConsoleFontInfoEx->nFont = 0;
    return 0;
}

NTSTATUS DriverResponder::SetConsoleDisplayModeImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                      _In_ ULONG const Flags,
                                                      _Out_ COORD* const pNewScreenBufferSize)
{
    UNREFERENCED_PARAMETER(pOutContext);
    UNREFERENCED_PARAMETER(Flags);
    pNewScreenBufferSize->X = 120;
    pNewScreenBufferSize->Y = 30;
    return 0;
}

NTSTATUS DriverResponder::GetConsoleDisplayModeImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                      _Out_ ULONG* const pFlags)
{
    UNREFERENCED_PARAMETER(pOutContext);
    *pFlags = 0;
    return 0;
}

NTSTATUS DriverResponder::AddConsoleAliasAImpl(_In_reads_(SourceBufferLength) const char* const pSourceBuffer,
                                                 _In_ ULONG const SourceBufferLength,
                                                 _In_reads_(TargetBufferLength) const char* const pTargetBuffer,
                                                 _In_ ULONG const TargetBufferLength,
                                                 _In_reads_(ExeNameBufferLength) const char* const pExeNameBuffer,
                                                 _In_ ULONG const ExeNameBufferLength)
{
    UNREFERENCED_PARAMETER(pSourceBuffer);
    UNREFERENCED_PARAMETER(SourceBufferLength);
    UNREFERENCED_PARAMETER(pTargetBuffer);
    UNREFERENCED_PARAMETER(TargetBufferLength);
    UNREFERENCED_PARAMETER(pExeNameBuffer);
    UNREFERENCED_PARAMETER(ExeNameBufferLength);
    return 0;
}

NTSTATUS DriverResponder::AddConsoleAliasWImpl(_In_reads_(SourceBufferLength) const wchar_t* const pSourceBuffer,
                                                 _In_ ULONG const SourceBufferLength,
                                                 _In_reads_(TargetBufferLength) const wchar_t* const pTargetBuffer,
                                                 _In_ ULONG const TargetBufferLength,
                                                 _In_reads_(ExeNameBufferLength) const wchar_t* const pExeNameBuffer,
                                                 _In_ ULONG const ExeNameBufferLength)
{
    UNREFERENCED_PARAMETER(pSourceBuffer);
    UNREFERENCED_PARAMETER(SourceBufferLength);
    UNREFERENCED_PARAMETER(pTargetBuffer);
    UNREFERENCED_PARAMETER(TargetBufferLength);
    UNREFERENCED_PARAMETER(pExeNameBuffer);
    UNREFERENCED_PARAMETER(ExeNameBufferLength);
    return 0;
}

NTSTATUS DriverResponder::GetConsoleAliasAImpl(_In_reads_(SourceBufferLength) const char* const pSourceBuffer,
                                                 _In_ ULONG const SourceBufferLength,
                                                 _Out_writes_(TargetBufferLength) char* const pTargetBuffer,
                                                 _In_ ULONG const TargetBufferLength,
                                                 _In_reads_(ExeNameBufferLength) const char* const pExeNameBuffer,
                                                 _In_ ULONG const ExeNameBufferLength)
{
    UNREFERENCED_PARAMETER(pSourceBuffer);
    UNREFERENCED_PARAMETER(SourceBufferLength);
    UNREFERENCED_PARAMETER(pTargetBuffer);
    UNREFERENCED_PARAMETER(TargetBufferLength);
    UNREFERENCED_PARAMETER(pExeNameBuffer);
    UNREFERENCED_PARAMETER(ExeNameBufferLength);
    return 0;
}

NTSTATUS DriverResponder::GetConsoleAliasWImpl(_In_reads_(SourceBufferLength) const wchar_t* const pSourceBuffer,
                                                 _In_ ULONG const SourceBufferLength,
                                                 _Out_writes_(TargetBufferLength) wchar_t* const pTargetBuffer,
                                                 _In_ ULONG const TargetBufferLength,
                                                 _In_reads_(ExeNameBufferLength) const wchar_t* const pExeNameBuffer,
                                                 _In_ ULONG const ExeNameBufferLength)
{
    UNREFERENCED_PARAMETER(pSourceBuffer);
    UNREFERENCED_PARAMETER(SourceBufferLength);
    UNREFERENCED_PARAMETER(pTargetBuffer);
    UNREFERENCED_PARAMETER(TargetBufferLength);
    UNREFERENCED_PARAMETER(pExeNameBuffer);
    UNREFERENCED_PARAMETER(ExeNameBufferLength);
    return 0;
}

NTSTATUS DriverResponder::GetConsoleAliasesLengthAImpl(_In_reads_(ExeNameBufferLength) const char* const pExeNameBuffer,
                                                         _In_ ULONG const ExeNameBufferLength,
                                                         _Out_ ULONG* const pAliasesBufferRequired)
{
    UNREFERENCED_PARAMETER(pExeNameBuffer);
    UNREFERENCED_PARAMETER(ExeNameBufferLength);
    *pAliasesBufferRequired = 0;
    return 0;
}

NTSTATUS DriverResponder::GetConsoleAliasesLengthWImpl(_In_reads_(ExeNameBufferLength) const wchar_t* const pExeNameBuffer,
                                                         _In_ ULONG const ExeNameBufferLength,
                                                         _Out_ ULONG* const pAliasesBufferRequired)
{
    UNREFERENCED_PARAMETER(pExeNameBuffer);
    UNREFERENCED_PARAMETER(ExeNameBufferLength);
    *pAliasesBufferRequired = 0;
    return 0;
}

NTSTATUS DriverResponder::GetConsoleAliasExesLengthAImpl(_Out_ ULONG* const pAliasExesBufferRequired)
{
    *pAliasExesBufferRequired = 0;
    return 0;
}

NTSTATUS DriverResponder::GetConsoleAliasExesLengthWImpl(_Out_ ULONG* const pAliasExesBufferRequired)
{
    *pAliasExesBufferRequired = 0;
    return 0;
}

NTSTATUS DriverResponder::GetConsoleAliasesAImpl(_In_reads_(ExeNameBufferLength) const char* const pExeNameBuffer,
                                                   _In_ ULONG const ExeNameBufferLength,
                                                   _Out_writes_(AliasBufferLength) char* const pAliasBuffer,
                                                   _In_ ULONG const AliasBufferLength)
{
    UNREFERENCED_PARAMETER(pExeNameBuffer);
    UNREFERENCED_PARAMETER(ExeNameBufferLength);
    UNREFERENCED_PARAMETER(pAliasBuffer);
    UNREFERENCED_PARAMETER(AliasBufferLength);
    return 0;
}

NTSTATUS DriverResponder::GetConsoleAliasesWImpl(_In_reads_(ExeNameBufferLength) const wchar_t* const pExeNameBuffer,
                                                   _In_ ULONG const ExeNameBufferLength,
                                                   _Out_writes_(AliasBufferLength) wchar_t* const pAliasBuffer,
                                                   _In_ ULONG const AliasBufferLength)
{
    UNREFERENCED_PARAMETER(pExeNameBuffer);
    UNREFERENCED_PARAMETER(ExeNameBufferLength);
    UNREFERENCED_PARAMETER(pAliasBuffer);
    UNREFERENCED_PARAMETER(AliasBufferLength);
    return 0;
}

NTSTATUS DriverResponder::GetConsoleAliasExesAImpl(_Out_writes_(AliasExesBufferLength) char* const pAliasExesBuffer,
                                                     _In_ ULONG const AliasExesBufferLength)
{
    UNREFERENCED_PARAMETER(pAliasExesBuffer);
    UNREFERENCED_PARAMETER(AliasExesBufferLength);
    return 0;
}

NTSTATUS DriverResponder::GetConsoleAliasExesWImpl(_Out_writes_(AliasExesBufferLength) wchar_t* const pAliasExesBuffer,
                                                     _In_ ULONG const AliasExesBufferLength)
{
    UNREFERENCED_PARAMETER(pAliasExesBuffer);
    UNREFERENCED_PARAMETER(AliasExesBufferLength);
    return 0;
}

NTSTATUS DriverResponder::GetConsoleWindowImpl(_Out_ HWND* const pHwnd)
{
    *pHwnd = (HWND)INVALID_HANDLE_VALUE; // sure, why not.
    return 0;
}

NTSTATUS DriverResponder::GetConsoleSelectionInfoImpl(_Out_ CONSOLE_SELECTION_INFO* const pConsoleSelectionInfo)
{
    pConsoleSelectionInfo->dwFlags = 0;
    pConsoleSelectionInfo->dwSelectionAnchor.X = 0;
    pConsoleSelectionInfo->dwSelectionAnchor.Y = 0;
    pConsoleSelectionInfo->srSelection.Top = 0;
    pConsoleSelectionInfo->srSelection.Left = 0;
    pConsoleSelectionInfo->srSelection.Bottom = 0;
    pConsoleSelectionInfo->srSelection.Right = 0;
    return 0;
}

NTSTATUS DriverResponder::GetConsoleHistoryInfoImpl(_Out_ CONSOLE_HISTORY_INFO* const pConsoleHistoryInfo)
{
    pConsoleHistoryInfo->cbSize = sizeof(CONSOLE_HISTORY_INFO);
    pConsoleHistoryInfo->dwFlags = 0;
    pConsoleHistoryInfo->HistoryBufferSize = 20;
    pConsoleHistoryInfo->NumberOfHistoryBuffers = 5;
    return 0;
}

NTSTATUS DriverResponder::SetConsoleHistoryInfoImpl(_In_ const CONSOLE_HISTORY_INFO* const pConsoleHistoryInfo)
{
    UNREFERENCED_PARAMETER(pConsoleHistoryInfo);
    return 0;
}

NTSTATUS DriverResponder::SetCurrentConsoleFontExImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                        _In_ BOOLEAN const IsForMaximumWindowSize,
                                                        _In_ const CONSOLE_FONT_INFOEX* const pConsoleFontInfoEx)
{
    UNREFERENCED_PARAMETER(pOutContext);
    UNREFERENCED_PARAMETER(IsForMaximumWindowSize);
    UNREFERENCED_PARAMETER(pConsoleFontInfoEx);
    return 0;
}
