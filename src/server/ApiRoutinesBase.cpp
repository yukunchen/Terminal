/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "IApiRoutines.h"

// This exists as a base class until we're all migrated over.

NTSTATUS IApiRoutines::GetConsoleInputCodePageImpl(_Out_ ULONG* const pCodePage)
{
    assert(false);
    *pCodePage = 437;
    return 0;
}

NTSTATUS IApiRoutines::GetConsoleOutputCodePageImpl(_Out_ ULONG* const pCodePage)
{
    assert(false);
    *pCodePage = 437;
    return 0;
}

NTSTATUS IApiRoutines::GetConsoleInputModeImpl(_In_ IConsoleInputObject* const pInContext,
                                               _Out_ ULONG* const pMode)
{
    assert(false);
    UNREFERENCED_PARAMETER(pInContext);
    *pMode = 0;
    return 0;
}

NTSTATUS IApiRoutines::GetConsoleOutputModeImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                _Out_ ULONG* const pMode)
{
    assert(false);
    UNREFERENCED_PARAMETER(pOutContext);
    *pMode = 0;
    return 0;
}

NTSTATUS IApiRoutines::SetConsoleInputModeImpl(IConsoleInputObject* const pInContext, ULONG const Mode)
{
    assert(false);
    UNREFERENCED_PARAMETER(pInContext);
    return 0;
}

NTSTATUS IApiRoutines::SetConsoleOutputModeImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                _In_ ULONG const Mode)
{
    assert(false);
    UNREFERENCED_PARAMETER(pOutContext);
    return 0;
}

NTSTATUS IApiRoutines::GetNumberOfConsoleInputEventsImpl(_In_ IConsoleInputObject* const pInContext,
                                                         _Out_ ULONG* const pEvents)
{
    assert(false);
    UNREFERENCED_PARAMETER(pInContext);
    *pEvents = 0;
    return 0;
}

NTSTATUS IApiRoutines::PeekConsoleInputAImpl(_In_ IConsoleInputObject* const pInContext,
                                             _Out_writes_to_(InputRecordsBufferLength, *pRecordsWritten) INPUT_RECORD* const pInputRecordsBuffer,
                                             _In_ ULONG const InputRecordsBufferLength,
                                             _Out_ ULONG* const pRecordsWritten)
{
    assert(false);
    UNREFERENCED_PARAMETER(pInContext);
    UNREFERENCED_PARAMETER(pInputRecordsBuffer);
    UNREFERENCED_PARAMETER(InputRecordsBufferLength);
    *pRecordsWritten = 0;
    return 0;
}

NTSTATUS IApiRoutines::PeekConsoleInputWImpl(_In_ IConsoleInputObject* const pInContext,
                                             _Out_writes_to_(InputRecordsBufferLength, *pRecordsWritten) INPUT_RECORD* const pInputRecordsBuffer,
                                             _In_ ULONG const InputRecordsBufferLength,
                                             _Out_ ULONG* const pRecordsWritten)
{
    assert(false);
    UNREFERENCED_PARAMETER(pInContext);
    UNREFERENCED_PARAMETER(pInputRecordsBuffer);
    UNREFERENCED_PARAMETER(InputRecordsBufferLength);
    *pRecordsWritten = 0;
    return 0;
}

NTSTATUS IApiRoutines::ReadConsoleInputAImpl(_In_ IConsoleInputObject* const pInContext,
                                             _Out_writes_to_(InputRecordsBufferLength, *pRecordsWritten) INPUT_RECORD* const pInputRecordsBuffer,
                                             _In_ ULONG const InputRecordsBufferLength,
                                             _Out_ ULONG* const pRecordsWritten)
{
    assert(false);
    UNREFERENCED_PARAMETER(pInContext);
    UNREFERENCED_PARAMETER(pInputRecordsBuffer);
    UNREFERENCED_PARAMETER(InputRecordsBufferLength);
    *pRecordsWritten = 0;
    return 0;
}

NTSTATUS IApiRoutines::ReadConsoleInputWImpl(_In_ IConsoleInputObject* const pInContext,
                                             _Out_writes_to_(InputRecordsBufferLength, *pRecordsWritten) INPUT_RECORD* const pInputRecordsBuffer,
                                             _In_ ULONG const InputRecordsBufferLength,
                                             _Out_ ULONG* const pRecordsWritten)
{
    assert(false);
    UNREFERENCED_PARAMETER(pInContext);
    UNREFERENCED_PARAMETER(pInputRecordsBuffer);
    UNREFERENCED_PARAMETER(InputRecordsBufferLength);
    *pRecordsWritten = 0;
    return 0;
}

NTSTATUS IApiRoutines::ReadConsoleAImpl(_In_ IConsoleInputObject* const pInContext,
                                        _Out_writes_to_(TextBufferLength, *pTextBufferWritten) char* const pTextBuffer,
                                        _In_ ULONG const TextBufferLength,
                                        _Out_ ULONG* const pTextBufferWritten,
                                        _In_opt_ CONSOLE_READCONSOLE_CONTROL* const pReadControl)
{
    assert(false);
    UNREFERENCED_PARAMETER(pInContext);
    UNREFERENCED_PARAMETER(pTextBuffer);
    UNREFERENCED_PARAMETER(TextBufferLength);
    *pTextBufferWritten = 0;
    UNREFERENCED_PARAMETER(pReadControl);
    return 0;
}

NTSTATUS IApiRoutines::ReadConsoleWImpl(_In_ IConsoleInputObject* const pInContext,
                                        _Out_writes_to_(TextBufferLength, *pTextBufferWritten) wchar_t* const pTextBuffer,
                                        _In_ ULONG const TextBufferLength,
                                        _Out_ ULONG* const pTextBufferWritten,
                                        _In_opt_ CONSOLE_READCONSOLE_CONTROL* const pReadControl)
{
    assert(false);
    UNREFERENCED_PARAMETER(pInContext);
    UNREFERENCED_PARAMETER(pTextBuffer);
    UNREFERENCED_PARAMETER(TextBufferLength);
    *pTextBufferWritten = 0;
    UNREFERENCED_PARAMETER(pReadControl);
    return 0;
}

NTSTATUS IApiRoutines::WriteConsoleAImpl(_In_ IConsoleOutputObject* const pOutContext,
                                         _In_reads_(TextBufferLength) char* const pTextBuffer,
                                         _In_ ULONG const TextBufferLength,
                                         _Out_ ULONG* const pTextBufferRead)
{
    assert(false);
    UNREFERENCED_PARAMETER(pOutContext);
    UNREFERENCED_PARAMETER(pTextBuffer);
    *pTextBufferRead = TextBufferLength;
    return 0;
}

NTSTATUS IApiRoutines::WriteConsoleWImpl(_In_ IConsoleOutputObject* const pOutContext,
                                         _In_reads_(TextBufferLength) wchar_t* const pTextBuffer,
                                         _In_ ULONG const TextBufferLength,
                                         _Out_ ULONG* const pTextBufferRead)
{
    assert(false);
    UNREFERENCED_PARAMETER(pOutContext);
    UNREFERENCED_PARAMETER(pTextBuffer);
    *pTextBufferRead = TextBufferLength;
    return 0;
}

NTSTATUS IApiRoutines::FillConsoleOutputAttributeImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                      _In_ WORD const Attribute,
                                                      _In_ DWORD const LengthToWrite,
                                                      _In_ COORD const StartingCoordinate,
                                                      _Out_ DWORD* const pCellsModified)
{
    assert(false);
    UNREFERENCED_PARAMETER(pOutContext);
    UNREFERENCED_PARAMETER(Attribute);
    UNREFERENCED_PARAMETER(StartingCoordinate);
    *pCellsModified = LengthToWrite;
    return 0;
}

NTSTATUS IApiRoutines::FillConsoleOutputCharacterAImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                       _In_ char const Character,
                                                       _In_ DWORD const LengthToWrite,
                                                       _In_ COORD const StartingCoordinate,
                                                       _Out_ DWORD* const pCellsModified)
{
    assert(false);
    UNREFERENCED_PARAMETER(pOutContext);
    UNREFERENCED_PARAMETER(Character);
    UNREFERENCED_PARAMETER(StartingCoordinate);
    *pCellsModified = LengthToWrite;
    return 0;
}

NTSTATUS IApiRoutines::FillConsoleOutputCharacterWImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                       _In_ wchar_t const Character,
                                                       _In_ DWORD const LengthToWrite,
                                                       _In_ COORD const StartingCoordinate,
                                                       _Out_ DWORD* const pCellsModified)
{
    assert(false);
    UNREFERENCED_PARAMETER(pOutContext);
    UNREFERENCED_PARAMETER(Character);
    UNREFERENCED_PARAMETER(StartingCoordinate);
    *pCellsModified = LengthToWrite;
    return 0;
}

NTSTATUS IApiRoutines::GenerateConsoleCtrlEventImpl(_In_ ULONG const ProcessGroupFilter,
                                                    _In_ ULONG const ControlEvent)
{
    assert(false);
    UNREFERENCED_PARAMETER(ProcessGroupFilter);
    UNREFERENCED_PARAMETER(ControlEvent);
    return 0;
}

NTSTATUS IApiRoutines::SetConsoleActiveScreenBufferImpl(_In_ HANDLE const NewOutContext)
{
    assert(false);
    UNREFERENCED_PARAMETER(NewOutContext);
    return 0;
}

NTSTATUS IApiRoutines::FlushConsoleInputBuffer(_In_ IConsoleInputObject* const pInContext)
{
    assert(false);
    UNREFERENCED_PARAMETER(pInContext);
    return 0;
}

NTSTATUS IApiRoutines::SetConsoleInputCodePageImpl(_In_ ULONG const CodePage)
{
    assert(false);
    UNREFERENCED_PARAMETER(CodePage);
    return 0;
}

NTSTATUS IApiRoutines::SetConsoleOutputCodePageImpl(_In_ ULONG const CodePage)
{
    assert(false);
    UNREFERENCED_PARAMETER(CodePage);
    return 0;
}

NTSTATUS IApiRoutines::GetConsoleCursorInfoImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                _Out_ ULONG* const pCursorSize,
                                                _Out_ BOOLEAN* const pIsVisible)
{
    assert(false);
    UNREFERENCED_PARAMETER(pOutContext);
    *pCursorSize = 60;
    *pIsVisible = TRUE;
    return 0;
}

NTSTATUS IApiRoutines::SetConsoleCursorInfoImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                _In_ ULONG const CursorSize,
                                                _In_ BOOLEAN const IsVisible)
{
    assert(false);
    UNREFERENCED_PARAMETER(pOutContext);
    UNREFERENCED_PARAMETER(CursorSize);
    UNREFERENCED_PARAMETER(IsVisible);
    return 0;
}

NTSTATUS IApiRoutines::GetConsoleScreenBufferInfoExImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                        _Out_ CONSOLE_SCREEN_BUFFER_INFOEX* const pScreenBufferInfoEx)
{
    assert(false);
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

NTSTATUS IApiRoutines::SetConsoleScreenBufferInfoExImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                        _In_ const CONSOLE_SCREEN_BUFFER_INFOEX* const pScreenBufferInfoEx)
{
    assert(false);
    UNREFERENCED_PARAMETER(pOutContext);
    UNREFERENCED_PARAMETER(pScreenBufferInfoEx);
    return 0;
}

NTSTATUS IApiRoutines::SetConsoleScreenBufferSizeImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                      _In_ const COORD* const pSize)
{
    assert(false);
    UNREFERENCED_PARAMETER(pOutContext);
    UNREFERENCED_PARAMETER(pSize);
    return 0;
}

NTSTATUS IApiRoutines::SetConsoleCursorPositionImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                    _In_ const COORD* const pCursorPosition)
{
    assert(false);
    UNREFERENCED_PARAMETER(pOutContext);
    UNREFERENCED_PARAMETER(pCursorPosition);
    return 0;
}

NTSTATUS IApiRoutines::GetLargestConsoleWindowSizeImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                       _Out_ COORD* const pSize)
{
    assert(false);
    UNREFERENCED_PARAMETER(pOutContext);
    pSize->X = 120;
    pSize->Y = 30;
    return 0;
}

NTSTATUS IApiRoutines::ScrollConsoleScreenBufferAImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                      _In_ const SMALL_RECT* const pSourceRectangle,
                                                      _In_ const COORD* const pTargetOrigin,
                                                      _In_opt_ const SMALL_RECT* const pTargetClipRectangle,
                                                      _In_ char const chFill,
                                                      _In_ WORD const attrFill)
{
    assert(false);
    UNREFERENCED_PARAMETER(pOutContext);
    UNREFERENCED_PARAMETER(pSourceRectangle);
    UNREFERENCED_PARAMETER(pTargetOrigin);
    UNREFERENCED_PARAMETER(pTargetClipRectangle);
    UNREFERENCED_PARAMETER(chFill);
    UNREFERENCED_PARAMETER(attrFill);
    return 0;
}

NTSTATUS IApiRoutines::ScrollConsoleScreenBufferWImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                      _In_ const SMALL_RECT* const pSourceRectangle,
                                                      _In_ const COORD* const pTargetOrigin,
                                                      _In_opt_ const SMALL_RECT* const pTargetClipRectangle,
                                                      _In_ wchar_t const wchFill,
                                                      _In_ WORD const attrFill)
{
    assert(false);
    UNREFERENCED_PARAMETER(pOutContext);
    UNREFERENCED_PARAMETER(pSourceRectangle);
    UNREFERENCED_PARAMETER(pTargetOrigin);
    UNREFERENCED_PARAMETER(pTargetClipRectangle);
    UNREFERENCED_PARAMETER(wchFill);
    UNREFERENCED_PARAMETER(attrFill);
    return 0;
}

NTSTATUS IApiRoutines::SetConsoleTextAttributeImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                   _In_ WORD const Attribute)
{
    assert(false);
    UNREFERENCED_PARAMETER(pOutContext);
    UNREFERENCED_PARAMETER(Attribute);
    return 0;
}

NTSTATUS IApiRoutines::SetConsoleWindowInfoImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                _In_ BOOLEAN const IsAbsoluteRectangle,
                                                _In_ const SMALL_RECT* const pWindowRectangle)
{
    assert(false);
    UNREFERENCED_PARAMETER(pOutContext);
    UNREFERENCED_PARAMETER(IsAbsoluteRectangle);
    UNREFERENCED_PARAMETER(pWindowRectangle);
    return 0;
}

NTSTATUS IApiRoutines::ReadConsoleOutputAttributeImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                      _In_ const COORD* const pSourceOrigin,
                                                      _Out_writes_to_(AttributeBufferLength, *pAttributeBufferWritten) WORD* const pAttributeBuffer,
                                                      _In_ ULONG const AttributeBufferLength,
                                                      _Out_ ULONG* const pAttributeBufferWritten)
{
    assert(false);
    UNREFERENCED_PARAMETER(pOutContext);
    UNREFERENCED_PARAMETER(pSourceOrigin);
    UNREFERENCED_PARAMETER(pAttributeBuffer);
    UNREFERENCED_PARAMETER(AttributeBufferLength);
    *pAttributeBufferWritten = 0;
    return 0;
}

NTSTATUS IApiRoutines::ReadConsoleOutputCharacterAImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                       _In_ const COORD* const pSourceOrigin,
                                                       _Out_writes_to_(TextBufferLength, *pTextBufferWritten) char* const pTextBuffer,
                                                       _In_ ULONG const TextBufferLength,
                                                       _Out_ ULONG* const pTextBufferWritten)
{
    assert(false);
    UNREFERENCED_PARAMETER(pOutContext);
    UNREFERENCED_PARAMETER(pSourceOrigin);
    UNREFERENCED_PARAMETER(pTextBuffer);
    UNREFERENCED_PARAMETER(TextBufferLength);
    *pTextBufferWritten = 0;
    return 0;
}

NTSTATUS IApiRoutines::ReadConsoleOutputCharacterWImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                       _In_ const COORD* const pSourceOrigin,
                                                       _Out_writes_to_(TextBufferLength, *pTextBufferWritten) wchar_t* const pTextBuffer,
                                                       _In_ ULONG const TextBufferLength,
                                                       _Out_ ULONG* const pTextBufferWritten)
{
    assert(false);
    UNREFERENCED_PARAMETER(pOutContext);
    UNREFERENCED_PARAMETER(pSourceOrigin);
    UNREFERENCED_PARAMETER(pTextBuffer);
    UNREFERENCED_PARAMETER(TextBufferLength);
    *pTextBufferWritten = 0;
    return 0;
}

NTSTATUS IApiRoutines::WriteConsoleInputAImpl(_In_ IConsoleInputObject* const pInContext,
                                              _In_reads_(InputBufferLength) const INPUT_RECORD* const pInputBuffer,
                                              _In_ ULONG const InputBufferLength,
                                              _Out_ ULONG* const pInputBufferRead)
{
    assert(false);
    UNREFERENCED_PARAMETER(pInContext);
    UNREFERENCED_PARAMETER(pInputBuffer);
    *pInputBufferRead = InputBufferLength;
    return 0;
}

NTSTATUS IApiRoutines::WriteConsoleInputWImpl(_In_ IConsoleInputObject* const pInContext,
                                              _In_reads_(InputBufferLength) const INPUT_RECORD* const pInputBuffer,
                                              _In_ ULONG const InputBufferLength,
                                              _Out_ ULONG* const pInputBufferRead)
{
    assert(false);
    UNREFERENCED_PARAMETER(pInContext);
    UNREFERENCED_PARAMETER(pInputBuffer);
    *pInputBufferRead = InputBufferLength;
    return 0;
}

NTSTATUS IApiRoutines::WriteConsoleOutputAImpl(_In_ IConsoleOutputObject* const pOutContext,
                                               _In_reads_(pTextBufferSize->X * pTextBufferSize->Y) const CHAR_INFO* const pTextBuffer,
                                               _In_ const COORD* const pTextBufferSize,
                                               _In_ const COORD* const pTextBufferSourceOrigin,
                                               _In_ const SMALL_RECT* const pTargetRectangle,
                                               _Out_ SMALL_RECT* const pAffectedRectangle)
{
    assert(false);
    UNREFERENCED_PARAMETER(pOutContext);
    UNREFERENCED_PARAMETER(pTextBuffer);
    UNREFERENCED_PARAMETER(pTextBufferSize);
    UNREFERENCED_PARAMETER(pTextBufferSourceOrigin);
    UNREFERENCED_PARAMETER(pTargetRectangle);
    *pAffectedRectangle = { 0 };
    return 0;
}

NTSTATUS IApiRoutines::WriteConsoleOutputWImpl(_In_ IConsoleOutputObject* const pOutContext,
                                               _In_reads_(pTextBufferSize->X * pTextBufferSize->Y) const CHAR_INFO* const pTextBuffer,
                                               _In_ const COORD* const pTextBufferSize,
                                               _In_ const COORD* const pTextBufferSourceOrigin,
                                               _In_ const SMALL_RECT* const pTargetRectangle,
                                               _Out_ SMALL_RECT* const pAffectedRectangle)
{
    assert(false);
    UNREFERENCED_PARAMETER(pOutContext);
    UNREFERENCED_PARAMETER(pTextBuffer);
    UNREFERENCED_PARAMETER(pTextBufferSize);
    UNREFERENCED_PARAMETER(pTextBufferSourceOrigin);
    UNREFERENCED_PARAMETER(pTargetRectangle);
    *pAffectedRectangle = { 0 };
    return 0;
}

NTSTATUS IApiRoutines::WriteConsoleOutputAttributeImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                       _In_reads_(AttributeBufferLength) const WORD* const pAttributeBuffer,
                                                       _In_ ULONG const AttributeBufferLength,
                                                       _In_ const COORD* const pTargetOrigin,
                                                       _Out_ ULONG* const pAttributeBufferRead)
{
    assert(false);
    UNREFERENCED_PARAMETER(pOutContext);
    UNREFERENCED_PARAMETER(pAttributeBuffer);
    UNREFERENCED_PARAMETER(pTargetOrigin);
    *pAttributeBufferRead = AttributeBufferLength;
    return 0;
}

NTSTATUS IApiRoutines::WriteConsoleOutputCharacterAImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                        _In_reads_(TextBufferLength) const char* const pTextBuffer,
                                                        _In_ ULONG const TextBufferLength,
                                                        _In_ const COORD* const pTargetOrigin,
                                                        _Out_ ULONG* const pTextBufferRead)
{
    assert(false);
    UNREFERENCED_PARAMETER(pOutContext);
    UNREFERENCED_PARAMETER(pTextBuffer);
    UNREFERENCED_PARAMETER(pTargetOrigin);
    *pTextBufferRead = TextBufferLength;
    return 0;
}

NTSTATUS IApiRoutines::WriteConsoleOutputCharacterWImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                        _In_reads_(TextBufferLength) const wchar_t* const pTextBuffer,
                                                        _In_ ULONG const TextBufferLength,
                                                        _In_ const COORD* const pTargetOrigin,
                                                        _Out_ ULONG* const pTextBufferRead)
{
    assert(false);
    UNREFERENCED_PARAMETER(pOutContext);
    UNREFERENCED_PARAMETER(pTextBuffer);
    UNREFERENCED_PARAMETER(pTargetOrigin);
    *pTextBufferRead = TextBufferLength;
    return 0;
}

NTSTATUS IApiRoutines::ReadConsoleOutputA(_In_ IConsoleOutputObject* const pOutContext,
                                          _Out_writes_(pTextBufferSize->X * pTextBufferSize->Y) CHAR_INFO* const pTextBuffer,
                                          _In_ const COORD* const pTextBufferSize,
                                          _In_ const COORD* const pTextBufferTargetOrigin,
                                          _In_ const SMALL_RECT* const pSourceRectangle,
                                          _Out_ SMALL_RECT* const pReadRectangle)
{
    assert(false);
    UNREFERENCED_PARAMETER(pOutContext);
    UNREFERENCED_PARAMETER(pTextBuffer);
    UNREFERENCED_PARAMETER(pTextBufferSize);
    UNREFERENCED_PARAMETER(pTextBufferTargetOrigin);
    UNREFERENCED_PARAMETER(pSourceRectangle);
    *pReadRectangle = { 0, 0, -1, -1 }; // because inclusive rects, an empty has right and bottom at -1
    return 0;
}

NTSTATUS IApiRoutines::ReadConsoleOutputW(_In_ IConsoleOutputObject* const pOutContext,
                                          _Out_writes_(pTextBufferSize->X * pTextBufferSize->Y) CHAR_INFO* const pTextBuffer,
                                          _In_ const COORD* const pTextBufferSize,
                                          _In_ const COORD* const pTextBufferTargetOrigin,
                                          _In_ const SMALL_RECT* const pSourceRectangle,
                                          _Out_ SMALL_RECT* const pReadRectangle)
{
    assert(false);
    UNREFERENCED_PARAMETER(pOutContext);
    UNREFERENCED_PARAMETER(pTextBuffer);
    UNREFERENCED_PARAMETER(pTextBufferSize);
    UNREFERENCED_PARAMETER(pTextBufferTargetOrigin);
    UNREFERENCED_PARAMETER(pSourceRectangle);
    *pReadRectangle = { 0, 0, -1, -1 }; // because inclusive rects, an empty has right and bottom at -1
    return 0;
}

NTSTATUS IApiRoutines::GetConsoleTitleAImpl(_Out_writes_(TextBufferSize) char* const pTextBuffer,
                                            _In_ ULONG const TextBufferSize)
{
    assert(false);
    if (TextBufferSize > 0)
    {
        assert(false);
        *pTextBuffer = '\0';
    }
    return 0;
}

NTSTATUS IApiRoutines::GetConsoleTitleWImpl(_Out_writes_(TextBufferSize) wchar_t* const pTextBuffer,
                                            _In_ ULONG const TextBufferSize)
{
    assert(false);
    if (TextBufferSize > 0)
    {
        assert(false);
        *pTextBuffer = '\0';
    }
    return 0;
}

NTSTATUS IApiRoutines::GetConsoleOriginalTitleAImpl(_Out_writes_(TextBufferSize) char* const pTextBuffer,
                                                    _In_ ULONG const TextBufferSize)
{
    assert(false);
    if (TextBufferSize > 0)
    {
        assert(false);
        *pTextBuffer = '\0';
    }
    return 0;
}

NTSTATUS IApiRoutines::GetConsoleOriginalTitleWImpl(_Out_writes_(TextBufferSize) char* const pTextBuffer,
                                                    _In_ ULONG const TextBufferSize)
{
    assert(false);
    if (TextBufferSize > 0)
    {
        assert(false);
        *pTextBuffer = '\0';
    }
    return 0;
}

NTSTATUS IApiRoutines::SetConsoleTitleAImpl(_In_reads_(TextBufferSize) char* const pTextBuffer,
                                            _In_ ULONG const TextBufferSize)
{
    assert(false);
    UNREFERENCED_PARAMETER(pTextBuffer);
    UNREFERENCED_PARAMETER(TextBufferSize);
    return 0;
}

NTSTATUS IApiRoutines::SetConsoleTitleWImpl(_In_reads_(TextBufferSize) wchar_t* const pTextBuffer,
                                            _In_ ULONG const TextBufferSize)
{
    assert(false);
    UNREFERENCED_PARAMETER(pTextBuffer);
    UNREFERENCED_PARAMETER(TextBufferSize);
    return 0;
}

NTSTATUS IApiRoutines::GetNumberOfConsoleMouseButtonsImpl(_Out_ ULONG* const pButtons)
{
    assert(false);
    *pButtons = 2;
    return 0;
}

NTSTATUS IApiRoutines::GetConsoleFontSizeImpl(_In_ IConsoleOutputObject* const pOutContext,
                                              _In_ DWORD const FontIndex,
                                              _Out_ COORD* const pFontSize)
{
    assert(false);
    UNREFERENCED_PARAMETER(pOutContext);
    UNREFERENCED_PARAMETER(FontIndex);
    pFontSize->X = 8;
    pFontSize->Y = 12;
    return 0;
}

// driver will pare down for non-Ex method
NTSTATUS IApiRoutines::GetCurrentConsoleFontExImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                   _In_ BOOLEAN const IsForMaximumWindowSize,
                                                   _Out_ CONSOLE_FONT_INFOEX* const pConsoleFontInfoEx)
{
    assert(false);


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

NTSTATUS IApiRoutines::SetConsoleDisplayModeImpl(_In_ SCREEN_INFORMATION* const pContext, 
                                                 _In_ ULONG const Flags,
                                                 _Out_ COORD* const pNewScreenBufferSize)
{
    assert(false);
    UNREFERENCED_PARAMETER(pContext);
    UNREFERENCED_PARAMETER(Flags);
    pNewScreenBufferSize->X = 120;
    pNewScreenBufferSize->Y = 30;
    return 0;
}

NTSTATUS IApiRoutines::GetConsoleDisplayModeImpl(_In_ SCREEN_INFORMATION* const pContext, 
                                                 _Out_ ULONG* const pFlags)
{
    assert(false);
    UNREFERENCED_PARAMETER(pContext);
    *pFlags = 0;
    return 0;
}

NTSTATUS IApiRoutines::AddConsoleAliasAImpl(_In_reads_(SourceBufferLength) const char* const pSourceBuffer,
                                            _In_ ULONG const SourceBufferLength,
                                            _In_reads_(TargetBufferLength) const char* const pTargetBuffer,
                                            _In_ ULONG const TargetBufferLength,
                                            _In_reads_(ExeNameBufferLength) const char* const pExeNameBuffer,
                                            _In_ ULONG const ExeNameBufferLength)
{
    assert(false);
    UNREFERENCED_PARAMETER(pSourceBuffer);
    UNREFERENCED_PARAMETER(SourceBufferLength);
    UNREFERENCED_PARAMETER(pTargetBuffer);
    UNREFERENCED_PARAMETER(TargetBufferLength);
    UNREFERENCED_PARAMETER(pExeNameBuffer);
    UNREFERENCED_PARAMETER(ExeNameBufferLength);
    return 0;
}

NTSTATUS IApiRoutines::AddConsoleAliasWImpl(_In_reads_(SourceBufferLength) const wchar_t* const pSourceBuffer,
                                            _In_ ULONG const SourceBufferLength,
                                            _In_reads_(TargetBufferLength) const wchar_t* const pTargetBuffer,
                                            _In_ ULONG const TargetBufferLength,
                                            _In_reads_(ExeNameBufferLength) const wchar_t* const pExeNameBuffer,
                                            _In_ ULONG const ExeNameBufferLength)
{
    assert(false);
    UNREFERENCED_PARAMETER(pSourceBuffer);
    UNREFERENCED_PARAMETER(SourceBufferLength);
    UNREFERENCED_PARAMETER(pTargetBuffer);
    UNREFERENCED_PARAMETER(TargetBufferLength);
    UNREFERENCED_PARAMETER(pExeNameBuffer);
    UNREFERENCED_PARAMETER(ExeNameBufferLength);
    return 0;
}

NTSTATUS IApiRoutines::GetConsoleAliasAImpl(_In_reads_(SourceBufferLength) const char* const pSourceBuffer,
                                            _In_ ULONG const SourceBufferLength,
                                            _Out_writes_(TargetBufferLength) char* const pTargetBuffer,
                                            _In_ ULONG const TargetBufferLength,
                                            _In_reads_(ExeNameBufferLength) const char* const pExeNameBuffer,
                                            _In_ ULONG const ExeNameBufferLength)
{
    assert(false);
    UNREFERENCED_PARAMETER(pSourceBuffer);
    UNREFERENCED_PARAMETER(SourceBufferLength);
    UNREFERENCED_PARAMETER(pTargetBuffer);
    UNREFERENCED_PARAMETER(TargetBufferLength);
    UNREFERENCED_PARAMETER(pExeNameBuffer);
    UNREFERENCED_PARAMETER(ExeNameBufferLength);
    return 0;
}

NTSTATUS IApiRoutines::GetConsoleAliasWImpl(_In_reads_(SourceBufferLength) const wchar_t* const pSourceBuffer,
                                            _In_ ULONG const SourceBufferLength,
                                            _Out_writes_(TargetBufferLength) wchar_t* const pTargetBuffer,
                                            _In_ ULONG const TargetBufferLength,
                                            _In_reads_(ExeNameBufferLength) const wchar_t* const pExeNameBuffer,
                                            _In_ ULONG const ExeNameBufferLength)
{
    assert(false);
    UNREFERENCED_PARAMETER(pSourceBuffer);
    UNREFERENCED_PARAMETER(SourceBufferLength);
    UNREFERENCED_PARAMETER(pTargetBuffer);
    UNREFERENCED_PARAMETER(TargetBufferLength);
    UNREFERENCED_PARAMETER(pExeNameBuffer);
    UNREFERENCED_PARAMETER(ExeNameBufferLength);
    return 0;
}

NTSTATUS IApiRoutines::GetConsoleAliasesLengthAImpl(_In_reads_(ExeNameBufferLength) const char* const pExeNameBuffer,
                                                    _In_ ULONG const ExeNameBufferLength,
                                                    _Out_ ULONG* const pAliasesBufferRequired)
{
    assert(false);
    UNREFERENCED_PARAMETER(pExeNameBuffer);
    UNREFERENCED_PARAMETER(ExeNameBufferLength);
    *pAliasesBufferRequired = 0;
    return 0;
}

NTSTATUS IApiRoutines::GetConsoleAliasesLengthWImpl(_In_reads_(ExeNameBufferLength) const wchar_t* const pExeNameBuffer,
                                                    _In_ ULONG const ExeNameBufferLength,
                                                    _Out_ ULONG* const pAliasesBufferRequired)
{
    assert(false);
    UNREFERENCED_PARAMETER(pExeNameBuffer);
    UNREFERENCED_PARAMETER(ExeNameBufferLength);
    *pAliasesBufferRequired = 0;
    return 0;
}

NTSTATUS IApiRoutines::GetConsoleAliasExesLengthAImpl(_Out_ ULONG* const pAliasExesBufferRequired)
{
    assert(false);
    *pAliasExesBufferRequired = 0;
    return 0;
}

NTSTATUS IApiRoutines::GetConsoleAliasExesLengthWImpl(_Out_ ULONG* const pAliasExesBufferRequired)
{
    assert(false);
    *pAliasExesBufferRequired = 0;
    return 0;
}

NTSTATUS IApiRoutines::GetConsoleAliasesAImpl(_In_reads_(ExeNameBufferLength) const char* const pExeNameBuffer,
                                              _In_ ULONG const ExeNameBufferLength,
                                              _Out_writes_(AliasBufferLength) char* const pAliasBuffer,
                                              _In_ ULONG const AliasBufferLength)
{
    assert(false);
    UNREFERENCED_PARAMETER(pExeNameBuffer);
    UNREFERENCED_PARAMETER(ExeNameBufferLength);
    UNREFERENCED_PARAMETER(pAliasBuffer);
    UNREFERENCED_PARAMETER(AliasBufferLength);
    return 0;
}

NTSTATUS IApiRoutines::GetConsoleAliasesWImpl(_In_reads_(ExeNameBufferLength) const wchar_t* const pExeNameBuffer,
                                              _In_ ULONG const ExeNameBufferLength,
                                              _Out_writes_(AliasBufferLength) wchar_t* const pAliasBuffer,
                                              _In_ ULONG const AliasBufferLength)
{
    assert(false);
    UNREFERENCED_PARAMETER(pExeNameBuffer);
    UNREFERENCED_PARAMETER(ExeNameBufferLength);
    UNREFERENCED_PARAMETER(pAliasBuffer);
    UNREFERENCED_PARAMETER(AliasBufferLength);
    return 0;
}

NTSTATUS IApiRoutines::GetConsoleAliasExesAImpl(_Out_writes_(AliasExesBufferLength) char* const pAliasExesBuffer,
                                                _In_ ULONG const AliasExesBufferLength)
{
    assert(false);
    UNREFERENCED_PARAMETER(pAliasExesBuffer);
    UNREFERENCED_PARAMETER(AliasExesBufferLength);
    return 0;
}

NTSTATUS IApiRoutines::GetConsoleAliasExesWImpl(_Out_writes_(AliasExesBufferLength) wchar_t* const pAliasExesBuffer,
                                                _In_ ULONG const AliasExesBufferLength)
{
    assert(false);
    UNREFERENCED_PARAMETER(pAliasExesBuffer);
    UNREFERENCED_PARAMETER(AliasExesBufferLength);
    return 0;
}

NTSTATUS IApiRoutines::GetConsoleWindowImpl(_Out_ HWND* const pHwnd)
{
    assert(false);
    *pHwnd = (HWND)INVALID_HANDLE_VALUE; // sure, why not.
    return 0;
}

NTSTATUS IApiRoutines::GetConsoleSelectionInfoImpl(_Out_ CONSOLE_SELECTION_INFO* const pConsoleSelectionInfo)
{
    assert(false);
    pConsoleSelectionInfo->dwFlags = 0;
    pConsoleSelectionInfo->dwSelectionAnchor.X = 0;
    pConsoleSelectionInfo->dwSelectionAnchor.Y = 0;
    pConsoleSelectionInfo->srSelection.Top = 0;
    pConsoleSelectionInfo->srSelection.Left = 0;
    pConsoleSelectionInfo->srSelection.Bottom = 0;
    pConsoleSelectionInfo->srSelection.Right = 0;
    return 0;
}

NTSTATUS IApiRoutines::GetConsoleHistoryInfoImpl(_Out_ CONSOLE_HISTORY_INFO* const pConsoleHistoryInfo)
{
    assert(false);
    pConsoleHistoryInfo->cbSize = sizeof(CONSOLE_HISTORY_INFO);
    pConsoleHistoryInfo->dwFlags = 0;
    pConsoleHistoryInfo->HistoryBufferSize = 20;
    pConsoleHistoryInfo->NumberOfHistoryBuffers = 5;
    return 0;
}

NTSTATUS IApiRoutines::SetConsoleHistoryInfoImpl(_In_ const CONSOLE_HISTORY_INFO* const pConsoleHistoryInfo)
{
    assert(false);
    UNREFERENCED_PARAMETER(pConsoleHistoryInfo);
    return 0;
}

NTSTATUS IApiRoutines::SetCurrentConsoleFontExImpl(_In_ IConsoleOutputObject* const pOutContext,
                                                   _In_ BOOLEAN const IsForMaximumWindowSize,
                                                   _In_ const CONSOLE_FONT_INFOEX* const pConsoleFontInfoEx)
{
    assert(false);
    UNREFERENCED_PARAMETER(pOutContext);
    UNREFERENCED_PARAMETER(IsForMaximumWindowSize);
    UNREFERENCED_PARAMETER(pConsoleFontInfoEx);
    return 0;
}

